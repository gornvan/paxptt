#!/usr/bin/env bash
# Drop unused ELF shared libraries left in AppDir/usr/lib after linuxdeploy.
#
# Keeps every .so reachable from usr/bin/paxp2t and any usr/plugins/**/*.so*
# via transitive ldd when LD_LIBRARY_PATH is AppDir/usr/lib first.
# Everything else directly under usr/lib goes (plus broken symlinks).
#
# Usage: prune-appdir-libs.sh <AppDir-root>
set -euo pipefail

usage() {
    echo "usage: prune-appdir-libs.sh <AppDir-root>" >&2
}

is_elf_so() {
    local p="$1"
    [[ -f "$p" && -r "$p" ]] || return 1
    local sig
    sig="$(head -c 4 "$p" 2>/dev/null || true)"
    [[ "${#sig}" -eq 4 && "$sig" == $'\x7fELF' ]]
}

# Echo one absolute resolved library path per line from ldd output for $1.
_ldd_dep_paths() {
    local line rhs first
    while IFS= read -r line || [[ -n "$line" ]]; do
        line="${line#"${line%%[![:space:]]*}"}"
        [[ -z "$line" ]] && continue
        case "$line" in
            *"not a dynamic executable"*) continue ;;
            *"statically linked"*) continue ;;
            *linux-vdso*) continue ;;
            *"=> not found"*) continue ;;
        esac
        if [[ "$line" == *"=>"* ]]; then
            rhs="${line#*"=>"}"
            rhs="${rhs#"${rhs%%[![:space:]]*}"}"
            read -r first _ <<<"$rhs"
            first="${first%%(*}"
            first="${first%"${first##*[![:space:]]}"}"
            [[ "$first" == /* ]] && echo "$first"
        fi
    done < <(
        if command -v timeout >/dev/null 2>&1; then
            timeout 120 ldd "$1" 2>/dev/null || true
        else
            ldd "$1" 2>/dev/null || true
        fi
    )
}

# argv: appdir (absolute). Prints "removed inspected" for stdout parsing.
_run_prune() {
    local appdir="$1"
    local lib_dir exe p canon cur removed inspected nm path

    lib_dir="${appdir}/usr/lib"
    if [[ ! -d "$lib_dir" ]]; then
        echo "0 0"
        return 0
    fi

    local prefix="${appdir}/"
    declare -A reachable
    declare -a queue

    # Seeds + BFS enqueue (inline so locals stay reliable across bash versions).
    _try_track() {
        local c="$1"
        [[ "$c" == "${prefix}"* ]] || return 0
        [[ -f "$c" ]] || return 0
        is_elf_so "$c" || return 0
        [[ -n "${reachable[$c]+x}" ]] && return 0
        reachable[$c]=1
        queue+=("$c")
    }

    exe="${appdir}/usr/bin/paxp2t"
    if [[ -f "$exe" ]]; then
        canon="$(readlink -f "$exe" 2>/dev/null || true)"
        [[ -n "$canon" ]] && _try_track "$canon"
    fi

    if [[ -d "${appdir}/usr/plugins" ]]; then
        while IFS= read -r -d '' p; do
            canon="$(readlink -f "$p" 2>/dev/null || true)"
            [[ -n "$canon" ]] && _try_track "$canon"
        done < <(find "${appdir}/usr/plugins" -type f \( -name '*.so' -o -name '*.so.*' \) -print0 2>/dev/null || true)
    fi

    export LD_LIBRARY_PATH="${lib_dir}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"

    local -i qi=0
    while ((qi < ${#queue[@]})); do
        cur="${queue[qi++]}"
        while IFS= read -r dep; do
            [[ -z "$dep" ]] && continue
            canon="$(readlink -f "$dep" 2>/dev/null || true)"
            [[ -n "$canon" ]] && _try_track "$canon"
        done < <(_ldd_dep_paths "$cur")
    done

    removed=0
    inspected=0

    while IFS= read -r path; do
        [[ -z "$path" ]] && continue
        nm="$(basename "$path")"
        [[ "$nm" == lib* ]] || continue
        [[ "$nm" == *.so* ]] || continue
        inspected=$((inspected + 1))
        if ! canon="$(readlink -f "$path" 2>/dev/null)"; then
            rm -f "$path" 2>/dev/null && removed=$((removed + 1)) || true
            continue
        fi
        if [[ -z "${reachable[$canon]+x}" ]]; then
            rm -f "$path" 2>/dev/null && removed=$((removed + 1)) || true
        fi
    done < <(
        find "${lib_dir}" -maxdepth 1 -mindepth 1 ! -type d -name 'lib*.so*' \
            -printf '%f\t%p\n' 2>/dev/null |
            LC_ALL=C sort -t $'\t' -k1 -sr |
            cut -f2-
    )

    shopt -s nullglob
    for path in "${lib_dir}"/*; do
        [[ -e "$path" ]] && continue # exists (file, dir, ok symlink target)
        [[ -L "$path" ]] || continue # broken symlink only
        rm -f "$path" 2>/dev/null && removed=$((removed + 1)) || true
    done
    shopt -u nullglob

    printf '%s %s\n' "$removed" "$inspected"
}

main() {
    local appdir removed scanned
    if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
        usage
        return 2
    fi
    if [[ $# -ne 1 ]]; then
        usage
        return 2
    fi
    appdir="$1"
    if [[ ! -d "$appdir" ]]; then
        echo "${appdir}: not an AppDir directory" >&2
        return 1
    fi
    appdir="$(cd "$appdir" && pwd)"
    read -r removed scanned < <(_run_prune "$appdir")
    echo "prune-appdir-libs.sh: removed ${removed} usr/lib artefact(s) (examined ~${scanned})."
}

main "$@"
