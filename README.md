# pttd

`pttd` is a push-to-talk utility allowing to set system-wide unmute and mute trigger,
by either mouse or keyboard press&release actions.

## Features

- Global push-to-talk bindings via **evdev** (preferred) or **XRecord** on X11 fallback; one config list uses Linux `BTN_*` / `KEY_*` codes
- PulseAudio total source mute/unmute via `pactl`
- Sound indication of unmute/mute actions via in-process PulseAudio playback (libpulse)
  - sounds stored as `.wav` under `~/.local/pttd/sounds/`, easy to replace (PCM16 mono/stereo; restart after changing)
- Tray icon via `QSystemTrayIcon` with:
  - a tray icon, toggling its color
  - `Open config`
  - `Terminate`
- Typed config read/backfill/rewrite at:
  - `~/.local/pttd/config.yml`

## Configurability

### Binds (push-to-talk buttons and keys)

Defaults on first run (`~/.local/pttd/config.yml`):

```yaml
BIND_PTT: [BTN_EXTRA, KEY_CAPSLOCK]
```

`BIND_PTT` is a YAML list of **Linux evdev** names (`BTN_*` for mouse buttons, `KEY_*` for keyboard keys) or decimal `EV_KEY` codes. Any listed control acts as push-to-talk (press = unmute, release = mute after delay).

```yaml
# One thumb button - the Forward
BIND_PTT: [BTN_EXTRA]

# Forward+Back thumb buttons + Caps Lock
BIND_PTT: [BTN_EXTRA, BTN_SIDE, KEY_CAPSLOCK]

# Keyboard Capslock only
BIND_PTT: [KEY_CAPSLOCK]
```

**Input backends:** pttd tries **evdev** first (reads `/dev/input/by-id/*-event-mouse` and `*-event-kbd`). If devices cannot be opened, it falls back to **XRecord** on X11 and maps the same `BIND_PTT` tokens to X11 buttons/keycodes internally. Startup logs `PTT input: evdev` or `PTT input: xrecord`.

**Permissions (evdev):** the running user must be able to read event nodes, e.g. If You just want to test, give it to Yourself with `sudo usermod -aG input "$USER"`, then log out and in again.
The advised way of running the app -- as a daemon with a dedicated user in the `input` group.

**Finding `BTN_*` / `KEY_*` codes (recommended)** — use evdev event devices, not X11 `xev` button numbers:

```bash
# Mice (multi-mouse safe)
.github/scripts/evdev-watch-mice.py
# OR, to only show 275 and 276 buttons acting
.github/scripts/evdev-watch-mice.py --watch 275,276

# Keyboards (all *-event-kbd nodes)
.github/scripts/evdev-watch-keyboard.py
# OR, to only show 58 and 125 keys acting
.github/scripts/evdev-watch-keyboard.py --watch 58,125
```

Press side buttons on each mouse; the script prints names like `BTN_SIDE (code=275)`. For keys, the keyboard script prints `KEY_CAPSLOCK (code=58)` and a ready-made `BIND_PTT: [KEY_CAPSLOCK]` hint.

Python watch scripts and the C++ app share the same vendored UAPI [`.github/scripts/vendor/linux/input-event-codes.h`](.github/scripts/vendor/linux/input-event-codes.h). Regenerate lookup tables after updating it:

```bash
.github/scripts/gen-evdev-code-tables.py
```

That refreshes [`.github/scripts/evdev_code_tables.py`](.github/scripts/evdev_code_tables.py) (watch scripts) and [`cpp/src/evdev_code_name_table.cpp`](cpp/src/evdev_code_name_table.cpp) (`BIND_PTT` token parsing). **XRecord fallback** still uses a small hand-written map in [`evdev_to_x11_map.cpp`](cpp/src/evdev_to_x11_map.cpp) (evdev code → X11 button/keysym), not the full kernel table. \
After such update the app has to be re-built. \
Note that Your distro probably has the original of that mapping under `/usr/include/linux/`. In case any key codes are not mapping as expected, try comparing the one in the repo with the one on Your system. 

Alternatives for listening to keyboard and mouse events: \
`evtest` on a `*-event-kbd` node, or `showkey -s` in a TTY.

