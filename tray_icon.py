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

    def show_icon(self, click_handler=None):
        """Show the tray icon. Starts muted (gray).

        Args:
            click_handler: Optional callable invoked on left-click.
        """
        def on_activate(icon, item):
            if click_handler:
                click_handler()

        menu = pystray.Menu(
            pystray.MenuItem("Open config", on_activate, default=True),
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
