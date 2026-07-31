# paxp2t

`paxp2t` is a push-to-talk utility allowing to set system-wide unmute and mute trigger,
by either a mouse or a keyboard button.

## Features

- X11 global mouse/keyboard bindings via XRecord (multiple PTT keys and mouse buttons configurable)
- PulseAudio total source mute/unmute via `pactl`
- Sound indication of unmute/mute actions via in-process PulseAudio playback (libpulse)
  - sounds stored as `.wav` under `~/.local/paxp2t/sounds/`, easy to replace (PCM16 mono/stereo; restart after changing)
- Tray icon via `QSystemTrayIcon` with:
  - a tray icon, toggling its color
  - `Open config`
  - `Terminate`
- Typed config read/backfill/rewrite at:
  - `~/.local/paxp2t/config.yml`

## Configurability

### Binds (push-to-talk buttons and keys)

Defaults on first run (`~/.local/paxp2t/config.yml`):

```yaml
BIND_KEYBOARD_KEYSYM: [Caps_Lock]
BIND_MOUSE_BUTTON: [9]
```

Both settings accept either a **single value** (legacy) or a **YAML inline list** — any listed control acts as push-to-talk (press = unmute, release = mute after delay).

**Mouse** — X11 button numbers (`1` = left, `2` = middle, `3` = right, `9` = forward on many mice):

```yaml
# One thumb button
BIND_MOUSE_BUTTON: [9]

# Two thumb buttons
BIND_MOUSE_BUTTON: [9, 8]

# No mouse bind
BIND_MOUSE_BUTTON: []
```

