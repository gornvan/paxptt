import os
import threading

os.environ.setdefault("PYSTRAY_BACKEND", "appindicator")

from PIL import Image, ImageDraw
import pystray


_ICON_SIZE = 64
_COLOR_ACTIVE = "#4CAF50"
_COLOR_INACTIVE = "#888888"


def _make_circle(color):
    img = Image.new("RGBA", (_ICON_SIZE, _ICON_SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    margin = 4
    draw.ellipse(
        [margin, margin, _ICON_SIZE - margin, _ICON_SIZE - margin],
        fill=color,
    )
    return img


class TrayIcon:
    """System tray icon with a colored circle indicating mute state."""

    def __init__(self):
        self._icon_active = _make_circle(_COLOR_ACTIVE)
        self._icon_inactive = _make_circle(_COLOR_INACTIVE)
        self._icon = None
        self._thread = None

    def show_icon(self, on_open_config=None, on_terminate=None):
        """Show the tray icon. Starts muted (gray).

        Args:
            on_open_config: Optional callable invoked by "Open config".
            on_terminate: Optional callable invoked by "Terminate".
        """
        def _handle_open_config(icon, item):
            if on_open_config:
                on_open_config()

        def _handle_terminate(icon, item):
            if on_terminate:
                on_terminate()

        menu = pystray.Menu(
            pystray.MenuItem("Open config", _handle_open_config, default=True),
            pystray.MenuItem("Terminate", _handle_terminate),
        )

        self._icon = pystray.Icon(
            name="paxp2t",
            icon=self._icon_inactive,
            title="PaX Push-to-Talk",
            menu=menu,
        )

        self._thread = threading.Thread(target=self._icon.run, daemon=True)
        self._thread.start()

    def hide_icon(self):
        """Remove the tray icon and stop its thread."""
        if self._icon:
            self._icon.stop()
            self._icon = None
            self._thread = None

    def set_icon_state(self, active):
        """Switch icon color: green when active (unmuted), gray when inactive."""
        if self._icon:
            self._icon.icon = (
                self._icon_active if active else self._icon_inactive
            )
