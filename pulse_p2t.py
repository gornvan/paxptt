import subprocess

from configreader import read_config, CONFIG_PATH
from mouse_bind import MouseBinder
from pulse_mute import PulseMute
from tray_icon import TrayIcon


def main():
    config = read_config()
    button = config["BIND_MOUSE_BUTTON"]

    tray = TrayIcon()

    def on_press(_btn):
        PulseMute.unmute()
        tray.set_icon_state(True)

    def on_release(_btn):
        PulseMute.mute()
        tray.set_icon_state(False)

    PulseMute.mute()
    tray.set_icon_state(False)
    print(f"All mics muted. Hold mouse button {button} to talk. Enjoy your privacy.")

    if config["SHOW_TRAY_ICON"]:
        tray.show_icon(
            click_handler=lambda: subprocess.Popen(["xdg-open", str(CONFIG_PATH)])
        )

    with MouseBinder() as binder:
        binder.bind(button=button, on_press=on_press, on_release=on_release)

        try:
            while True:
                pass
        except KeyboardInterrupt:
            pass

    tray.hide_icon()
    PulseMute.unmute()
    print("Mics restored. No more privacy. Have a nice day.")


if __name__ == "__main__":
    main()
