#!/usr/bin/env bash
# Create Source0 tarball from git and run rpmbuild -ba (openSUSE).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
VERSION="$(sed -n 's/^project(p2td VERSION \([0-9.]*\).*/\1/p' "$ROOT/cpp/CMakeLists.txt")"
if [[ -z "$VERSION" ]]; then
  echo "Could not read version from cpp/CMakeLists.txt" >&2
  exit 1
fi
NAME="p2td-${VERSION}"
TARBALL="${NAME}.tar.gz"
SOURCES="${HOME}/rpmbuild/SOURCES"
mkdir -p "$SOURCES"
git -C "$ROOT" archive --format=tar.gz --prefix="${NAME}/" -o "${SOURCES}/${TARBALL}" HEAD
rpmbuild -ba "$ROOT/packaging/opensuse/p2td.spec"
