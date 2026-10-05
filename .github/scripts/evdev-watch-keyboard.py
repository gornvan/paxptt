#!/usr/bin/env python3
"""
Watch EV_KEY events from keyboard evdev nodes.

Use this to find KEY_* names for BIND_PTT in ~/.local/p2td/config.yml
(not X11 keysym names). Names come from vendored linux/input-event-codes.h tables.

Requires read access to the devices (root or membership in group "input").

Usage:
  .github/scripts/evdev-watch-keyboard.py
  .github/scripts/evdev-watch-keyboard.py --devices /dev/input/event3
  .github/scripts/evdev-watch-keyboard.py --watch 58,125
"""
from __future__ import annotations

import argparse
import errno
import glob
import os
import select
import struct
import sys
from pathlib import Path

from evdev_linux_codes import KEY_CODE_TO_NAME, evdev_code_name

EV_KEY = 0x01
BTN_MISC = 0x100
INPUT_EVENT_FMT = "llHHI"
INPUT_EVENT_SIZE = struct.calcsize(INPUT_EVENT_FMT)


def default_keyboard_event_paths() -> list[str]:
    paths = sorted(glob.glob("/dev/input/by-id/*-event-kbd"))
    if paths:
        return paths
    return discover_keyboards_from_proc()


def discover_keyboards_from_proc() -> list[str]:
    text = Path("/proc/bus/input/devices").read_text(encoding="utf-8", errors="replace")
    out: list[str] = []
    for block in text.strip().split("\n\n"):
        if "keyboard" not in block.lower():
            continue
        for line in block.splitlines():
            if line.startswith("H: Handlers="):
                handlers = line.split("=", 1)[1].split()
                for h in handlers:
                    if h.startswith("event"):
                        out.append(f"/dev/input/{h}")
                break
    return sorted(set(out))


def resolve_paths(devices_arg: str | None) -> list[str]:
    if devices_arg:
        return [p.strip() for p in devices_arg.split(",") if p.strip()]
    return default_keyboard_event_paths()


def open_devices(paths: list[str]) -> dict[int, str]:
    fds: dict[int, str] = {}
    for path in paths:
        try:
            fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
        except OSError as e:
            print(f"skip {path}: {e}", file=sys.stderr)
            continue
        fds[fd] = path
    return fds


def main() -> int:
    parser = argparse.ArgumentParser(description="Watch key events on keyboard evdev nodes.")
    parser.add_argument(
        "--devices",
        help="Comma-separated event paths (default: all /dev/input/by-id/*-event-kbd)",
    )
    parser.add_argument(
        "--watch",
        help="Comma-separated EV_KEY codes to highlight (e.g. 58,125 for CAPSLOCK, LEFTMETA)",
    )
    args = parser.parse_args()

    watch: set[int] = set()
    if args.watch:
        for part in args.watch.split(","):
            part = part.strip()
            if part:
                watch.add(int(part, 0))

    paths = resolve_paths(args.devices)
    if not paths:
        print("No keyboard evdev paths found.", file=sys.stderr)
        print("Try: ls -l /dev/input/by-id/*event-kbd*", file=sys.stderr)
        print("Or:  sudo evtest", file=sys.stderr)
        return 1

    print("Opening keyboard evdev nodes:")
    for p in paths:
        print(f"  {p}")
    print()
    print("Press keys on the keyboard(s) you care about. Use the KEY_* name in BIND_PTT.")
    print("Press Ctrl+C to stop.\n")

    fds = open_devices(paths)
    if not fds:
        print("Could not open any device (need root or 'input' group?).", file=sys.stderr)
        return 1

    poll = select.poll()
    for fd in fds:
        poll.register(fd, select.POLLIN)

    try:
        while True:
            for fd, _ in poll.poll():
                while True:
                    try:
                        buf = os.read(fd, INPUT_EVENT_SIZE)
                    except OSError as e:
                        if e.errno in (errno.EAGAIN, errno.EWOULDBLOCK):
                            break
                        raise
                    if len(buf) < INPUT_EVENT_SIZE:
                        break
                    _sec, _usec, ev_type, code, value = struct.unpack(INPUT_EVENT_FMT, buf)
                    if ev_type != EV_KEY:
                        continue
                    if code >= BTN_MISC:
                        continue
                    name = evdev_code_name(code, KEY_CODE_TO_NAME, "KEY_")
                    action = {0: "release", 1: "press", 2: "repeat"}.get(value, str(value))
                    mark = " *** PTT watch" if code in watch else ""
                    print(f"{fds[fd]}: {name} (code={code}) {action}{mark}  → BIND_PTT: [{name}]")
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        for fd in fds:
            os.close(fd)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
