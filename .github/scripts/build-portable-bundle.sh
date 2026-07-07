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
  PAXP2T_SKIP_BUNDLE_TRIM=1  Skip aggressive post-bundle trimming (translations, extras)
  PAXP2T_SKIP_LDD_CHECK=1    Skip post-trim NEEDED audit (check-portable-ldd.sh)
  PAXP2T_ARCHIVE_GZIP=-9 Pass-through to gzip for the release tarball (-z levels; default -9 if unset).

  -h, --help               Show this help
EOF
}

trim_portable_bundle() {
    local root platdir f base dir
    root="$1"
    if [[ "${PAXP2T_SKIP_BUNDLE_TRIM:-}" == "1" ]]; then
        echo "Skipping bundle trim (PAXP2T_SKIP_BUNDLE_TRIM=1)"
        return 0
    fi
    if [[ ! -d "$root" ]]; then
        return 0
    fi

    echo "Aggressively trimming portable bundle (${root})…"
    du -sh "${root}" 2>/dev/null || true

    # --- Qt locales (directories + stray .qm) ---------------------------------
    rm -rf \
        "${root}/usr/share/qt/translations" \
        "${root}/usr/share/qt6/translations" \
        "${root}/usr/translations" \
        "${root}/translations" \
        "${root}/usr/lib/qt/translations" \
        "${root}/usr/lib/qt6/translations"
    find "${root}" -type f -iname '*.qm' -delete || true

    # --- QML / SQL (widgets app doesn't use these) ----------------------------
    rm -rf "${root}/usr/qml" "${root}/usr/lib/qml"
    while IFS= read -r -d '' dir; do
        rm -rf "$dir"
    done < <(find "${root}" -type d \( -path '*/qt6/qml' -o -path '*/qt/qml' \) -print0 2>/dev/null || true)
    while IFS= read -r -d '' dir; do
        rm -rf "$dir"
    done < <(find "${root}" -type d -path '*/plugins/sqldrivers' -print0 2>/dev/null || true)

    # --- Platform: X11 only — keep libqxcb.so(+version), drop other QPA plugins -----
    while IFS= read -r -d '' platdir; do
        for f in "${platdir}"/*; do
            [[ ! -f "$f" && ! -L "$f" ]] && continue
            base="$(basename "$f")"
            if [[ "${base}" == libqxcb.so || "${base}" == libqxcb.so.* ]]; then
                continue
            fi
            rm -f "$f"
        done
    done < <(find "${root}" -type d -path '*/plugins/platforms' -print0 2>/dev/null || true)

    while IFS= read -r -d '' dir; do
        rm -rf "${dir:?}"
    done < <(
        find "${root}" -depth -type d \( \
            -path '*/plugins/platformthemes' \
            -o -path '*/plugins/wayland-*' \
            \) -print0 2>/dev/null || true
    )

    # Qt6 QNetwork TLS backends (~OpenSSL blobs per variant); tray app loads no HTTPS assets.
    while IFS= read -r -d '' dir; do
        rm -rf "${dir:?}"
    done < <(find "${root}" -depth -type d -path '*/plugins/tls' -print0 2>/dev/null || true)

    while IFS= read -r -d '' dir; do
        rm -rf "${dir:?}"
    done < <(find "${root}" -depth -type d -path '*/plugins/multimedia' -print0 2>/dev/null || true)

    # No QImage plugin loading needed: tray SVG loaded via QtSvg (linked). Drops codec pack.
    while IFS= read -r -d '' dir; do
        rm -rf "${dir:?}"
    done < <(find "${root}" -depth -type d -path '*/plugins/imageformats' -print0 2>/dev/null || true)

    # --- Unused Qt modules often bundled but not used by this app ---------------
    find "${root}" -type f \( \
        -name 'libQt6Quick*.so*' -o \
        -name 'libQt6Qml*.so*' -o \
        -name 'libQt6Quick3D*.so*' -o \
        -name 'libQt6ShaderTools*.so*' -o \
        -name 'libQt6Vulkan*.so*' -o \
        -name 'libQt63D*.so*' -o \
        -name 'libQt6Labs*.so*' -o \
        -name 'libQt6Charts*.so*' -o \
        -name 'libQt6DataVisualization*.so*' -o \
        -name 'libQt6Multimedia*.so*' -o \
        -name 'libQt6SpatialAudio*.so*' -o \
        -name 'libQt6Positioning*.so*' -o \
        -name 'libQt6WebChannel*.so*' -o \
        -name 'libQt6WebSockets*.so*' -o \
        -name 'libQt6Bluetooth*.so*' -o \
        -name 'libQt6Nfc*.so*' -o \
        -name 'libQt6SerialPort*.so*' -o \
        -name 'libQt6Scxml*.so*' \
        \) -delete 2>/dev/null || true

    # CMake / dev cruft accidentally copied next to libs
    find "${root}" -type f -name '*.a' -delete 2>/dev/null || true
    # Strip detached debug payloads some distros stash next to libs
    find "${root}" -type f \( -name '*.debug' -o -name '*.dwz' \) -delete || true

    rm -rf "${root}/usr/share/doc" "${root}/usr/share/man"

    # SVG icon engine (not used — tray uses QtSvg). IBus/virtual-keyboard IME not used. GLX/EGL integrations optional.
    rm -rf "${root}/usr/plugins/iconengines" "${root}/usr/plugins/xcbglintegrations"
    shopt -s nullglob
    rm -f "${root}/usr/plugins/platforminputcontexts"/libibus* \
          "${root}/usr/plugins/platforminputcontexts"/libqtvirtualkeyboard*
    shopt -u nullglob

    # Drop orphaned .so blobs linuxdeploy duplicated (JPEG/AVIF/OpenSSL tails, KDE Frameworks, VirtualKeyboard libs…)
    SCRIPTS_HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    bash "${SCRIPTS_HERE}/prune-appdir-libs.sh" "${root}"

    find "${root}/usr/share/icons" -depth -type d -empty -delete 2>/dev/null || true
    find "${root}" -depth -type d -empty -delete 2>/dev/null || true

    echo "Trimmed bundle size:"
    du -sh "${root}" 2>/dev/null || true
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

trim_portable_bundle "${REPO_ROOT}/${APPDIR_NAME}"

if [[ "${PAXP2T_SKIP_LDD_CHECK:-}" != "1" ]]; then
    echo "Checking portable NEEDED dependencies (check-portable-ldd.sh)..."
    bash "${SCRIPT_DIR}/check-portable-ldd.sh" --fail-orphans "${REPO_ROOT}/${APPDIR_NAME}"
fi

PORTABLE_TOP="paxp2t-${RELEASE_VERSION}-linux-x86_64-portable"
ARCHIVE_BASENAME="paxp2t-${RELEASE_VERSION}-linux-x86_64-portable.tar.gz"
ARCHIVE_PATH="${OUT_DIR}/${ARCHIVE_BASENAME}"

rm -rf "${REPO_ROOT:?}/${PORTABLE_TOP}"
mv "${REPO_ROOT}/${APPDIR_NAME}" "${REPO_ROOT}/${PORTABLE_TOP}"

mkdir -p "${OUT_DIR}"
# GNU tar honours GZIP flags for deflate; default mirrors gzip -9 (smaller tarball, same unpacked tree).
GZIP="${PAXP2T_ARCHIVE_GZIP:--9}" tar -czvf "${ARCHIVE_PATH}" -C "${REPO_ROOT}" "${PORTABLE_TOP}"
rm -rf "${REPO_ROOT:?}/${PORTABLE_TOP}"

echo "Done:"
echo "  ${ARCHIVE_PATH}"
