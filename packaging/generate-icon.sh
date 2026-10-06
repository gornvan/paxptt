#!/usr/bin/env bash
# Generate packaging/p2td.png for desktop/RPM (gitignored). Requires ImageMagick convert.
set -euo pipefail
out="${1:?usage: generate-icon.sh OUTPUT.png}"
if ! command -v convert >/dev/null 2>&1; then
  echo "generate-icon.sh: ImageMagick convert not found" >&2
  exit 1
fi
mkdir -p "$(dirname "$out")"
convert -size 128x128 xc:none -fill '#44BB44' -draw 'circle 64,64 64,10' "$out"
