#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PYTHON="$SCRIPT_DIR/venv/bin/python"
ENTRY="$SCRIPT_DIR/pulse_p2t.py"
SERVICE_NAME="pulse-p2t"
SERVICE_DIR="$HOME/.config/systemd/user"
SERVICE_FILE="$SERVICE_DIR/$SERVICE_NAME.service"

if [[ ! -x "$PYTHON" ]]; then
    echo "Error: venv python not found at $PYTHON" >&2
    exit 1
fi

mkdir -p "$SERVICE_DIR"

cat > "$SERVICE_FILE" <<EOF
[Unit]
Description=Push-to-Talk via mouse button 9
After=graphical-session.target

[Service]
Type=simple
ExecStart=$PYTHON $ENTRY
Restart=on-failure
RestartSec=3
Environment=DISPLAY=:0

[Install]
WantedBy=graphical-session.target
EOF

systemctl --user daemon-reload
systemctl --user enable --now "$SERVICE_NAME"

echo "Service installed and started."
echo "  Status:  systemctl --user status $SERVICE_NAME"
echo "  Logs:    journalctl --user -u $SERVICE_NAME -f"
echo "  Stop:    systemctl --user disable --now $SERVICE_NAME"
