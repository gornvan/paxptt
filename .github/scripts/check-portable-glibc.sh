#!/usr/bin/env bash
# Fail if any ELF in a portable AppDir requires GLIBC newer than a cap (default 2.35 ≈ Ubuntu 22.04).
#
# Usage: check-portable-glibc.sh [--max MAJOR.MINOR] <AppDir-root>
#
set -euo pipefail

MAX_GLIBC="2.35"

usage() {
    cat >&2 <<'EOF'
Usage: check-portable-glibc.sh [--max MAJOR.MINOR] <AppDir-root>

Scans bundled ELF files (usr/bin/paxp2t, usr/lib/**/*.so*) for GLIBC_* symbol versions.
Exits 1 if any required version is greater than --max (default 2.35).
EOF
}

while [[ "${1:-}" == -* ]]; do
    case "$1" in
        --max)
            MAX_GLIBC="${2:?--max requires MAJOR.MINOR}"
            shift 2
            ;;
        -h | --help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

APPDIR="${1:-}"
if [[ -z "${APPDIR}" || ! -d "${APPDIR}" ]]; then
    echo "check-portable-glibc.sh: missing AppDir directory" >&2
    usage >&2
    exit 2
fi

if ! command -v objdump >/dev/null 2>&1; then
    echo "check-portable-glibc.sh: objdump not found (install binutils)" >&2
    exit 2
fi

is_elf() {
    local f="$1"
    [[ -f "$f" ]] && file -b "$f" 2>/dev/null | grep -q 'ELF'
}

collect_glibc_versions() {
    local elf="$1"
    objdump -T "$elf" 2>/dev/null \
        | sed -n 's/.*\(GLIBC_2\.[0-9][0-9]*\).*/\1/p' \
        | sort -u
}

version_gt() {
    # true if $1 > $2 (e.g. GLIBC_2.38 vs GLIBC_2.35)
    local a="${1#GLIBC_}" b="${2#GLIBC_}"
    [[ "$(printf '%s\n%s\n' "$b" "$a" | sort -V | tail -1)" == "$a" && "$a" != "$b" ]]
}

declare -A offenders=()

scan_elf() {
    local elf="$1"
    [[ -n "${offenders[$elf]:-}" ]] && return 0
    local ver worst=""
    while IFS= read -r ver; do
        [[ -z "$ver" ]] && continue
        if version_gt "$ver" "GLIBC_${MAX_GLIBC}"; then
            if [[ -z "$worst" ]] || version_gt "$ver" "$worst"; then
                worst="$ver"
            fi
        fi
    done < <(collect_glibc_versions "$elf")
    if [[ -n "$worst" ]]; then
        offenders["$elf"]="$worst"
    fi
}

if is_elf "${APPDIR}/usr/bin/paxp2t"; then
    scan_elf "${APPDIR}/usr/bin/paxp2t"
fi

while IFS= read -r -d '' elf; do
    is_elf "$elf" && scan_elf "$elf"
done < <(find "${APPDIR}/usr/lib" -type f \( -name 'lib*.so' -o -name 'lib*.so.*' \) -print0 2>/dev/null || true)

if [[ ${#offenders[@]} -eq 0 ]]; then
    echo "check-portable-glibc.sh: OK (no GLIBC > ${MAX_GLIBC} in bundled ELFs)"
    exit 0
fi

echo "check-portable-glibc.sh: bundled libraries require GLIBC newer than ${MAX_GLIBC}:" >&2
for elf in $(printf '%s\n' "${!offenders[@]}" | LC_ALL=C sort); do
    echo "  ${elf}: needs ${offenders[$elf]} or newer" >&2
done
echo "Pin Release CI to ubuntu-22.04 or rebuild Qt bundle on an older baseline." >&2
exit 1