**Keyboard** — [X11 keysym](https://cgit.freedesktop.org/xorg/proto/xproto/tree/keysymdef.h) names (`Caps_Lock`, `F24`, `Pause`, …):

```yaml
# One key
BIND_KEYBOARD_KEYSYM: [Caps_Lock]

# PTT on either of two keys
BIND_KEYBOARD_KEYSYM: [Caps_Lock, Scroll_Lock]

# No keyboard bind
BIND_KEYBOARD_KEYSYM: []
```

The plural key names `BIND_MOUSE_BUTTONS` / `BIND_KEYBOARD_KEYSYMS` are accepted as aliases when reading; the file is rewritten with the singular names above.

**Finding keysym names** — run in a terminal; each keypress prints a name:

```bash
# xev might need to be installed
xev -event keyboard | grep --line-buffered keycode | sed -E 's/.*, ([^,^\)]*)\).*/\1/'
```

**Finding mouse button numbers** — run the following command in a terminal to monitor mouse button presses:

```bash
xev -event button | grep -A2 --line-buffered ButtonPress
```

Then press the desired button and look for lines like `ButtonPress event, serial ..., button N, ...`, where `N` is the X11 button number to use.

Restart paxp2t after editing the config.

### Icons

After first run, You can replace the icons under `~/.local/paxp2t/icons/` with any svg You like.
Changing color is easy - just edit the .svg icons with a text editor and replace the color code of `fill` value in the `circle` tag.

## Considerations
- Cannot mute sources selectively - do not expect _any_ of pulseaudio inputs to stay unmuted while paxp2t is active;
- Adaptive noise/echo cancellation codecs might get a bit mad with such mic behavior - in _some_ cases best disable them;
- Caches sources at startup. After connecting a new mic or otherwise adding an input, please restart paxp2t. If You do that a lot, consider setting `CACHE_INPUTS` to `false`;

## Build

### Toolchain

- **CMake** ≥ 3.20 (`cmake`)
- **C++17 compiler** — GCC or Clang with standard library (Debian/Ubuntu: `build-essential`; openSUSE: `patterns-devel-cpp` or `gcc-c++` + `cmake`)

### Libraries (development packages)

Qt6 including **Svg** (tray icons are SVG files loaded via `QSvgRenderer`, not the optional image-format plugin), plus X11 headers/libs:

| Role | Debian / Ubuntu | openSUSE |
|------|-----------------|----------|
| Qt6 Core, Gui, Widgets | `qt6-base-dev` | Qt6 devel metapackage / `qt6-core-devel` etc. (same as any Qt6 app) |
| Qt6 Svg | `qt6-svg-dev` | **`qt6-svg-devel`** (`libQt6Svg6` alone is runtime-only and will **not** satisfy CMake — `zypper wp …/Qt6SvgConfig.cmake`) |
| X11 | `libx11-dev` | `libX11-devel` |
| XTest (global input) | `libxtst-dev` | `libXtst-devel` |
| PulseAudio (indicator sounds) | `libpulse-dev` | `libpulse-devel` |

**Example (Debian/Ubuntu):**

```bash
sudo apt-get install -y --no-install-recommends \
  build-essential cmake \
  qt6-base-dev qt6-svg-dev libx11-dev libxtst-dev libpulse-dev
```

### Compile

```bash
# Configure: source dir is 'cpp', build dir is 'build-cpp'
cmake -S cpp -B build-cpp
# Build; '-j' uses all CPU cores
cmake --build build-cpp -j
```

### Run

```bash
./build-cpp/paxp2t
```

### Optional: portable AppDir tarball

Same repo script as [Releases (portable Linux)](#releases-portable-linux) — [`.github/scripts/build-portable-bundle.sh`](.github/scripts/build-portable-bundle.sh). Extra tools on top of the table above:

| Tool | Debian / Ubuntu | Notes |
|------|-----------------|-------|
| `curl` | `curl` | fetch linuxdeploy AppImages |
| `strip` | `binutils` | shrink bundled `.so` / executable |
| `file` | `file` | ELF checks in bundle scripts |
| `convert` | `imagemagick` | only if `packaging/paxp2t.png` is missing |
| `bash` | (preinstalled) | trim / audit scripts |

linuxdeploy binaries are downloaded automatically on first run (cached under `~/.cache/paxp2t-release-tools`).

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

After bundling and **`strip`**, the script **aggressively trims** leftovers linuxdeploy still copies:

- **`*.qm`** / translation dirs · **QML** · **sqldrivers** · **doc/man**
- **Non-XCB platform plugins** (Wayland/offscreen leftovers; **`libqxcb.so` stays** only)
- **`platformthemes`** and **Wayland** plugin dirs
- **`plugins/imageformats`** and **`plugins/iconengines`** (tray rasterizes SVG via linked **Qt Svg**)
- **`plugins/tls`**, **`plugins/multimedia`**
- **`platforminputcontexts`**: removes **IBus** and **Qt Virtual Keyboard** only (**Compose** context stays)
- **`xcbglintegrations`** (EGL/GLX XCB backends not needed for tray + widgets here)
- Obvious stray **Qt/KDE module** `.so` names (Quick, QML, Vulkan, Charts, Multimedia, NFC, …), **`*.a`**, **`*.debug`**, **`*.dwz`**
- **`.github/scripts/prune-appdir-libs.sh`**: **`ldd` transitive closure** from **`usr/bin/paxp2t`** + every **`usr/plugins/**/*.so`**, then delete anything in **`usr/lib/`** not in that closure (large savings: codec stacks, **KF6Archive**, OpenSSL tails, **VirtualKeyboard**, … when not actually linked)
- Empty icon dirs under **`usr/share/icons`**, then **every remaining empty directory** under the trimmed AppDir (drops hollow **`pixmaps/`** stubs, etc.)
- **`.github/scripts/check-portable-ldd.sh --fail-orphans`** (after trim, before tarball): fails the build on unresolved SONAMEs or orphan **`usr/lib`** blobs — also runs automatically in the **Release** workflow via **`build-portable-bundle.sh`**

It logs **`du -sh`** before and after. Set **`PAXP2T_SKIP_BUNDLE_TRIM=1`** to skip this whole pass.

What’s left is mostly **Qt Gui/Widgets/Core + Svg**, **XCB + X11-ish deps**, and **`libqxcb.so`**’s own dependencies.

The script configures **`PAXP2T_RELEASE_MINIMAL=ON`** for smaller Release binaries (**`-Os`**, section **`--gc-sections`**, **`--as-needed`**, **[LTO](https://cmake.org/cmake/help/latest/module/CheckIPOSupported.html)** when the toolchain supports it), **`strip`** on the exe and bundled **`*.so`**, then **`tar.gz`** with **`GZIP=-9`** (**`PAXP2T_ARCHIVE_GZIP`** overrides) so the downloaded archive is tighter without changing what extractors receive. For an ordinary Release build without linuxdeploy you can apply the same flag when running CMake manually.

#### When the tree stops shrinking (~tens of MB uncompressed)

Portable builds use distro Qt (Release CI on **Ubuntu 22.04**), which on Ubuntu pulls **full ICU** as **`Qt6Core`’s transitive dependency**. **`libicudata.so`** (Unicode / locale payload) commonly dominates **`usr/lib/`** sizes; it is kept because **Qt**, not trimming heuristics, actually links it — `ldd`-closure pruning is already doing honest work here. Shrinking ICU further means distributing a Qt built with **minimal or no ICU**, which is outside this repo’s APT-based workflow (**Flatpak**/custom Qt / different distro runtimes).

**`libQt6DBus`**, **`libdbus`**, **`glib`**, **`systemd`**: likewise normal fallout of Linux **Qt Gui** desktop integration (`QGuiApplication` pulls this stack on typical builds). Removing it would risk broken session/notifications/integration rather than reclaiming predictable space.

Inspect what’s bulky with:

```bash
du -h --max-depth=1 paxp2t-*-portable/usr/lib | sort -h
```

Audit **NEEDED** dependencies (same seeds as the pruner: exe + every plugin `.so`). **`ldd` does not see `dlopen()`** — only link-time **`DT_NEEDED`** edges and whatever you explicitly **`ldd`** on (hence seeding plugins):

```bash
.github/scripts/check-portable-ldd.sh paxp2t-*-portable          # hybrid: allow glibc/X11/Mesa/fonts on host
.github/scripts/check-portable-ldd.sh --strict paxp2t-*-portable # everything else must be under the AppDir
```

For **runtime-only** loads (Qt picking a plugin after a menu click, GL drivers, NSS), exercise the app and capture the loader log:

```bash
APPD="$(readlink -f paxp2t-*-portable)"
LD_DEBUG=libs LD_LIBRARY_PATH="$APPD/usr/lib" "$APPD/usr/bin/paxp2t" 2>&1 | tee /tmp/paxp2t-ld.log
grep -E 'calling init:|file=' /tmp/paxp2t-ld.log
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

## Troubleshooting

### `GLIBC_2.xx not found` when running the portable tarball

Example:

```text
./AppRun: /lib/x86_64-linux-gnu/libc.so.6: version `GLIBC_2.38' not found
  (required by .../usr/lib/libQt6Gui.so.6)
```

The portable bundle **does not ship `libc.so.6`** — the dynamic linker always uses the host’s glibc. The bundled **Qt** libraries were built on a **newer** machine (e.g. GitHub Actions on a recent Ubuntu) and expect a **newer** glibc than your system provides. \
See **Build** section for build dependencies.

**Fix:** rebuild on **your** machine so linuxdeploy copies Qt/libs linked against **your** glibc. From a checkout of this repo:

```bash
# Install build deps from the **Build** section (qt6-base-dev, qt6-svg-dev, libx11-dev, libxtst-dev, …)
.github/scripts/build-portable-bundle.sh
tar xf dist/paxp2t-*-linux-x86_64-portable.tar.gz
cd paxp2t-*-linux-x86_64-portable
./AppRun
```

Alternatively, build and run without linuxdeploy (same deps as **Build**):

```bash
cmake -S cpp -B build-cpp
cmake --build build-cpp -j
./build-cpp/paxp2t
```

Check your host glibc with `ldd --version | head -1`. After a local portable rebuild, optional sanity checks:

```bash
./.github/scripts/check-portable-glibc.sh paxp2t-*-portable
./.github/scripts/check-portable-ldd.sh paxp2t-*-portable
```

## Notes

- Won't work on Wayland-based systems or any other system with no X11-served displays.
- On desktops without tray host support, the app keeps working without tray. If PulseAudio is available, the sound indication will still work.