### Icons

After first run, You can replace the icons under `~/.local/pttd/icons/` with any svg You like.
Changing color is easy - just edit the .svg icons with a text editor and replace the color code of `fill` value in the `circle` tag.

## Considerations
- Cannot mute sources selectively - do not expect _any_ of pulseaudio inputs to stay unmuted while pttd is active;
- Adaptive noise/echo cancellation codecs might get a bit mad with such mic behavior - in _some_ cases best disable them;
- Caches sources at startup. After connecting a new mic or otherwise adding an input, please restart pttd. If You do that a lot, consider setting `CACHE_INPUTS` to `false`;

## Build

### Toolchain

- **CMake** ≥ 3.20 (`cmake`)
- **C++17 compiler** — GCC or Clang with standard library (Debian/Ubuntu: `build-essential`; openSUSE: `patterns-devel-cpp` or `gcc-c++` + `cmake`)

### Libraries (development packages)

Qt6 including **Svg** (tray icons are SVG files loaded via `QSvgRenderer`, not the optional image-format plugin), plus X11 headers/libs:

| Role | Debian / Ubuntu | openSUSE |
|------|-----------------|----------|
| Qt6 Core, Gui, Widgets | `qt6-base-dev` (+ **`libgl-dev`** on minimal images so CMake finds `WrapOpenGL`) | Qt6 devel metapackage / `qt6-core-devel` etc. (same as any Qt6 app) |
| Qt6 Svg | **`libqt6svg6-dev`** on Ubuntu 22.04; **`qt6-svg-dev`** on Ubuntu 24.04+ (same CMake target) | **`qt6-svg-devel`** (`libQt6Svg6` alone is runtime-only and will **not** satisfy CMake — `zypper wp …/Qt6SvgConfig.cmake`) |
| X11 | `libx11-dev` | `libX11-devel` |
| XTest (XRecord input fallback) | `libxtst-dev` | `libXtst-devel` |
| PulseAudio (indicator sounds) | `libpulse-dev` | `libpulse-devel` |
| Linux input UAPI for compile | Vendored under `.github/scripts/vendor/linux/` (regen via `gen-evdev-code-tables.py`) | same checkout; no extra distro package required for `KEY_*`/`BTN_*` names |

**Example (Debian/Ubuntu):**

```bash
sudo apt-get install -y --no-install-recommends \
  build-essential cmake pkg-config libgl-dev \
  qt6-base-dev libqt6svg6-dev libx11-dev libxtst-dev libpulse-dev
```

### Compile

```bash
# Configure: source dir is 'cpp', build dir is 'build-cpp'
cmake -S cpp -B build-cpp
# Build; '-j' uses all CPU cores
cmake --build build-cpp -j
```

### Run

From the **repository root** (same place you ran `cmake`):

```bash
./build-cpp/pttd
```

For **evdev** PTT, your user needs read access to `/dev/input/event*` (see **Binds** → permissions). The **tray** and **XRecord** fallback still need a display (`DISPLAY` set; X11 or XWayland with XCB). PulseAudio or PipeWire-Pulse is required for mute/unmute.

### Optional: portable AppDir tarball

