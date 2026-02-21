from pathlib import Path
import wave
import struct
from pathlib import Path
import math
import subprocess

SOUND_DIR = Path.home() / ".local" / "paxp2t" / "sounds"
UNMUTE_SOUND_PATH = SOUND_DIR / "unmute.wav"
MUTE_SOUND_PATH = SOUND_DIR / "mute.wav"
SAMPLE_RATE = 44100

# Generate a sine wave beep and save as WAV.
def generate_beep(path: Path, frequency: float, duration: float = 0.06, volume: float = 0.04):
    n_samples = int(SAMPLE_RATE * duration)

    with wave.open(str(path), "w") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)  # 16-bit
        wav.setframerate(SAMPLE_RATE)

        for i in range(n_samples):
            t = i / SAMPLE_RATE
            sample = volume * math.sin(2 * math.pi * frequency * t)
            wav.writeframes(struct.pack("<h", int(sample * 32767)))

# Create sounds if they don't exist.
def ensure_sounds():
    SOUND_DIR.mkdir(parents=True, exist_ok=True)

    if not UNMUTE_SOUND_PATH.exists():
        generate_beep(UNMUTE_SOUND_PATH, frequency=100)

    if not MUTE_SOUND_PATH.exists():
        generate_beep(MUTE_SOUND_PATH, frequency=50)


def play_sound(path):
    subprocess.Popen(["paplay", str(path)],
                     stdout=subprocess.DEVNULL,
                     stderr=subprocess.DEVNULL)