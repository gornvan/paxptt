import subprocess


class PulseMute:
    """Mute/unmute all PulseAudio recording (source) devices."""

    @staticmethod
    def _get_sources():
        """Return names of all non-monitor PulseAudio sources."""
        result = subprocess.run(
            ["pactl", "list", "sources", "short"],
            capture_output=True, text=True, check=True,
        )
        sources = []
        for line in result.stdout.strip().splitlines():
            cols = line.split("\t")
            if len(cols) >= 2 and ".monitor" not in cols[1]:
                sources.append(cols[1])
        return sources

    @staticmethod
    def mute():
        """Mute every recording source."""
        for source in PulseMute._get_sources():
            subprocess.run(
                ["pactl", "set-source-mute", source, "1"],
                check=True,
            )

    @staticmethod
    def unmute():
        """Unmute every recording source."""
        for source in PulseMute._get_sources():
            subprocess.run(
                ["pactl", "set-source-mute", source, "0"],
                check=True,
            )