Same repo script as [Releases (portable Linux)](#releases-portable-linux) — [`.github/scripts/build-portable-bundle.sh`](.github/scripts/build-portable-bundle.sh). Extra tools on top of the table above:

| Tool | Debian / Ubuntu | Notes |
|------|-----------------|-------|
| `curl` | `curl` | fetch linuxdeploy AppImages |
| `strip` | `binutils` | shrink bundled `.so` / executable |
| `file` | `file` | ELF checks in bundle scripts |
| `convert` | `imagemagick` | only if `packaging/pttd.png` is missing |
| `bash` | (preinstalled) | trim / audit scripts |

linuxdeploy binaries are downloaded automatically on first run (cached under `~/.cache/pttd-release-tools`).

## Releases (portable Linux)

The GitHub Actions **Release** workflow and your machine can run the same script: [`.github/scripts/build-portable-bundle.sh`](.github/scripts/build-portable-bundle.sh).

### Dry-run locally (no GitHub Release)

From the repo root, after installing build dependencies (same as README **Build**, including Qt Svg):

```bash
.github/scripts/build-portable-bundle.sh               # RELEASE_VERSION defaults to git describe or local-<timestamp>
.github/scripts/build-portable-bundle.sh v9.9.9-test # optional explicit name for the tarball
```

The tarball is written to **`dist/`** (`OUT_DIR`; override with env). linuxdeploy downloads are cached under **`~/.cache/pttd-release-tools`** unless you set **`pttd_RELEASE_TOOLS_DIR`**.

Pushing a version tag triggers the **Release** workflow, which runs the same script with the tag name and uploads **`dist/pttd-<tag>-linux-x86_64-portable.tar.gz`** to GitHub Releases.

That bundle is an **AppDir-style tree**: [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy) plus the Qt plugin copy Qt libs next to the binary so recipients do not need system Qt packages.

After bundling and **`strip`**, the script **aggressively trims** leftovers linuxdeploy still copies:

- **`*.qm`** / translation dirs · **QML** · **sqldrivers** · **doc/man**
- **Non-XCB platform plugins** (Wayland/offscreen leftovers; **`libqxcb.so` stays** only)
- **`platformthemes`** and **Wayland** plugin dirs
- **`plugins/imageformats`** and **`plugins/iconengines`** (tray rasterizes SVG via linked **Qt Svg**)
- **`plugins/tls`**, **`plugins/multimedia`**
- **`platforminputcontexts`**: removes **IBus** and **Qt Virtual Keyboard** only (**Compose** context stays)
- **`xcbglintegrations`** (EGL/GLX XCB backends not needed for tray + widgets here)
- Obvious stray **Qt/KDE module** `.so` names (Quick, QML, Vulkan, Charts, Multimedia, NFC, …), **`*.a`**, **`*.debug`**, **`*.dwz`**
- **`.github/scripts/prune-appdir-libs.sh`**: **`ldd` transitive closure** from **`usr/bin/pttd`** + every **`usr/plugins/**/*.so`**, then delete anything in **`usr/lib/`** not in that closure (large savings: codec stacks, **KF6Archive**, OpenSSL tails, **VirtualKeyboard**, … when not actually linked)
- Empty icon dirs under **`usr/share/icons`**, then **every remaining empty directory** under the trimmed AppDir (drops hollow **`pixmaps/`** stubs, etc.)
- **`.github/scripts/check-portable-ldd.sh --fail-orphans`** (after trim, before tarball): fails the build on unresolved SONAMEs or orphan **`usr/lib`** blobs — also runs automatically in the **Release** workflow via **`build-portable-bundle.sh`**
- **`.github/scripts/check-portable-glibc.sh`** (same stage): fails if bundled ELFs need **GLIBC** newer than **`pttd_GLIBC_MAX`** (default **2.35**, Ubuntu 22.04 baseline)

It logs **`du -sh`** before and after. Set **`pttd_SKIP_BUNDLE_TRIM=1`** to skip this whole pass.

What’s left is mostly **Qt Gui/Widgets/Core + Svg**, **XCB + X11-ish deps**, and **`libqxcb.so`**’s own dependencies.

The script configures **`pttd_RELEASE_MINIMAL=ON`** for smaller Release binaries (**`-Os`**, section **`--gc-sections`**, **`--as-needed`**, **[LTO](https://cmake.org/cmake/help/latest/module/CheckIPOSupported.html)** when the toolchain supports it), **`strip`** on the exe and bundled **`*.so`**, then **`tar.gz`** with **`GZIP=-9`** (**`pttd_ARCHIVE_GZIP`** overrides) so the downloaded archive is tighter without changing what extractors receive. For an ordinary Release build without linuxdeploy you can apply the same flag when running CMake manually.

#### When the tree stops shrinking (~tens of MB uncompressed)

Portable builds use distro Qt (Release CI on **Ubuntu 22.04**), which on Ubuntu pulls **full ICU** as **`Qt6Core`’s transitive dependency**. **`libicudata.so`** (Unicode / locale payload) commonly dominates **`usr/lib/`** sizes; it is kept because **Qt**, not trimming heuristics, actually links it — `ldd`-closure pruning is already doing honest work here. Shrinking ICU further means distributing a Qt built with **minimal or no ICU**, which is outside this repo’s APT-based workflow (**Flatpak**/custom Qt / different distro runtimes).

**`libQt6DBus`**, **`libdbus`**, **`glib`**, **`systemd`**: likewise normal fallout of Linux **Qt Gui** desktop integration (`QGuiApplication` pulls this stack on typical builds). Removing it would risk broken session/notifications/integration rather than reclaiming predictable space.

Inspect what’s bulky with:

```bash
du -h --max-depth=1 pttd-*-portable/usr/lib | sort -h
```

Audit **NEEDED** dependencies (same seeds as the pruner: exe + every plugin `.so`). **`ldd` does not see `dlopen()`** — only link-time **`DT_NEEDED`** edges and whatever you explicitly **`ldd`** on (hence seeding plugins):

```bash
.github/scripts/check-portable-ldd.sh pttd-*-portable          # hybrid: allow glibc/X11/Mesa/fonts on host
.github/scripts/check-portable-ldd.sh --strict pttd-*-portable # everything else must be under the AppDir
```

For **runtime-only** loads (Qt picking a plugin after a menu click, GL drivers, NSS), exercise the app and capture the loader log:

```bash
APPD="$(readlink -f pttd-*-portable)"
LD_DEBUG=libs LD_LIBRARY_PATH="$APPD/usr/lib" "$APPD/usr/bin/pttd" 2>&1 | tee /tmp/pttd-ld.log
grep -E 'calling init:|file=' /tmp/pttd-ld.log
```

Extract the archive and run:

```bash
tar xf pttd-v0.1.0-linux-x86_64-portable.tar.gz
cd pttd-v0.1.0-linux-x86_64-portable
./AppRun
```

You still need PulseAudio or PipeWire-Pulse for mute/unmute. **PTT input** prefers **evdev** (`input` group); the portable tree is still **XCB/Qt-on-X11** for the tray UI. XRecord fallback needs an X11 session and `DISPLAY`.

### Flatpak later

Flatpak does **not** usually mean “one binary with Qt embedded.” It means the app is packaged against a **runtime** (for example a KDE/Qt runtime) declared in a manifest, plus your files. Bundling with linuxdeploy is still useful as a stepping stone or for non-Flatpak distribution; a Flatpak manifest would declare dependencies differently.

## Troubleshooting

### `GLIBC_2.xx not found` when running the portable tarball

Example:

```text
./AppRun: /lib/x86_64-linux-gnu/libc.so.6: version `GLIBC_2.38' not found
  (required by .../usr/lib/libQt6Gui.so.6)
```

The portable bundle **does not ship `libc.so.6`** — the dynamic linker always uses the host’s glibc. The bundled **Qt** libraries were built on a **newer** machine (including official releases built on **Ubuntu 22.04**, or any newer build host) and may expect a **newer** glibc than your system provides. \
See **Build** section for build dependencies.

**Fix:** rebuild on **your** machine so linuxdeploy copies Qt/libs linked against **your** glibc. From a checkout of this repo:

```bash
# Install build deps from the **Build** section (qt6-base-dev, libqt6svg6-dev, libx11-dev, libxtst-dev, …)
.github/scripts/build-portable-bundle.sh
tar xf dist/pttd-*-linux-x86_64-portable.tar.gz
cd pttd-*-linux-x86_64-portable
./AppRun
```

Alternatively, build and run without linuxdeploy (same deps as **Build**):

```bash
cmake -S cpp -B build-cpp
cmake --build build-cpp -j
./build-cpp/pttd
```

Check your host glibc with `ldd --version | head -1`. After a local portable rebuild, optional sanity checks:

```bash
./.github/scripts/check-portable-glibc.sh pttd-*-portable
./.github/scripts/check-portable-ldd.sh pttd-*-portable
```

## Notes

- Portable **AppRun** uses the **XCB** platform plugin (not a native Wayland bundle). Evdev PTT can work without XRecord, but the tray still needs a display server Qt can reach (typically X11 or XWayland).
- On desktops without tray host support, the app keeps working without tray. If PulseAudio is available, the sound indication will still work.
