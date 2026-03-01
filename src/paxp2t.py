import subprocess

from configreader import read_config, CONFIG_PATH
from bind_mouse import MouseBinder
from pulse_mute import PulseMute
from tray_icon import TrayIcon
from sound import ensure_sounds, play_sound, UNMUTE_SOUND_PATH, MUTE_SOUND_PATH
from bind_kbd import KeyboardBinder

def main():
    config = read_config()
    bound_mousebtn = config["BIND_MOUSE_BUTTON"]
    bound_kbkey = config["BIND_KEYBOARD_KEY"]
    ensure_sounds()
    tray = TrayIcon()

    def on_press(_btn):
        print("on_press: ", _btn)
        play_sound(UNMUTE_SOUND_PATH)
        PulseMute.unmute()
        tray.set_icon_state(True)

    def on_release(_btn):
        play_sound(MUTE_SOUND_PATH)
        PulseMute.mute()
        tray.set_icon_state(False)

    PulseMute.mute()
    tray.set_icon_state(False)
    print(f"All mics muted. Hold mouse button {bound_mousebtn} to talk. Enjoy your privacy.")

    if config["SHOW_TRAY_ICON"]:
        tray.show_icon(
            click_handler=lambda: subprocess.Popen(["xdg-open", str(CONFIG_PATH)])
        )
    
  
    with MouseBinder() as mouse_binder:
        keyboard_binder = KeyboardBinder()

        if bound_mousebtn:
            mouse_binder.bind(
                button=bound_mousebtn,
                on_press=on_press,
                on_release=on_release,
            )

        with KeyboardBinder() as kb:
            if bound_kbkey:
                keyboard_binder.bind(
                    key=bound_kbkey,
                    on_press=on_press,
                    on_release=on_release,
                )

            try:
                while True:
                    time.sleep(0.5)
            except KeyboardInterrupt:
                pass

    keyboard_binder.stop()

    tray.hide_icon()
    PulseMute.unmute()
    print("Mics restored. No more privacy. Have a nice day.")


if __name__ == "__main__":
    main()
