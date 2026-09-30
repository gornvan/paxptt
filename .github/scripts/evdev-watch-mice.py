#!/usr/bin/env python3
"""
Watch EV_KEY events from all mouse evdev nodes (multi-mouse).

Use this to see which /dev/input/by-id/...-event-mouse node fires for each
physical mouse and which BTN_* code side buttons use (not X11 button numbers).

Requires read access to the devices (root or membership in group "input").

Usage:
  .github/scripts/evdev-watch-mice.py
  .github/scripts/evdev-watch-mice.py --devices /dev/input/event5,/dev/input/event12
  .github/scripts/evdev-watch-mice.py --watch 275,277,276
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

from evdev_linux_codes import BTN_CODE_TO_NAME, evdev_code_name

EV_KEY = 0x01
INPUT_EVENT_FMT = "llHHI"
INPUT_EVENT_SIZE = struct.calcsize(INPUT_EVENT_FMT)


def default_mouse_event_paths() -> list[str]:
    paths = sorted(glob.glob("/dev/input/by-id/*-event-mouse"))
    if paths:
        return paths
    # Fallback: every event node that looks like a mouse in /proc (best-effort).
    return discover_mice_from_proc()


def discover_mice_from_proc() -> list[str]:
    text = Path("/proc/bus/input/devices").read_text(encoding="utf-8", errors="replace")
    out: list[str] = []
    for block in text.strip().split("\n\n"):
        if "mouse" not in block.lower() and "MOUSE" not in block:
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
    return default_mouse_event_paths()


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
    parser = argparse.ArgumentParser(description="Watch button events on all mouse evdev nodes.")
    parser.add_argument(
        "--devices",
        help="Comma-separated event paths (default: all /dev/input/by-id/*-event-mouse)",
    )
    parser.add_argument(
        "--watch",
        help="Comma-separated EV_KEY codes to highlight (e.g. 275,277 for BTN_SIDE, BTN_FORWARD)",
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
        print("No mouse evdev paths found.", file=sys.stderr)
        print("Try: ls -l /dev/input/by-id/*event-mouse*", file=sys.stderr)
        print("Or:  sudo evtest", file=sys.stderr)
        return 1

    print("Opening mouse evdev nodes:")
    for p in paths:
        print(f"  {p}")
    print()
    print("/dev/input/mouse0..N are legacy PS/2 streams — ignore them; use event* (this script).")
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
                    name = evdev_code_name(code, BTN_CODE_TO_NAME, "BTN_")
                    action = {0: "release", 1: "press", 2: "repeat"}.get(value, str(value))
                    mark = " *** PTT watch" if code in watch else ""
                    print(f"{fds[fd]}: {name} (code={code}) {action}{mark}")
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        for fd in fds:
            os.close(fd)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
