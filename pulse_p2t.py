from configreader import read_config
from mouse_bind import MouseBinder
from pulse_mute import PulseMute


def main():
    config = read_config()
    button = config["BIND_MOUSE_BUTTON"]

    PulseMute.mute()
    print(f"All mics muted. Hold mouse button {button} to talk.")

    with MouseBinder() as binder:
        binder.bind(
            button=button,
            on_press=lambda _: PulseMute.unmute(),
            on_release=lambda _: PulseMute.mute(),
        )

        try:
            while True:
                pass
        except KeyboardInterrupt:
            pass

    PulseMute.unmute()
    print("Mics restored. Bye.")


if __name__ == "__main__":
    main()
