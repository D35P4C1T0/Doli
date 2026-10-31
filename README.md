<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/wordmark-dark.png">
    <source media="(prefers-color-scheme: light)" srcset="docs/wordmark-light.png">
    <img src="docs/wordmark-dark.png" width="320" alt="Doli">
  </picture>
</p>
<p align="center"><b>A native music player for Nintendo Switch, with Subsonic support.</b><br>
Play your SD-card collection or music from your own Subsonic server.<br>
<a href="../../actions/workflows/build-nro.yml">Download a build</a> · <a href="../../issues">Report an issue</a></p>

Doli is a native C++ homebrew app built with devkitPro, libnx and SDL2. It is a fork of [Songate](https://github.com/AndryTheBeast/Songate) by AndryTheBeast, with remote Subsonic playback and its own identity and development direction.

## Features

- **Local and remote music.** Browse albums, artists and genres, search, build queues, like songs and create playlists from your SD card or a Subsonic server.
- **Cover colours and lyrics.** Album art colours the interface; local synced lyrics appear in Now playing and the full-screen Canvas.
- **Playback controls.** Equalizer, crossfade, sleep timer, play statistics and smart playlists.
- **Local-library extras.** Automix, mood playlists and volume levelling analyse your music on the console.
- **Optional background playback for local files.** A sysmodule keeps music playing on HOME and in games; an Ultrahand overlay controls it.
- **Touch and controller input.** Tap and drag, navigate with the D-pad, or use the left stick as a pointer.

## Subsonic

In **Settings → Subsonic**, enter the server base URL (for example `https://music.example.com`, including any base path but without `/rest`), username and password, then choose **Connect**. Doli reads the server's albums and songs into its library views. Search, queues, Liked and your own playlists work with remote tracks. **Library → Rescan** refreshes the server library; **Use SD card** returns to your remembered music folder.

Playback fetches only the selected song through the [Subsonic API](https://www.subsonic.org/pages/api.jsp). One complete song is buffered on the SD card before playback, using the server's MP3 stream at up to 192 kbps; enable MP3 transcoding on the server if needed. Now-playing cover art is fetched with the song. The buffer is replaced for the next song and removed on normal exit (at most 128 MiB of audio and 8 MiB of cover art). Network work runs on a worker thread. Remote libraries are limited to 100,000 songs.

**Remote music plays while Doli is open.** Remote Automix, loudness/mood analysis, background playback, folder browsing, library cover thumbnails, server playlist synchronisation and server tag edits are not implemented. The server must support token authentication and the album APIs (Subsonic API 1.16.1 or compatible).

Credentials are saved as plain text in `sdmc:/config/gatefold-nx/settings.txt`; requests use salted authentication tokens. Prefer HTTPS: certificate verification stays enabled, and HTTP traffic is unencrypted. Redirects are rejected; enter the server's final URL.

## Installation

You need a Switch running **Atmosphère**. Download **Doli-nro** from a successful [Build NRO workflow run](../../actions/workflows/build-nro.yml), extract `Doli.nro`, and copy it to `sdmc:/switch/Doli/Doli.nro`.

**Launch Doli by holding R while starting a game** (title override). Applet mode, from the Album, has too little memory and no keyboard.

Put music anywhere on the card. On first start, Doli uses `/Music` if present; otherwise it opens a folder picker. Change the folder in **Settings → Library → Music folder**, or connect a Subsonic server.

For optional local-file background playback, build the sysmodule and overlay as described below:

| Build output | SD-card destination |
|---|---|
| `Doli.nro` | `switch/Doli/Doli.nro` |
| `sysmodule/gatefold-bg.nsp` | `atmosphere/contents/4200000047464E58/exefs.nsp` |
| `sysmodule/toolbox.json` | `atmosphere/contents/4200000047464E58/toolbox.json` |
| `dist/atmosphere/contents/4200000047464E58/flags/boot2.flag` | `atmosphere/contents/4200000047464E58/flags/boot2.flag` |
| `overlay/Doli.ovl` | `switch/.overlays/Doli.ovl` |

Restart the console after installing or updating the sysmodule. The overlay requires **Ultrahand**; open it with L + D-pad down + R3 unless you changed its combo. Hidden folders such as `.overlays` may need to be shown in your file manager.

**Upgrading from Songate or Gatefold NX:** settings, playlists, Liked, statistics and Subsonic credentials continue using `sdmc:/config/gatefold-nx/`. The legacy directory, background-service identity and backup format stay compatible. Copy Doli first; you can then remove the old Songate app and overlay yourself. Doli's updater checks this fork's releases, independently of the old release cache. Automix model downloads still use the original [Songate Automix model repository](https://github.com/AndryTheBeast/songate-automix).

## Controls

| Button | Action |
|---|---|
| D-pad | Move focus |
| Left stick | Pointer; A taps or drags |
| A / B | Select / back |
| X / Y | Play or pause / search |
| L / R | Previous / next track |
| ZL / ZR | Seek −10 / +10 seconds |
| − / + | Full-screen Canvas / track menu |
| Right stick | Scroll |
| HOME | Leave Doli |

Touch supports tapping, dragging lists, seeking and volume changes. Settings → About includes a guided tour.

## Building

Requires [devkitPro](https://devkitpro.org/wiki/Getting_Started) with Switch support:

```bash
dkp-pacman -S switch-dev switch-sdl2 switch-sdl2_mixer switch-sdl2_ttf switch-sdl2_image switch-ffmpeg switch-libjpeg-turbo switch-curl switch-libxml2
make DEV=0               # Doli.nro, including its RomFS fonts and icons
make -C sysmodule        # gatefold-bg.nsp (stable internal service name)
make -C overlay          # Doli.ovl
```

GitHub Actions builds the release NRO on pushes and pull requests. Open **Actions → Build NRO**, select a successful run and download **Doli-nro**. The workflow also supports manual runs.

Run `bash tools/tests/run.sh` for host regression checks (C++20; FFmpeg is optional for a generated AAC fixture). Run `bash tools/tests/subsonic.sh` for local mock-server checks (Python 3, curl, libxml2 and FFmpeg development libraries through `pkg-config`).

The Windows UI preview is a development tool, with Subsonic disabled. Build it with `tools/preview/build.sh`; its output is `doli-preview.exe`. `GF_ROOT` selects a fake SD card, `GF_SHOW=1` opens its window, and `GF_SCRIPT` supplies scripted input. These existing environment-variable names remain compatible.

Brand assets are generated with Pillow: `tools/brand/wordmark.py` writes the app's wordmark, `tools/brand/icon.py` writes the launcher icon, and `tools/brand/wordmark-png.py` writes README wordmarks (also requires numpy).

## Limits and support

- Local MP3, M4A and FLAC are the tested formats. Other decoders exist but are less tested; the background player skips ALAC, Ogg Vorbis and Opus.
- Background AAC sample tables are capped at 256 KiB; very long files exceeding that budget are skipped.
- Firmware coverage is limited. The upstream project reported testing on HOS 22.5.0 and 23.0.0 with HATS; this is not a guarantee for other setups.
- Subsonic support has been exercised on a real Switch; network latency and server transcoding affect startup time.
- Report issues in [this fork's issue tracker](../../issues). Normal builds have no embedded report key and cannot submit reports from the console.

## Credits and licences

Doli preserves the original project's copyright and licence notices.

- **[Songate](https://github.com/AndryTheBeast/Songate), by [AndryTheBeast](https://github.com/AndryTheBeast):** the original native Switch app, background player, overlay and tools. MIT (`LICENSE`). Doli is maintained by [D35P4C1T0](https://github.com/D35P4C1T0).
- **[Gatefold](https://github.com/DEVIROYrus/gatefold-foobar2000), by [DEVIROYrus](https://github.com/DEVIROYrus):** the foobar2000 interface Songate recreated, with its design, palette algorithm and interface icons. © Gatefold contributors, MIT (`LICENSE-gatefold`).
- **Inter:** © The Inter Project Authors, SIL Open Font Licence (`romfs/fonts/OFL.txt`). **Fraunces:** Doli's wordmark, SIL Open Font Licence (`tools/brand/OFL.txt`).
- **FFmpeg** (LGPL 2.1), **SDL 2** with SDL_mixer, SDL_ttf and SDL_image (zlib), **libFLAC**, **mpg123**, **Opus**, **Tremor**, **libjpeg-turbo**, **libcurl**, **libxml2** and **zlib**, through devkitPro's portlibs. **stb_image** (public domain / MIT).
- **libnx and devkitPro:** the Switch homebrew toolchain (ISC).
- **Background player:** Helix fixed-point AAC decoder © RealNetworks, RPSL (`sysmodule/lib/helix-aac`); minimp3 (CC0) and dr_flac (public domain / MIT-0), in `sysmodule/lib/single`.
- **Overlay:** GPL 2, built on sys-tune's overlay © HookedBehemoth and ppkantorski (`overlay/LICENSE-sys-tune`), with libultrahand © ppkantorski and libtesla © WerWolv (`overlay/lib/libultrahand`).
- Historical screenshots and tour footage use music from [NCS](https://ncs.io). No music is included with Doli.

Doli is unofficial homebrew, with no affiliation to Nintendo or foobar2000.
