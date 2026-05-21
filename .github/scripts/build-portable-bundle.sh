#!/usr/bin/env bash
# Build portable AppDir tarball (linuxdeploy + Qt plugin).
# Intended for CI and local runs — same steps as publishing a GitHub Release asset.
#
# Usage (from repo root):
#   .github/scripts/build-portable-bundle.sh [RELEASE_VERSION]
#
# Dependencies (install yourself on non-Ubuntu; CI installs via workflow):
#   cmake, compilers, qt6-base-dev, qt6-svg-dev, libx11-dev, libxtst-dev, curl,
#   strip (binutils), file, ImageMagick (convert) — optional if packaging/paxp2t.png exists
#
set -euo pipefail

usage() {
    cat >&2 <<'EOF'
Build portable AppDir tarball (same as GitHub Release asset).

Usage:
  .github/scripts/build-portable-bundle.sh [RELEASE_VERSION]

From repo root, optional RELEASE_VERSION defaults to git describe or local-<timestamp>.

Environment:
  BUILD_DIR                CMake binary dir (default: build-cpp, relative to repo root or absolute)
  OUT_DIR                  Where to write tarball (default: dist)
  PAXP2T_RELEASE_TOOLS_DIR Cache for linuxdeploy AppImages (default: ~/.cache/paxp2t-release-tools)
  REGENERATE_PAXP2T_ICON=1 Replace packaging/paxp2t.png even if it exists

  -h, --help               Show this help
EOF
}

while [[ "${1:-}" == -* ]]; do
    case "$1" in
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

RELEASE_VERSION="${1:-}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
cd "${REPO_ROOT}"

BUILD_DIR="${BUILD_DIR:-build-cpp}"
OUT_DIR="${OUT_DIR:-dist}"
TOOLS_DIR="${PAXP2T_RELEASE_TOOLS_DIR:-${XDG_CACHE_HOME:-$HOME/.cache}/paxp2t-release-tools}"

mkdir -p "${TOOLS_DIR}"

if [[ "${BUILD_DIR}" = /* ]]; then
    BIN_DIR="${BUILD_DIR}"
else
    BIN_DIR="${REPO_ROOT}/${BUILD_DIR}"
fi

if [[ -z "${RELEASE_VERSION}" ]]; then
    RELEASE_VERSION="$(git describe --tags --always --dirty 2>/dev/null || true)"
fi
if [[ -z "${RELEASE_VERSION}" ]]; then
    RELEASE_VERSION="local-$(date +%Y%m%d%H%M%S)"
fi

DESKTOP="${REPO_ROOT}/packaging/paxp2t.desktop"
if [[ ! -f "${DESKTOP}" ]]; then
    echo "Missing desktop file (commit it): ${DESKTOP}" >&2
    exit 1
fi

PNG="${REPO_ROOT}/packaging/paxp2t.png"
mkdir -p "${REPO_ROOT}/packaging"

if [[ -f "${PNG}" ]] && [[ "${REGENERATE_PAXP2T_ICON:-}" != "1" ]]; then
    echo "Using existing ${PNG}"
elif command -v convert >/dev/null 2>&1; then
    echo "Generating placeholder icon..."
    convert -size 128x128 xc:none \
        -fill '#44BB44' \
        -draw 'circle 64,64 64,10' \
        "${PNG}"
else
    echo "ImageMagick ''convert'' not found and packaging/paxp2t.png missing." >&2
    echo "Install imagemagick or add packaging/paxp2t.png, or copy an icon manually." >&2
    exit 1
fi

echo "CMake configure (${BUILD_DIR})..."
cmake -S cpp -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DPAXP2T_RELEASE_MINIMAL=ON

echo "Build..."
cmake --build "${BUILD_DIR}" -j "$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

EXE_ABS="${BIN_DIR}/paxp2t"
if [[ ! -f "${EXE_ABS}" ]]; then
    echo "Binary not found: ${EXE_ABS}" >&2
    exit 1
fi

strip --strip-unneeded "${EXE_ABS}"

LINUXDEPLOY="${TOOLS_DIR}/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_QT="${TOOLS_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage"
if [[ ! -f "${LINUXDEPLOY}" ]]; then
    echo "Fetching linuxdeploy..."
    curl -fsSL -o "${LINUXDEPLOY}" \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
fi
if [[ ! -f "${LINUXDEPLOY_QT}" ]]; then
    echo "Fetching linuxdeploy-plugin-qt..."
    curl -fsSL -o "${LINUXDEPLOY_QT}" \
        "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
fi
chmod +x "${LINUXDEPLOY}" "${LINUXDEPLOY_QT}"

QMAKE="$(command -v qmake6 || command -v qmake || true)"
if [[ -z "${QMAKE}" ]]; then
    echo "qmake not found (install Qt development packages)." >&2
    exit 1
fi
export QMAKE

export APPIMAGE_EXTRACT_AND_RUN=1

APPDIR_NAME="AppDir"
rm -rf "${REPO_ROOT}/${APPDIR_NAME}"

echo "Bundling portable AppDir (linuxdeploy Qt plugin, QMAKE=${QMAKE})..."
(
    cd "${TOOLS_DIR}"
    "${LINUXDEPLOY}" \
        --appdir "${REPO_ROOT}/${APPDIR_NAME}" \
        -e "${EXE_ABS}" \
        -d "${DESKTOP}" \
        -i "${PNG}" \
        --plugin qt
)

echo "Stripping bundled shared libraries..."
find "${REPO_ROOT}/${APPDIR_NAME}" -type f \( -name '*.so' -o -name '*.so.*' \) -print0 \
    | while IFS= read -r -d '' f; do
        strip --strip-unneeded "$f" 2>/dev/null || true
    done

PORTABLE_TOP="paxp2t-${RELEASE_VERSION}-linux-x86_64-portable"
ARCHIVE_BASENAME="paxp2t-${RELEASE_VERSION}-linux-x86_64-portable.tar.gz"
ARCHIVE_PATH="${OUT_DIR}/${ARCHIVE_BASENAME}"

rm -rf "${REPO_ROOT:?}/${PORTABLE_TOP}"
mv "${REPO_ROOT}/${APPDIR_NAME}" "${REPO_ROOT}/${PORTABLE_TOP}"

mkdir -p "${OUT_DIR}"
tar -czvf "${ARCHIVE_PATH}" -C "${REPO_ROOT}" "${PORTABLE_TOP}"
rm -rf "${REPO_ROOT:?}/${PORTABLE_TOP}"

echo "Done:"
echo "  ${ARCHIVE_PATH}"
