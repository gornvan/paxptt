#!/usr/bin/env python3
"""
Compare evdev device discovery with what p2td uses (by-id + /proc merge).

Run from repo root:
  .github/scripts/evdev-diagnose-device-paths.py

Checks which paths exist, which open(O_RDONLY), and whether by-id vs /proc differ.
"""
from __future__ import annotations

import errno
import glob
import os
from pathlib import Path


def glob_by_id(suffix: str) -> list[str]:
    return sorted(glob.glob(f"/dev/input/by-id/*{suffix}"))


def discover_from_proc(want_mouse: bool, want_kbd: bool) -> list[str]:
    text = Path("/proc/bus/input/devices").read_text(encoding="utf-8", errors="replace")
    out: list[str] = []
    for block in text.split("\n\n"):
        lower = block.lower()
        is_mouse = "mouse" in lower
        is_kbd = "keyboard" in lower
        if (want_mouse and is_mouse) or (want_kbd and is_kbd):
            for line in block.splitlines():
                if line.startswith("H: Handlers="):
                    for h in line.split("=", 1)[1].split():
                        if h.startswith("event"):
                            out.append(f"/dev/input/{h}")
    return sorted(set(out))


def discover_paths() -> list[str]:
    paths = set(glob_by_id("-event-mouse"))
    paths.update(glob_by_id("-event-kbd"))
    paths.update(discover_from_proc(True, False))
    paths.update(discover_from_proc(False, True))
    return sorted(paths)


def try_open(path: str) -> str:
    try:
        fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    except OSError as e:
        return f"FAIL ({e})"
    os.close(fd)
    return "ok"


def main() -> int:
    by_mouse = glob_by_id("-event-mouse")
    by_kbd = glob_by_id("-event-kbd")
    proc_mouse = discover_from_proc(True, False)
    proc_kbd = discover_from_proc(False, True)
    merged = discover_paths()

    print("=== by-id ===")
    print(f"  *-event-mouse: {len(by_mouse)}")
    for p in by_mouse:
        print(f"    {p}  open: {try_open(p)}")
    print(f"  *-event-kbd:   {len(by_kbd)}")
    for p in by_kbd:
        print(f"    {p}  open: {try_open(p)}")

    print("\n=== /proc (extra vs by-id) ===")
    extra_mouse = sorted(set(proc_mouse) - set(by_mouse))
    extra_kbd = sorted(set(proc_kbd) - set(by_kbd))
    print(f"  mouse event nodes only in /proc: {len(extra_mouse)}")
    for p in extra_mouse:
        print(f"    {p}  open: {try_open(p)}")
    print(f"  kbd event nodes only in /proc: {len(extra_kbd)}")
    for p in extra_kbd:
        print(f"    {p}  open: {try_open(p)}")

    print(f"\n=== merged (p2td discoverEvdevInputDevicePaths): {len(merged)} paths ===")
    mice_like = [p for p in merged if "mouse" in p or p in proc_mouse]
    print(f"  paths that look mouse-related: {len(mice_like)}")
    for p in merged:
        tag = ""
        if p in by_mouse:
            tag = " [by-id mouse]"
        elif p in by_kbd:
            tag = " [by-id kbd]"
        elif p in proc_mouse:
            tag = " [proc mouse]"
        elif p in proc_kbd:
            tag = " [proc kbd]"
        print(f"    {p}  open: {try_open(p)}{tag}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
