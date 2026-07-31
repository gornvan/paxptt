#!/usr/bin/env bash
# Audit portable AppDir NEEDED dependencies via transitive ldd (same seeds as prune-appdir-libs.sh).
#
# What this catches:
#   - DT_NEEDED edges from usr/bin/paxp2t and every usr/plugins/**/*.so* (transitive for ELFs inside the AppDir).
#
# What this does NOT catch (use LD_DEBUG=libs while exercising the app for those):
#   - dlopen() / Qt QPluginLoader paths chosen only at runtime (e.g. a plugin removed from disk but never loaded).
#   - NSS modules, GL/Vulkan drivers, IBus bridges loaded indirectly after startup.
#
# Usage: check-portable-ldd.sh [--strict|--hybrid] [--fail-orphans] [--verbose] <AppDir-root>
set -euo pipefail

MODE="${PAXP2T_LDD_MODE:-hybrid}"
VERBOSE=0
FAIL_ORPHANS=0
if [[ "${PAXP2T_LDD_FAIL_ORPHANS:-}" == "1" ]]; then
    FAIL_ORPHANS=1
fi

usage() {
    cat <<'EOF'
Audit NEEDED dependencies for a portable AppDir (transitive ldd).

Usage:
  check-portable-ldd.sh [--strict|--hybrid] [--fail-orphans] [--verbose] <AppDir-root>

Modes:
  hybrid (default)  Allow typical host baseline: glibc, libstdc++, X11 core, PulseAudio, Mesa GL, fonts.
  strict            Every resolved .so must live under the AppDir (except ld-linux / vdso).

Environment:
  PAXP2T_LDD_MODE=strict|hybrid       Same as flags above.
  PAXP2T_LDD_FAIL_ORPHANS=1           Treat orphan usr/lib blobs as failure (trim drift).

Exit status:
  0  NEEDED closure OK for the selected mode (no missing SONAMEs).
  1  At least one SONAME unresolved (ldd => not found), and/or (strict) disallowed host
     resolution, and/or (--fail-orphans) orphan usr/lib libraries.
  2  Usage / invalid arguments.

Runtime audit (dlopen — not visible to ldd alone):
  LD_DEBUG=libs LD_LIBRARY_PATH="$APPD/usr/lib" "$APPD/usr/bin/paxp2t" 2>&1 | tee /tmp/paxp2t-ld.log
  grep -E 'calling init:|file=' /tmp/paxp2t-ld.log
EOF
}

is_elf() {
    local p="$1"
    [[ -f "$p" && -r "$p" ]] || return 1
    local sig
    sig="$(head -c 4 "$p" 2>/dev/null || true)"
    [[ "${#sig}" -eq 4 && "$sig" == $'\x7fELF' ]]
}

_ldd_lines() {
    if command -v timeout >/dev/null 2>&1; then
        timeout 120 ldd "$1" 2>&1 || true
    else
        ldd "$1" 2>&1 || true
    fi
}

# Parse ldd output: prints "soname<TAB>resolved_path" (path empty if not found).
_parse_ldd() {
    local line soname path rhs first
    while IFS= read -r line || [[ -n "$line" ]]; do
        line="${line#"${line%%[![:space:]]*}"}"
        [[ -z "$line" ]] && continue
        case "$line" in
            *linux-vdso*) continue ;;
            *"not a dynamic executable"*) continue ;;
            *"statically linked"*) continue ;;
        esac
        soname="${line%%[[:space:]]*}"
        soname="${soname%%:*}"
        if [[ "$line" == *"=> not found"* ]]; then
            printf '%s\t\n' "$soname"
            continue
        fi
        if [[ "$line" == *"=>"* ]]; then
            rhs="${line#*"=>"}"
            rhs="${rhs#"${rhs%%[![:space:]]*}"}"
            read -r first _ <<<"$rhs"
            path="${first%%(*}"
            path="${path%"${path##*[![:space:]]}"}"
            printf '%s\t%s\n' "$soname" "$path"
            continue
        fi
        # "libfoo.so.6 (0x...)" — already mapped (ld-linux itself).
        if [[ "$soname" == ld-linux* ]]; then
            continue
        fi
    done
}

