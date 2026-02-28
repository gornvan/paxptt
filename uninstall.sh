#!/usr/bin/env bash
set -euo pipefail

SERVICE_NAME="paxp2t"
SERVICE_DIR="$HOME/.config/systemd/user"
SERVICE_FILE="$SERVICE_DIR/$SERVICE_NAME.service"

if ! command -v systemctl >/dev/null 2>&1; then
    echo "Error: required command not found: systemctl" >&2
    exit 1
fi

if ! systemctl --user show-environment >/dev/null 2>&1; then
    echo "Error: user systemd manager is not available." >&2
    exit 1
fi

if systemctl --user list-unit-files --type=service | grep -q "^${SERVICE_NAME}\.service"; then
    systemctl --user disable --now "$SERVICE_NAME" || true
fi

if [[ -f "$SERVICE_FILE" ]]; then
    rm -f "$SERVICE_FILE"
fi

systemctl --user daemon-reload

if systemctl --user list-unit-files --type=service | grep -q "^${SERVICE_NAME}\.service"; then
    echo "Warning: service unit still appears in unit-file list: $SERVICE_NAME.service" >&2
else
    echo "Service uninstalled."
fi

echo "  Verify: systemctl --user status $SERVICE_NAME"
