import subprocess
import time
import threading

from configmngr import read_config, CONFIG_PATH
from bind_mouse import MouseBinder
from pulse_mute import PulseMute
from tray_icon import TrayIcon
from sound import ensure_sounds, play_sound, UNMUTE_SOUND_PATH, MUTE_SOUND_PATH
from bind_kbd import KeyboardBinder


def _open_config_file():
    # Prefer portal-aware openers, then host opener if running in Flatpak.
    openers = (
        ["xdg-open", str(CONFIG_PATH)],
        ["gio", "open", str(CONFIG_PATH)],
        ["flatpak-spawn", "--host", "xdg-open", str(CONFIG_PATH)],
    )
    for command in openers:
        try:
            subprocess.Popen(command)
            return
        except FileNotFoundError:
            continue
        except Exception as exc:
            print(f"Config open failed with {command[0]}: {exc}")


def main():
    config = read_config()
    bound_mousebtn = config.BIND_MOUSE_BUTTON
    bound_kbkey = config.BIND_KEYBOARD_KEY
    mute_delay_s = config.MUTE_DELAY_MS / 1000.0
    ensure_sounds()
    tray = TrayIcon()
    terminate_requested = threading.Event()
    state_lock = threading.Lock()
    ptt_is_down = False

    def apply_mute():
        PulseMute.mute()
        tray.set_icon_state(False)
        play_sound(MUTE_SOUND_PATH)

    def schedule_mute():
        def delayed_mute():
            with state_lock:
                if ptt_is_down:
                    return
            apply_mute()

        timer = threading.Timer(mute_delay_s, delayed_mute)
        timer.daemon = True
        timer.start()

    def on_press(_btn):
        nonlocal ptt_is_down
        with state_lock:
            should_unmute = not ptt_is_down
            ptt_is_down = True

        if should_unmute:
            play_sound(UNMUTE_SOUND_PATH)
            PulseMute.unmute()
            tray.set_icon_state(True)

    def on_release(_btn):
        nonlocal ptt_is_down
        with state_lock:
            ptt_is_down = False
        schedule_mute()

    PulseMute.mute()
    tray.set_icon_state(False)
    print(f"All mics muted. Hold mouse button {bound_mousebtn} to talk. Enjoy your privacy.")

    if config.SHOW_TRAY_ICON:
        tray.show_icon(
            on_open_config=_open_config_file,
            on_terminate=terminate_requested.set,
        )

    with MouseBinder() as mouse_binder:
        if bound_mousebtn:
            mouse_binder.bind(
                button=bound_mousebtn,
                on_press=on_press,
                on_release=on_release,
            )

        with KeyboardBinder() as keyboard_binder:
            if bound_kbkey:
                keyboard_binder.bind(
                    key=bound_kbkey,
                    on_press=on_press,
                    on_release=on_release,
                )

            try:
                while not terminate_requested.is_set():
                    time.sleep(0.5)
            except KeyboardInterrupt:
                pass

    tray.hide_icon()
    PulseMute.unmute()
    print("Mics restored. No more privacy. Have a nice day.")


if __name__ == "__main__":
    main()