_hybrid_allows_host() {
    local soname="$1"
    case "$soname" in
        ld-linux* | linux-vdso*) return 0 ;;
        libc.so.6 | libm.so.6 | libpthread.so.0 | libdl.so.2 | librt.so.1 | libresolv.so.2) return 0 ;;
        libstdc++.so.6 | libgcc_s.so.1) return 0 ;;
        libX11.so.6 | libXext.so.6 | libXau.so.6 | libxcb.so.1 | libX11-xcb.so.1) return 0 ;;
        libICE.so.6 | libSM.so.6) return 0 ;;
        libpulse.so.0 | libpulse-simple.so.0) return 0 ;;
        libpulsecommon-*.so*) return 0 ;;
        libEGL.so.1 | libGLX.so.0 | libOpenGL.so.0 | libGLdispatch.so.0) return 0 ;;
        libfontconfig.so.1 | libfreetype.so.6 | libharfbuzz.so.0 | libgraphite2.so.3 | libexpat.so.1) return 0 ;;
        libz.so.1 | libbz2.so.1 | libbrotlicommon.so.1 | libbrotlidec.so.1) return 0 ;;
        libuuid.so.1 | libgpg-error.so.0) return 0 ;;
    esac
    return 1
}

_audit() {
    local appdir="$1"
    local lib_dir="${appdir}/usr/lib"
    local prefix="${appdir}/"
    local exe="${appdir}/usr/bin/paxp2t"

    declare -A visited_elf=()
    declare -a queue=()
    declare -A missing_seen=() host_seen=() bundled_seen=() orphan_seen=()
    declare -A host_from=() # soname -> "seed=>path"

    _queue_elf() {
        local c="$1"
        [[ "$c" == "${prefix}"* ]] || return 0
        [[ -f "$c" ]] || return 0
        is_elf "$c" || return 0
        [[ -n "${visited_elf["$c"]+x}" ]] && return 0
        visited_elf["$c"]=1
        queue+=("$c")
    }

    if [[ -f "$exe" ]]; then
        local canon
        canon="$(readlink -f "$exe" 2>/dev/null || true)"
        [[ -n "$canon" ]] && _queue_elf "$canon"
    else
        echo "check-portable-ldd.sh: missing ${exe}" >&2
        return 1
    fi

    if [[ -d "${appdir}/usr/plugins" ]]; then
        local p canon
        while IFS= read -r -d '' p; do
            canon="$(readlink -f "$p" 2>/dev/null || true)"
            [[ -n "$canon" ]] && _queue_elf "$canon"
        done < <(find "${appdir}/usr/plugins" -type f \( -name '*.so' -o -name '*.so.*' \) -print0 2>/dev/null || true)
    fi

    export LD_LIBRARY_PATH="${lib_dir}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"

    local cur soname path canon
    local -i qi=0
    while ((qi < ${#queue[@]})); do
        cur="${queue[qi]}"
        qi=$((qi + 1))
        while IFS=$'\t' read -r soname path; do
            [[ -z "$soname" ]] && continue
            if [[ -z "$path" ]]; then
                missing_seen["${cur} :: ${soname}"]=1
                continue
            fi
            if [[ "$path" == "${prefix}"* ]]; then
                bundled_seen["${soname}=>${path}"]=1
                canon="$(readlink -f "$path" 2>/dev/null || true)"
                [[ -n "$canon" ]] && _queue_elf "$canon"
                continue
            fi
            host_seen["${soname}=>${path}"]=1
            host_from["${soname}"]="${cur} => ${path}"
        done < <(_parse_ldd < <(_ldd_lines "$cur"))
    done

    # usr/lib entries not reachable from seeds (trim drift / stale blobs).
    if [[ -d "$lib_dir" ]]; then
        local libpath nm lcanon
        while IFS= read -r -d '' libpath; do
            nm="$(basename "$libpath")"
            [[ "$nm" == lib*.so* ]] || continue
            lcanon="$(readlink -f "$libpath" 2>/dev/null || true)"
            [[ -n "$lcanon" && -n "${visited_elf["$lcanon"]+x}" ]] && continue
            orphan_seen["$nm"]=1
        done < <(find "$lib_dir" -maxdepth 1 -mindepth 1 ! -type d -name 'lib*.so*' -print0 2>/dev/null || true)
    fi

    local -i n_seeds=${#queue[@]} n_bundled=${#bundled_seen[@]} n_host=${#host_seen[@]} n_missing=${#missing_seen[@]} n_orphan=${#orphan_seen[@]}
    local -i fail=0

    echo "check-portable-ldd.sh: mode=${MODE} appdir=${appdir}"
    echo "  seeds (exe + plugins): ${n_seeds} ELF(s) in closure walk"
    echo "  bundled resolutions:   ${n_bundled}"
    echo "  host resolutions:      ${n_host}"
    echo "  missing (not found):   ${n_missing}"
    echo "  orphan usr/lib libs:   ${n_orphan} (present but not in NEEDED closure from seeds)"

    if ((n_missing > 0)); then
        echo >&2
        echo "MISSING (ldd => not found):" >&2
        local k
        for k in "${!missing_seen[@]}"; do
            echo "  ${k}" >&2
        done
        fail=1
    fi

    if ((n_host > 0)); then
        echo >&2
        if [[ "$MODE" == "strict" ]]; then
            echo "HOST (disallowed in strict mode):" >&2
        else
            echo "HOST (allowed baseline in hybrid mode marked with ~):" >&2
        fi
        local entry soname respath allowed=0 disallowed=0
        for entry in "${!host_seen[@]}"; do
            soname="${entry%%=>*}"
            respath="${entry#*=>}"
            if [[ "$MODE" == "hybrid" ]] && _hybrid_allows_host "$soname"; then
                echo "  ~ ${soname} => ${respath}" >&2
                allowed=$((allowed + 1))
            else
                echo "  ! ${soname} => ${respath}" >&2
                disallowed=$((disallowed + 1))
                fail=1
            fi
        done
        if [[ "$MODE" == "hybrid" && $disallowed -eq 0 ]]; then
            echo "  (${allowed} expected baseline host lib(s); ${disallowed} unexpected)" >&2
        fi
    fi

    if ((n_orphan > 0)); then
        echo >&2
        if ((FAIL_ORPHANS)); then
            echo "ORPHAN usr/lib (trim drift — failing):" >&2
        else
            echo "ORPHAN usr/lib (safe to drop if trim is correct; warn only):" >&2
        fi
        local o
        for o in "${!orphan_seen[@]}"; do
            echo "  ${o}" >&2
        done
        if ((FAIL_ORPHANS)); then
            fail=1
        fi
    fi

    if ((VERBOSE)); then
        echo
        echo "Bundled resolutions:"
        local b
        for b in $(printf '%s\n' "${!bundled_seen[@]}" | LC_ALL=C sort); do
            echo "  ${b}"
        done
    fi

    if ((fail == 0)); then
        echo
        echo "OK: NEEDED closure looks consistent for mode=${MODE}."
        echo "Note: ldd does not prove runtime dlopen paths; exercise the app with LD_DEBUG=libs if unsure."
    else
        echo >&2
        echo "FAILED: unresolved SONAME(s) and/or policy violation above (see exit status in --help)." >&2
    fi

    return "$fail"
}

main() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -h | --help)
                usage
                return 0
                ;;
            --strict)
                MODE=strict
                shift
                ;;
            --hybrid)
                MODE=hybrid
                shift
                ;;
            --fail-orphans)
                FAIL_ORPHANS=1
                shift
                ;;
            --verbose | -v)
                VERBOSE=1
                shift
                ;;
            --)
                shift
                break
                ;;
            -*)
                echo "Unknown option: $1" >&2
                usage >&2
                return 2
                ;;
            *)
                break
                ;;
        esac
    done

    if [[ $# -ne 1 ]]; then
        usage >&2
        return 2
    fi
    if [[ ! -d "$1" ]]; then
        echo "$1: not an AppDir directory" >&2
        return 1
    fi
    local appdir
    appdir="$(cd "$1" && pwd)"
    if _audit "$appdir"; then
        exit 0
    fi
    exit 1
}

main "$@"
