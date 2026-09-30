"""Evdev EV_KEY code → Linux UAPI name (from vendored input-event-codes.h tables)."""
from __future__ import annotations

from evdev_code_tables import BTN_CODE_TO_NAME, KEY_CODE_TO_NAME


def evdev_code_name(code: int, code_to_name: dict[int, str], fallback_prefix: str) -> str:
    if code in code_to_name:
        return code_to_name[code]
    return f"{fallback_prefix}{code}"


__all__ = [
    "BTN_CODE_TO_NAME",
    "KEY_CODE_TO_NAME",
    "evdev_code_name",
]
