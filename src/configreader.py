from pathlib import Path
from dataclasses import dataclass, asdict

import yaml

CONFIG_DIR = Path.home() / ".local" / "paxp2t"
CONFIG_PATH = CONFIG_DIR / "config.yml"

@dataclass
class Config:
    BIND_MOUSE_BUTTON: int = 9
    BIND_KEYBOARD_KEY: int = 66
    SHOW_TRAY_ICON: bool = True
    MUTE_DELAY_MS: int = 0


def read_config():
    """Load config from ~/.local/paxp2t/config.yml.

    Creates the file with defaults if it doesn't exist yet.
    Missing keys are filled in from DEFAULTS.
    """
    if not CONFIG_PATH.exists():
        config = Config()
        _write_config(config)
        return config

    with open(CONFIG_PATH) as f:
        raw = yaml.safe_load(f) or {}

    defaults = asdict(Config())
    parsed = {}
    for key, value in defaults.items():
        parsed[key] = raw.get(key, value)

    config = Config(
        BIND_MOUSE_BUTTON=int(parsed["BIND_MOUSE_BUTTON"]),
        BIND_KEYBOARD_KEY=int(parsed["BIND_KEYBOARD_KEY"]),
        SHOW_TRAY_ICON=bool(parsed["SHOW_TRAY_ICON"]),
        MUTE_DELAY_MS=max(0, int(parsed["MUTE_DELAY_MS"])),
    )
    return config


def _write_config(config: Config):
    CONFIG_DIR.mkdir(parents=True, exist_ok=True)
    with open(CONFIG_PATH, "w") as f:
        yaml.dump(asdict(config), f, default_flow_style=False)
