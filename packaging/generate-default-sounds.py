#!/usr/bin/env python3
"""Generate default mute/unmute indicator WAVs (PCM16 mono 44.1 kHz).

Same parameters as SoundController::generateBeepWav in cpp/src/sound_controller.cpp.
Invoked at package/build install time; outputs are not stored in git.
"""

from __future__ import annotations

import argparse
import math
import struct
import wave
from pathlib import Path


def write_beep_wav(path: Path, frequency_hz: float, duration_s: float = 0.06, volume: float = 0.04) -> None:
    sample_rate = 44100
    sample_count = int(sample_rate * duration_s)
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sample_rate)
        frames = bytearray()
        for i in range(sample_count):
            t = i / sample_rate
            sample = int(volume * 32767.0 * math.sin(2.0 * math.pi * frequency_hz * t))
            frames += struct.pack("<h", sample)
        wf.writeframes(frames)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "out_dir",
        type=Path,
        help="Directory for mute.wav and unmute.wav",
    )
    args = parser.parse_args()
    out = args.out_dir
    write_beep_wav(out / "unmute.wav", 100.0)
    write_beep_wav(out / "mute.wav", 50.0)


if __name__ == "__main__":
    main()
