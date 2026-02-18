from mouse_bind import MouseBinder
from pulse_mute import PulseMute


def main():
    PulseMute.mute()
    print("All mics muted. Hold mouse button 9 to talk.")

    with MouseBinder() as binder:
        binder.bind(
            button=9,
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
