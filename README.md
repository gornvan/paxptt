# paxp2t

`paxp2t` is a push-to-talk utility allowing to set system-wide unmute and mute trigger,
by either a mouse or a keyboard button.

## Features

- X11 global mouse/keyboard bindings via XRecord
- PulseAudio total source mute/unmute via `pactl`
- Sound indication of unmute/mute actions via `paplay`
  - sounds stored as `.wav` under `~/.local/paxp2t/sounds/`, easy to replace
- Tray icon via `QSystemTrayIcon` with:
  - a tray icon, toggling its color
  - `Open config`
  - `Terminate`
- Typed config read/backfill/rewrite at:
  - `~/.local/paxp2t/config.yml`

## Configurability
### Binds
By default, the bound buttons are:
- mouse button `9` (FORWARD)
- X11 keysym `Caps_Lock`

Set `BIND_MOUSE_BUTTON` to `0` to unbind mouse PTT.
Set `BIND_KEYBOARD_KEYSYM` to `None` to unbind keyboard PTT.

Keyboard binding uses **X11 keysyms** (names like `Caps_Lock`, `F24`, `Pause`).

To discover the keysym for a key, run this in a terminal; each keypress prints the keysym name:
```bash
# xev might need to be installed
xev -event keyboard | grep --line-buffered keycode | sed -E 's/.*, ([^,^\)]*)\).*/\1/'
```
Then set `BIND_KEYBOARD_KEYSYM: <name>` in `~/.local/paxp2t/config.yml` \
(for example `BIND_KEYBOARD_KEYSYM: Caps_Lock`).

### Icons
After first run, You can replace the icons under `~/.local/paxp2t/icons/` with any svg You like.
Changing color is easy - just edit the .svg icons with a text editor and replace the color code of `fill` value in the `circle` tag.

## Considerations
- Cannot mute sources selectively - do not expect _any_ of pulseaudio inputs to stay unmuted while paxp2t is active;
- Adaptive noise/echo cancellation codecs might get a bit mad with such mic behavior - in _some_ cases best disable them;
- Caches sources at startup. After connecting a new mic or otherwise adding an input, please restart paxp2t. If You do that a lot, consider setting `CACHE_INPUTS` to `false`;

## Build

```bash
# Configure the project: source dir is 'cpp', build dir is 'build-cpp'
cmake -S cpp -B build-cpp
# Build the project in 'build-cpp'; '-j' speeds up build using all CPU cores
cmake --build build-cpp -j
```

### Run:

```bash
./build-cpp/paxp2t
```

## Releases (portable Linux)

Tagged releases (e.g. `git push origin v0.1.0`) build a **portable tree** with [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy) and the **Qt plugin**, so Qt libraries and plugins are shipped next to the binary instead of relying on system Qt.

The release job uses **`PAXP2T_RELEASE_MINIMAL=ON`** (smaller Release flags: `-Os`, section GC, `--as-needed`), **strips** the main binary, then **strips** bundled `.so` files where possible so the archive stays lean. Local Release builds can use the same flag if you want parity:

```bash
cmake -S cpp -B build-cpp -DCMAKE_BUILD_TYPE=Release -DPAXP2T_RELEASE_MINIMAL=ON
```

Extract the archive and run:

```bash
tar xf paxp2t-v0.1.0-linux-x86_64-portable.tar.gz
cd paxp2t-v0.1.0-linux-x86_64-portable
./AppRun
```

You still need a normal desktop stack on the host (X11, PulseAudio or PipeWire-Pulse, etc.); the bundle mainly removes the “install Qt6 from the distro” requirement.

### Flatpak later

Flatpak does **not** usually mean “one binary with Qt embedded.” It means the app is packaged against a **runtime** (for example a KDE/Qt runtime) declared in a manifest, plus your files. Bundling with linuxdeploy is still useful as a stepping stone or for non-Flatpak distribution; a Flatpak manifest would declare dependencies differently.

## Notes

- Won't work on Wayland-based systems or any other system with no X11-served displays.
- On desktops without tray host support, the app keeps working without tray. If pulseaudio is available, the soudnd indication will still work.
