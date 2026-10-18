<div align="center">

<img src="icon.jpg" alt="papaspaleteria_nx" width="160">

# papaspaleteria_nx

**Papa's Paleteria To Go! on Nintendo Switch**

An unofficial Nintendo Switch port of the Android version of  
**Papa's Paleteria To Go! — 1.1.1**

**By xFlippy**

[![Switch](https://img.shields.io/badge/Nintendo_Switch-Homebrew-E60012?style=for-the-badge&logo=nintendoswitch&logoColor=white)](#)
[![Ko-fi](https://img.shields.io/badge/Support_on_Ko--fi-FF5E5B?style=for-the-badge&logo=ko-fi&logoColor=white)](https://ko-fi.com/flippyy)

</div>

---

## About

`papaspaleteria_nx` is an experimental standalone Nintendo Switch port targeting
Papa's Paleteria To Go! Android 1.1.1 (ARM64).

It provides the compatibility layer that loads the Android package under Horizon
OS. It opens the game directly and needs no APK64 installation.

> Supply your own legally obtained APK. The game package, native libraries and gameplay

---

## Controls

Use the game's on-screen controls through the touchscreen, or enable the cursor:

- **Minus** toggles the outlined ice-blue cursor.
- **Left stick / D-pad** move it; **A** taps. Hold **A** while moving to drag.
- Touchscreen input takes priority. Release **A** before resuming cursor clicks.
- **Plus** exits to hbmenu.

### Launcher Dialog

The standalone automatically chooses **Decline** on the supplied release's
bandwidth-sharing prompt through its original button callback. No launcher
selection is needed. Other dialogs keep their normal controls.

- Touch an action, or use **D-pad up/down** to choose and **A** to select.
- **L/R** scroll long dialogs; dragging also scrolls.
- **B** sends Back; dialogs may disable cancellation.
- **Plus** exits to hbmenu.

---

## Build

### Requirements

- [devkitPro](https://devkitpro.org/), devkitA64 and libnx
- Switch portlibs: Mesa/libdrm_nouveau, libvorbis/libogg, FreeType, curl,
  Mbed TLS and zlib
- Linux or WSL, GNU Make, Bash, Python 3.11+, curl, tar and patch
- A host C compiler, zlib and OpenSSL development headers for `make check`
- Host Mesa EGL/OpenGL libraries (libEGL.so.1 and libGL.so.1) for the real GLES cursor checks
- Your own legally obtained Papa's Paleteria To Go! Android 1.1.1 ARM64 APK

### Compile

Open the source checkout:

```bash
cd papaspaleteria_nx
```

Configure your devkitPro environment:

```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITA64=$DEVKITPRO/devkitA64
export PATH=$DEVKITA64/bin:$DEVKITPRO/tools/bin:$PATH
```

Build the homebrew:

```bash
make -j$(nproc)
```

To rebuild from scratch:

```bash
make clean
make -j$(nproc)
```

The output is `papaspaleteria_nx.nro`. The build enforces a maximum NRO size of
13,000,000 bytes. The NRO contains the complete compressed runtime, including
platform tables, boot libraries, fonts and notices. No system folder is needed.

## Running

Extract the SD ZIP onto your SD card and put your original APK beside the NRO:

```text
sd:/switch/papaspaleteria_nx/
├── papaspaleteria_nx.nro
└── game.apk
```

Use application/title takeover mode. Hold **R** while opening an installed game
to enter hbmenu, then launch Papa's Paleteria To Go! Applet mode is not the
supported launch mode.

Only these two files need to be installed manually. On first launch or update,
the NRO unpacks its complete runtime to `cache/runtime.nro`, verifies its SHA-256,
then starts it through hbloader. This derived cache is about 56 MB and is rebuilt
if damaged or outdated. Allow about 120 MB free for cache updates. A normal
hbmenu/hbloader environment with next-load support is required.

The port creates `saves/`, `cache/`, `config/` and `logs/` as needed. Writable game
data stays under `sd:/switch/papaspaleteria_nx/`. Keep `saves/` when updating;
replace only `papaspaleteria_nx.nro`, leaving your APK and save folders in place.
The original Android package identity is preserved. This port does not import
saves from an existing APK64 installation automatically.

Run `make check` after building for bootstrap, storage and platform-data checks, and `make package` to
create source and SD ZIPs. Neither ZIP contains your game APK.

---

## Support

Bug reports with your game version, launch mode and relevant log output help
improve the port.

---

## Disclaimer

This is an unofficial fan project. It is not affiliated with or endorsed by
Flipline Studios.

Papa's Paleteria To Go!, its trademarks, launcher artwork, audio and game assets
belong to their respective rightsholders. The homebrew menu author field
identifies the port maintainer, xFlippy. See `LICENSES.md` for inherited source
and third-party distribution requirements.

## Contribution

Bug reports, performance improvements and documented pull requests are welcome.
Include your game version, launch mode, reproduction steps and relevant log
output. Do not attach game packages, native libraries, personal save data or
credentials to public issues.
