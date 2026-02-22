from bind_mouse import MouseBinder


def main():
    binder = MouseBinder()

    binder.bind(
        button=9,
        on_press=lambda btn: print(f"Button {btn} pressed"),
        on_release=lambda btn: print(f"Button {btn} released"),
    )

    binder.bind(
        button=1,
        on_press=lambda btn: print(f"Left click pressed"),
        on_release=lambda btn: print(f"Left click released"),
    )

    print("Listening... (Ctrl+C to stop)")
    try:
        while True:
            pass
    except KeyboardInterrupt:
        pass
    finally:
        binder.stop()


if __name__ == "__main__":
    main()
