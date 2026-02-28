#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PYTHON="$SCRIPT_DIR/venv/bin/python"
ENTRY="$SCRIPT_DIR/src/paxp2t.py"
SERVICE_NAME="paxp2t"
SERVICE_DIR="$HOME/.config/systemd/user"
SERVICE_FILE="$SERVICE_DIR/$SERVICE_NAME.service"

require_cmd() {
    local cmd="$1"
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "Error: required command not found: $cmd" >&2
        exit 1
    fi
}

systemd_quote() {
    local s="$1"
    s="${s//\\/\\\\}"
    s="${s//\"/\\\"}"
    printf '"%s"' "$s"
}

echo "Running preflight checks..."
require_cmd bash
require_cmd systemctl

if ! systemctl --user show-environment >/dev/null 2>&1; then
    echo "Error: user systemd manager is not available." >&2
    echo "Hint: log in via a graphical session with systemd --user enabled." >&2
    exit 1
fi

if [[ "${XDG_SESSION_TYPE:-}" != "x11" ]]; then
    echo "Error: this installer supports X11 only. Detected XDG_SESSION_TYPE='${XDG_SESSION_TYPE:-unset}'." >&2
    exit 1
fi

if [[ -z "${DISPLAY:-}" ]]; then
    echo "Error: DISPLAY is not set. Cannot connect to X11." >&2
    exit 1
fi

if [[ ! -x "$PYTHON" ]]; then
    echo "Error: expected Python runtime not found: $PYTHON" >&2
    echo "Hint: prepare the bundled runtime before running install.sh." >&2
    exit 1
fi

if [[ ! -f "$ENTRY" ]]; then
    echo "Error: entry script not found: $ENTRY" >&2
    exit 1
fi

if ! "$PYTHON" -c "from Xlib import display; display.Display().close()" >/dev/null 2>&1; then
    echo "Error: Python runtime cannot connect to X11 or is missing Xlib dependency." >&2
    exit 1
fi

if [[ -n "${XAUTHORITY:-}" ]] && [[ ! -r "${XAUTHORITY}" ]]; then
    echo "Error: XAUTHORITY is set but not readable: ${XAUTHORITY}" >&2
    exit 1
fi

mkdir -p "$SERVICE_DIR"

if [[ -f "$SERVICE_FILE" ]]; then
    echo "Updating existing user service at $SERVICE_FILE"
else
    echo "Creating user service at $SERVICE_FILE"
fi

PYTHON_ESCAPED="$(systemd_quote "$PYTHON")"
ENTRY_ESCAPED="$(systemd_quote "$ENTRY")"
DISPLAY_ESCAPED="$(systemd_quote "${DISPLAY}")"

if [[ -n "${XAUTHORITY:-}" ]]; then
    XAUTHORITY_ESCAPED="$(systemd_quote "${XAUTHORITY}")"
    XAUTH_LINE="Environment=XAUTHORITY=${XAUTHORITY_ESCAPED}"
else
    XAUTH_LINE=""
fi

cat > "$SERVICE_FILE" <<EOF
[Unit]
Description=Push-to-Talk via mouse button 9
After=graphical-session.target

[Service]
Type=simple
ExecStart=$PYTHON_ESCAPED $ENTRY_ESCAPED
Restart=on-failure
RestartSec=3
Environment=DISPLAY=$DISPLAY_ESCAPED
$XAUTH_LINE
NoNewPrivileges=yes
PrivateTmp=yes
ProtectSystem=full

[Install]
WantedBy=graphical-session.target
EOF

if [[ -n "${XAUTHORITY:-}" ]]; then
    systemctl --user import-environment DISPLAY XAUTHORITY
else
    systemctl --user import-environment DISPLAY
fi

systemctl --user daemon-reload
systemctl --user enable --now "$SERVICE_NAME"

if ! systemctl --user is-enabled "$SERVICE_NAME" >/dev/null 2>&1; then
    echo "Error: service failed enable check: $SERVICE_NAME" >&2
    exit 1
fi

if ! systemctl --user is-active "$SERVICE_NAME" >/dev/null 2>&1; then
    echo "Error: service is not active after installation: $SERVICE_NAME" >&2
    systemctl --user --no-pager --full status "$SERVICE_NAME" || true
    exit 1
fi

echo "Service installed, enabled, and running."
echo "  Status:  systemctl --user status $SERVICE_NAME"
echo "  Logs:    journalctl --user -u $SERVICE_NAME -f"
echo "  Stop:    systemctl --user disable --now $SERVICE_NAME"
