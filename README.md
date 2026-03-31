# paxp2t

`paxp2t` is a push-to-talk utility allowing to set system-wide unmute and mute trigger,
by either a mouse or a keyboard button.

## Features

- X11 global mouse/keyboard bindings via XRecord
- PulseAudio total source mute/unmute via `pactl`
- Sound indication of unmute/mute actions via `paplay`
  - sounds stored as `.wav` under `~/.local/paxp2t/sounds/`, easy to replace
- Tray icon via `QSystemTrayIcon` with:
  - dynamic gray/green circle icon
  - `Open config`
  - `Terminate`
- Typed config read/backfill/rewrite at:
  - `~/.local/paxp2t/config.yml`


## Considerations
- Cannot mute sources selectively - do not expect _any_ of pulseaudio inputs to stay unmuted while paxp2t is active;
- Adaptive noise/echo cancellation codecs might get a bit mad with such mic behavior - in _some_ cases best disable them;
- Caches sources at startup. After connecting a new mic or otherwise adding an input, please restart paxp2t. If You do that a lot, consider setting `CACHE_INPUTS` to `false`;

## Build

```bash
cmake -S cpp -B build-cpp
cmake --build build-cpp -j
```

Run:

```bash
./build-cpp/paxp2t-cpp
```

## Notes

- Won't work on Wayland-based systems or any other system with no X11-served displays.
- On desktops without tray host support, the app keeps working without tray. If pulseaudio is available, the soudnd will indication will still work.
