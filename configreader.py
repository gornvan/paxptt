from pathlib import Path

import yaml

CONFIG_DIR = Path.home() / ".local" / "paxp2t"
CONFIG_PATH = CONFIG_DIR / "config.yml"

DEFAULTS = {
    "BIND_MOUSE_BUTTON": 9,
    "BIND_KEYBOARD_KEY": 66,
    "SHOW_TRAY_ICON": True,
}


def read_config():
    """Load config from ~/.local/paxp2t/config.yml.

    Creates the file with defaults if it doesn't exist yet.
    Missing keys are filled in from DEFAULTS.
    """
    if not CONFIG_PATH.exists():
        _write_defaults()
        return dict(DEFAULTS)

    with open(CONFIG_PATH) as f:
        config = yaml.safe_load(f) or {}

    for key, value in DEFAULTS.items():
        config.setdefault(key, value)

    return config


def _write_defaults():
    CONFIG_DIR.mkdir(parents=True, exist_ok=True)
    with open(CONFIG_PATH, "w") as f:
        yaml.dump(DEFAULTS, f, default_flow_style=False)
