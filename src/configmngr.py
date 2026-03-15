from pathlib import Path
from dataclasses import dataclass, asdict
from typing import Any

import yaml

CONFIG_DIR = Path.home() / ".local" / "paxp2t"
CONFIG_PATH = CONFIG_DIR / "config.yml"

@dataclass
class Config:
    BIND_MOUSE_BUTTON: int = 9
    BIND_KEYBOARD_KEY: int = 66
    SHOW_TRAY_ICON: bool = True
    MUTE_DELAY_MS: int = 0

    def __post_init__(self):
        self.BIND_MOUSE_BUTTON = int(self.BIND_MOUSE_BUTTON)
        self.BIND_KEYBOARD_KEY = int(self.BIND_KEYBOARD_KEY)
        self.SHOW_TRAY_ICON = _to_bool(self.SHOW_TRAY_ICON)
        self.MUTE_DELAY_MS = max(0, int(self.MUTE_DELAY_MS))


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
        loaded = yaml.safe_load(f) or {}
    raw = loaded if isinstance(loaded, dict) else {}

    defaults = asdict(Config())
    parsed = {}
    needs_rewrite = not isinstance(loaded, dict)
    for key, default_value in defaults.items():
        if key not in raw:
            needs_rewrite = True
            parsed[key] = default_value
            continue

        try:
            parsed[key] = _coerce_value(raw[key], default_value)
        except (TypeError, ValueError):
            needs_rewrite = True
            parsed[key] = default_value
            continue

        if parsed[key] != raw[key]:
            needs_rewrite = True

    config = Config(**parsed)
    if asdict(config) != parsed:
        needs_rewrite = True

    if needs_rewrite:
        extra_values = {k: v for k, v in raw.items() if k not in defaults}
        _write_config(config, extra_values)

    return config


def _coerce_value(value: Any, default_value: Any):
    if isinstance(default_value, bool):
        return _to_bool(value)
    if isinstance(default_value, int):
        return int(value)
    return value


def _to_bool(value: Any) -> bool:
    if isinstance(value, bool):
        return value
    if isinstance(value, str):
        normalized = value.strip().lower()
        if normalized in {"1", "true", "yes", "on"}:
            return True
        if normalized in {"0", "false", "no", "off"}:
            return False
    raise ValueError(f"Invalid boolean value: {value!r}")


def _write_config(config: Config, extra_values: dict | None = None):
    CONFIG_DIR.mkdir(parents=True, exist_ok=True)
    payload = dict(extra_values or {})
    payload.update(asdict(config))
    with open(CONFIG_PATH, "w") as f:
        yaml.dump(payload, f, default_flow_style=False)
