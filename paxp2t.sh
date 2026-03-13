#!/usr/bin/env sh
set -eu

if [ -d "/app/share/paxp2t/src" ]; then
  export PYTHONPATH="/app/share/paxp2t/src${PYTHONPATH:+:${PYTHONPATH}}"
  exec python3 -m paxp2t
fi

APP_DIR=$(dirname "$0")

if [ -x "${APP_DIR}/venv/bin/python" ]; then
  exec "${APP_DIR}/venv/bin/python" "${APP_DIR}/src/paxp2t.py"
fi

if [ -x "${APP_DIR}/.venv/bin/python" ]; then
  exec "${APP_DIR}/.venv/bin/python" "${APP_DIR}/src/paxp2t.py"
fi

echo "No virtual environment found. Expected one of:" >&2
echo "  ${APP_DIR}/venv/bin/python" >&2
echo "  ${APP_DIR}/.venv/bin/python" >&2
exit 1
