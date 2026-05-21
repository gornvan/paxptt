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

You need Qt6 development libraries including **Svg** (tray icons are SVG files loaded via `QSvgRenderer`, not the optional image-format plugin):

- Debian/Ubuntu: `qt6-base-dev`, `qt6-svg-dev`, `libx11-dev`, `libxtst-dev`
- openSUSE: **`qt6-svg-devel`** installs `Qt6SvgConfig.cmake`; **`libQt6Svg6`** is runtime-only and will **not** satisfy CMake. Add base Qt build deps as for any Qt app (e.g. `qt6-core-private-devel` grouping varies — `zypper search -s qt6 svg` / `zypper wp /usr/lib64/cmake/Qt6Svg/Qt6SvgConfig.cmake`).

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

The GitHub Actions **Release** workflow and your machine can run the same script: [`.github/scripts/build-portable-bundle.sh`](.github/scripts/build-portable-bundle.sh).

### Dry-run locally (no GitHub Release)

From the repo root, after installing build dependencies (same as README **Build**, including Qt Svg):

```bash
.github/scripts/build-portable-bundle.sh               # RELEASE_VERSION defaults to git describe or local-<timestamp>
.github/scripts/build-portable-bundle.sh v9.9.9-test # optional explicit name for the tarball
```

The tarball is written to **`dist/`** (`OUT_DIR`; override with env). linuxdeploy downloads are cached under **`~/.cache/paxp2t-release-tools`** unless you set **`PAXP2T_RELEASE_TOOLS_DIR`**.

Pushing a version tag triggers the **Release** workflow, which runs the same script with the tag name and uploads **`dist/paxp2t-<tag>-linux-x86_64-portable.tar.gz`** to GitHub Releases.

That bundle is an **AppDir-style tree**: [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy) plus the Qt plugin copy Qt libs next to the binary so recipients do not need system Qt packages.

After bundling and **`strip`**, the script **`trim`**s obvious dead weight linuxdeploy tends to drag in anyway: **`translations`** (often tens of MiB of unused `.qm` locales), **`qml`** trees, **`sqldrivers`** (this app doesn’t use SQL), and distro **`usr/share/doc` / `man`**. Logs `du -sh` before and after. Set **`PAXP2T_SKIP_BUNDLE_TRIM=1`** if you ever need an untrimmed tree for debugging.

Remaining size is mostly **Qt Gui/Widgets/XCB + Svg** libraries and **`platformplugins`** (`libqxcb.so` loads the rest).

The script configures **`PAXP2T_RELEASE_MINIMAL=ON`** for smaller Release binaries (**`-Os`**, section **`--gc-sections`**, **`--as-needed`**), strips the executable and bundled **`*.so`**, then archives the portable directory. For an ordinary Release build without linuxdeploy you can apply the same flag when running CMake manually.

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
