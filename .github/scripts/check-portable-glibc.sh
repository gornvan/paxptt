#!/usr/bin/env bash
# Report (and optionally cap) the highest GLIBC_x.y symbol version required by
# bundled ELFs in a portable AppDir. Host libc is never bundled — this guards
# CI builds that accidentally pull Qt/libs from too new a distro (ubuntu-latest).
#
# Usage: check-portable-glibc.sh [--max 2.35] <AppDir-root>
set -euo pipefail

MAX_GLIBC="${PAXP2T_MAX_GLIBC:-2.35}"

usage() {
    cat <<EOF
Usage: check-portable-glibc.sh [--max VERSION] <AppDir-root>

  --max VERSION   Fail if any bundled ELF needs GLIBC > VERSION (default: ${MAX_GLIBC}).

Environment:
  PAXP2T_MAX_GLIBC   Same as --max (e.g. 2.35 for Ubuntu 22.04 baseline).

Exit status:
  0  Within limit (or no GLIBC versioned symbols found).
  1  A bundled ELF requires GLIBC above the cap.
  2  Usage / invalid arguments.
EOF
}

# Highest GLIBC_x.y referenced by undefined/versioned symbols in ELF $1.
_elf_max_glibc() {
    local f="$1"
    objdump -T "$f" 2>/dev/null \
        | awk '$NF ~ /^GLIBC_[0-9]/ { sub(/^GLIBC_/, "", $NF); print $NF }' \
        | LC_ALL=C sort -t. -k1,1n -k2,2n \
        | tail -n 1
}

_is_elf() {
    local p="$1"
    [[ -f "$p" && -r "$p" ]] || return 1
    local sig
    sig="$(head -c 4 "$p" 2>/dev/null || true)"
    [[ "${#sig}" -eq 4 && "$sig" == $'\x7fELF' ]]
}

_audit() {
    local appdir="$1"
    local cap="$2"
    local f ver overall="" worst_file=""

    if ! command -v objdump >/dev/null 2>&1; then
        echo "check-portable-glibc.sh: objdump not found (install binutils)" >&2
        return 1
    fi

    while IFS= read -r -d '' f; do
        _is_elf "$f" || continue
        ver="$(_elf_max_glibc "$f")"
        [[ -z "$ver" ]] && continue
        if [[ -z "$overall" ]] || [[ "$(printf '%s\n' "$overall" "$ver" | LC_ALL=C sort -t. -k1,1n -k2,2n | tail -1)" != "$overall" ]]; then
            overall="$ver"
            worst_file="$f"
        elif [[ "$ver" == "$overall" ]]; then
            worst_file="$f"
        fi
    done < <(
        find "${appdir}/usr/bin" "${appdir}/usr/lib" "${appdir}/usr/plugins" \
            -type f \( -name 'paxp2t' -o -name '*.so' -o -name '*.so.*' \) \
            -print0 2>/dev/null || true
    )

    echo "check-portable-glibc.sh: appdir=${appdir}"
    if [[ -z "$overall" ]]; then
        echo "  max GLIBC required by bundled ELFs: (none detected)"
        return 0
    fi

    echo "  max GLIBC required by bundled ELFs: ${overall} (e.g. ${worst_file#"${appdir}/"})"
    echo "  policy cap:                         ${cap} (host libc must be >= max; build on older CI to lower max)"

    if [[ "$(printf '%s\n' "$cap" "$overall" | LC_ALL=C sort -t. -k1,1n -k2,2n | tail -1)" == "$overall" && "$overall" != "$cap" ]]; then
        echo >&2
        echo "FAILED: bundled ELFs need GLIBC_${overall} but cap is ${cap}." >&2
        echo "Rebuild on an older baseline (Release CI uses ubuntu-22.04) or raise --max knowingly." >&2
        echo "Note: bundling libc.so.6 in the AppDir is not supported — lower the requirement instead." >&2
        return 1
    fi

    echo "OK: bundled GLIBC requirement ${overall} <= cap ${cap}."
    return 0
}

main() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -h | --help)
                usage
                exit 0
                ;;
            --max)
                [[ $# -ge 2 ]] || {
                    echo "--max requires a version" >&2
                    exit 2
                }
                MAX_GLIBC="$2"
                shift 2
                ;;
            --)
                shift
                break
                ;;
            -*)
                echo "Unknown option: $1" >&2
                usage >&2
                exit 2
                ;;
            *)
                break
                ;;
        esac
    done

    if [[ $# -ne 1 ]]; then
        usage >&2
        exit 2
    fi
    if [[ ! -d "$1" ]]; then
        echo "$1: not an AppDir directory" >&2
        exit 1
    fi

    local appdir
    appdir="$(cd "$1" && pwd)"
    if _audit "$appdir" "$MAX_GLIBC"; then
        exit 0
    fi
    exit 1
}

main "$@"
