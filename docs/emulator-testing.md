# Running Doli in an emulator

This documents the local test method used on 2026-10-31. **Eden loaded the NRO but did not reach Doli's interface. Ryujinx did**, after the compatibility changes below. The Eden steps reproduce a launch attempt, not a confirmed working configuration.

## Tested environment

| Component | Local test environment |
| --- | --- |
| Host | macOS 15.8, Apple M4 |
| Eden | 0.2.1, MoltenVK / Vulkan, 1× resolution, Docked |
| Ryujinx | 1.3.3, Vulkan, 1× resolution, Docked |
| Installed firmware | 22.0.0 |
| App | Doli 2.0.0 BETA 1, release NRO |

The application renders at 1280×720. The emulator window was larger on the Retina display; that does not verify readability or touch comfort at the Switch's physical screen size.

## 1. Build the application

With the dependencies listed in the [README](../README.md#building), run from the repository root:

```sh
make -j6 DEV=0
```

The output is `Doli.nro`, with its RomFS assets included. Load the application NRO, not the overlay `.ovl` or background-player `.nsp`.

The local macOS toolchain was missing Switch FFmpeg and libxml development packages. The successful build used the existing devkitPro Docker image in a separate build copy:

```sh
# Run from the repository root. Requires rsync and Docker.
# Use a fresh directory so old build objects cannot be reused.
test_build="$(mktemp -d "${TMPDIR:-/tmp}/doli-emulator.XXXXXX")"
rsync -a --exclude=.git --exclude=build --exclude=dist ./ "$test_build/"
docker run --rm \
  -v "$test_build:/work" -w /work \
  devkitpro/devkita64:latest make -j6 DEV=0
printf 'Load this file: %s/Doli.nro\n' "$test_build"
```

Keep that path for the emulator's file picker. This procedure builds the source in your working directory, including uncommitted changes.

## 2. Prepare a controlled SD-card library

Use the SD-card directory configured in the emulator. On the tested Mac these were:

- Eden: `~/.local/share/eden/sdmc/`
- Ryujinx: `~/Library/Application Support/Ryujinx/sdcard/`

These are observed local paths, not guaranteed defaults on other installations. Eden's local configuration recorded its path as `sdmc_directory` in `~/.config/eden/qt-config.ini`.

The directory layout for a minimal test is:

```text
<emulated SD card>/
├── config/
│   └── gatefold-nx/
│       └── settings.txt
└── doli-emulator-test/
    └── test.flac
```

Copy music with album/artist/title tags into `doli-emulator-test`. Alternatively, generate a two-minute test tone with host FFmpeg:

```sh
# Set this to the SD-card path configured in your emulator.
# This example uses the observed Eden path on macOS.
emulator_sd="$HOME/.local/share/eden/sdmc"
mkdir -p "$emulator_sd/doli-emulator-test"
ffmpeg -n -v error -f lavfi -i 'sine=frequency=440:duration=120' \
  -c:a flac -metadata 'album=Emulator test' \
  -metadata 'artist=Device test' -metadata 'title=Test track' \
  "$emulator_sd/doli-emulator-test/test.flac"
```

With emulation stopped, back up an existing `config/gatefold-nx/settings.txt` before editing it. Create its parent directories if this is a fresh SD card. Set these entries, replacing existing entries rather than appending duplicates:

```ini
background=0
resume=0
musicRoot=sdmc:/doli-emulator-test
volume=0
libTab=0
```

`background=0` uses Doli's local player instead of probing the optional Atmosphère background service. `resume=0` avoids restoring a queue from another library. `volume=0` keeps the generated tone silent; temporarily raise it in Settings → Playback if you need to check audio.

For the UI test, background cover/analysis work was also paused:

```ini
stopCovers=1
stopThumbs=1
stopLevels=1
```

This controlled SD-card setup was used for the successful Ryujinx test. It has **not** been demonstrated to resolve Eden's startup failure; the original Eden launch attempt used its existing SD-card contents.

## 3. Load the NRO in Eden

1. Start Eden with its firmware already configured. The test installation had firmware 22.0.0.
2. Use **File → Load File** (`⌘O` on the tested Mac).
3. Select the freshly built `Doli.nro`. In the macOS file picker, `⌘⇧G` opens Go to Folder; paste the full NRO path, press Return, then click Open.
4. Confirm the window title identifies **Doli (64-bit)**. The tested title also identified MoltenVK.
5. Wait for Doli's Library or first-run folder picker. A window title, Launching progress indicator, or recognized app icon alone does not establish that the app is running.

### Observed Eden failure

Eden 0.2.1 stayed at **Launching…**, with **0 FPS**. Its local log was:

```text
~/.local/share/eden/log/eden_log.txt
```

Relevant messages included:

```text
Unmapped Write32 @ 0x0000000000000000
assert !is_executing
Cannot execute instruction at unmapped address 0x00000000000000
```

The log also reported missing Vulkan extensions/features. These messages are evidence of the failed launch, not an established diagnosis. No Eden-specific workaround was verified. If this reproduces, preserve the log and record the emulator version, host, graphics backend and firmware before trying another configuration.

**Emulation → Stop** (`F5` in the tested installation) requested confirmation, but the failed session then hung at Closing software. The stalled test process had to be terminated. Do not assume that a hung emulator has saved its application state.

## 4. Working fallback: Ryujinx

1. Prepare Ryujinx's own emulated SD card as above, including `background=0`.
2. With Ryujinx closed, back up its configuration. On the tested Mac it was `~/Library/Application Support/Ryujinx/Config.json`.
3. Change the existing JSON property to `"ignore_missing_services": true`, preserving the rest of the file.
4. Start Ryujinx and choose **File → Load Application…**, then select `Doli.nro`.
5. Check that the actual Library renders, open an album, and start playback.

Without these changes, the local Ryujinx installation aborted on:

- The background-service probe: `IUserInterface: 65100`, with `gatefold` in the request data. Disabling background audio in the test SD-card settings avoided this probe.
- Raw battery-charge sampling: `IPsmServer: 13`. Enabling `ignore_missing_services` allowed the UI test to proceed.

The compatibility flag can mask unsupported services, so this run does not validate battery reporting or background audio. It is an emulator configuration change, not an application default or a fix for real hardware. Local logs were under `~/Library/Logs/Ryujinx/`.

## 5. Check interaction, not just startup

The successful test used four tagged albums / eight generated FLAC tracks, including a long title that wrapped onto two lines. The following were exercised in Ryujinx:

- Touch-style mouse input through Library → Album → Now Playing.
- D-pad focus movement and A activation through the same flow.
- Pause via X, an obvious transport focus outline, and B returning to the album with its previous focus.
- The final play/pause icon contrast and reduced default sidebar.

For automation, brief key taps could be missed by the emulator's sampled input. Holding a key for roughly 180 ms worked in the local test. Read the emulator's actual controller mapping; do not assume the following local keyboard bindings are universal:

| Switch control | Local Ryujinx keyboard binding |
| --- | --- |
| D-pad | Arrow keys |
| A / B / X / Y | Z / X / C / V |
| L / R | E / U |
| ZL / ZR | Q / O |
| Minus / Plus | Minus / Plus |

Seeking, lyrics, Automix transitions, large-library scrolling, light themes, physical touch comfort and real-device performance still need dedicated checks. Passing host regression tests does not substitute for those checks:

```sh
bash tools/tests/run.sh
```

## 6. Clean up

Stop emulation before restoring files. Restore the backed-up Doli settings and Ryujinx compatibility value, and remove only the generated test music/configuration that you created. Keep existing libraries, keys, firmware and other applications intact. The temporary Ryujinx compatibility change and generated SD-card fixture were removed after the local test.
