# BSDTV

**A smart TV that doesn't spy on you, built on FreeBSD.**

BSDTV turns any small PC (a NUC, a mini PC, an old laptop) running FreeBSD into
a clean, ad-free TV box: a calm Apple TV–style home screen, your apps, a remote
on your phone, HDMI-CEC for the TV's own remote, and optional network-wide ad
blocking. No accounts, no telemetry, no content rows paid for by someone else.

[Česky](README.cs.md)

![Home screen](docs/img/j-home.png)

Inspired by [CleanYourTV](https://github.com/Gathin23/cleanyourtv), which
debloats Android TVs. BSDTV takes the other route: instead of cleaning up a
TV's operating system, it brings its own, starting from a FreeBSD base that has
nothing to remove in the first place.

## What you get

- **Home screen in the style of today's smart TVs** (Google TV and the like):
  "For you" and "Apps" tabs, a full-width rotating banner of featured apps,
  then rows of cards (streaming, movies & music, games) and "Your apps" as
  round icons. Dark graphite colours with flat tiles in each app's brand
  colours. Made for a remote: arrows, OK, Back, Home. Five themes. A simpler "classic" home (clock, dock
  and grid) is one switch away in Settings.
- **Your own films from Jellyfin**: "Continue watching" and "Recently added"
  rows with artwork and progress bars, film backdrops in the banner, and a
  details page with "Watch on Jellyfin" (or play straight away in mpv).
- **Profiles**: "Who's watching?" with round avatars; each profile can have its
  own favourite apps, theme, language and Jellyfin user.
- **Search** across apps and the Jellyfin library, with an on-screen keyboard
  for the remote (or type on the phone remote).
- **Ambient mode**: after a while without input, a slideshow of your photos
  (or film backdrops) with a big clock. Never covers a running app.
- **English and Czech**, following the system language or chosen in Settings.
- **Apps**: Kodi, YouTube (the TV interface, with uBlock Origin), Jellyfin, a
  web browser, IPTV and a music folder in mpv, Moonlight game streaming,
  RetroArch. Only installed apps are shown; adding your own is a JSON entry.
- **Netflix, Disney+, Prime Video, Max, Spotify** (optional, `--drm`): through
  the Linux build of Brave with Widevine, running on FreeBSD's Linux
  compatibility layer. Brave Shields block ads there too.
- **One app at a time**, like a real TV: Home closes the running app and
  brings back the home screen.
- **Phone remote**: open a URL on your phone (paired with a code shown on the
  TV) for a D-pad, media keys, volume, typing into search boxes and app
  shortcuts. No app to install.
- **HDMI-CEC**: use the TV's own remote through a USB-CEC adapter (libcec).
- **Ad blocking for the whole home** (optional): FreeBSD's `local_unbound`
  with public block lists, refreshed weekly.
- **Boots straight into the TV**: autologin on the console, no display manager,
  boot menu delay cut to 2 seconds, quiet boot.
- **Reversible**: every change is recorded in `/var/db/bsdtv`;
  `uninstall.sh` puts the system back.

| Continue watching | Details | Search |
| --- | --- | --- |
| ![Continue watching](docs/img/j-continue.png) | ![Details](docs/img/j-details.png) | ![Search](docs/img/j-search.png) |
| **Profiles** | **Ambient mode** | **Classic home** |
| ![Profiles](docs/img/j-profiles.png) | ![Ambient](docs/img/j-ambient.png) | ![Classic](docs/img/home.png) |

## Install

You need a 64-bit PC with FreeBSD 14 or newer installed (a minimal install is
enough; see [docs/HARDWARE.md](docs/HARDWARE.md) for what works well). Then, as
root:

```sh
pkg install -y git
git clone https://github.com/martinfx/BSDTV.git
cd BSDTV
sh install.sh            # add --adblock, --ssh, --apps "kodi firefox mpv moonlight-qt"
shutdown -r now
```

The installer asks once before it changes anything, then installs the packages
and compiles BSDTV (a few minutes on a mini PC). Run `sh install.sh --dry-run`
first if you want to see every step.

| Option | Meaning |
| --- | --- |
| `--apps "LIST"` | app packages to install (default `kodi firefox mpv`) |
| `--gpu TYPE` | `auto`, `intel`, `amd`, `nvidia` or `generic` |
| `--adblock` | network-wide ad blocking with `local_unbound` |
| `--drm` | streaming services that need Widevine, via `linux-brave` (see below) |
| `--ssh` | enable sshd for remote administration |
| `--audio-unit N` | send sound to `pcmN` (HDMI is often not unit 0, see `cat /dev/sndstat`) |
| `--user NAME` | account that runs the TV (default `tv`) |

### Let an AI assistant do it

Like CleanYourTV, BSDTV is easy to set up with a coding assistant that has
shell access (Claude Code, Codex, Gemini CLI). Enable SSH on the box, then paste
[PROMPT.md](PROMPT.md) into the assistant on your computer.

## Using it

| Remote / keyboard | Home screen | In an app |
| --- | --- | --- |
| Arrows, OK (Enter) | move, open | passed to the app |
| Back (Esc) | back to the tabs, close panels | passed to the app |
| Home (`XF86HomePage`, `F12`, `Super+H`) | – | close the app, back home |
| Menu | settings | passed to the app |
| Volume keys | volume (via `mixer`) | volume |

Phone remote: open **Settings** on the TV and visit the address shown there
from a phone on the same network. The `?t=` part is the pairing code.

From a shell (as the `tv` user or over SSH):

```sh
bsdtv-ctl apps                # list installed apps
bsdtv-ctl launch kodi
bsdtv-ctl key down down ok
bsdtv-ctl text "nature documentaries"
bsdtv-ctl home
bsdtv-ctl theme aurora
bsdtv-ctl url                 # phone remote address
```

## Configuration

- `/usr/local/etc/bsdtv/config.json`: system defaults (dock, theme, Jellyfin
  URL, IPTV playlist, music folder, CEC, remote port).
- `~tv/.config/bsdtv/config.json`: per-user overrides; the Settings panel
  writes here.
- `/usr/local/etc/bsdtv/apps.json` and `~tv/.config/bsdtv/apps.json`: the
  app catalogue.
- `/usr/local/etc/bsdtv/themes/<name>/theme.json`: themes (colours and an
  optional wallpaper).

To show your Jellyfin library, create an API key in Jellyfin (Dashboard → API
Keys) and add it:

```json
"jellyfin": {"url": "http://192.168.1.10:8096", "api_key": "…", "user": "martin"}
```

See [docs/CONFIG.md](docs/CONFIG.md) for every option, profiles, ambient mode, adding apps and making
themes.

## How it works

```
 FreeBSD boot ─▶ getty autologin (ttyv0) ─▶ startx ─▶ openbox (invisible WM)
                                                   └▶ bsdtv-session ─▶ bsdtv launcher (Qt 6: C++ + QML)
                                                                         ├─ starts apps in their own process group
                                                                         ├─ HTTP API + phone remote on :8765
                                                                         └─ bsdtv-cec ─ libcec ─ TV remote
```

- `qml/`: the whole user interface (Qt Quick).
- `src/`: the C++ side. `backend.cpp` is the `TV` singleton QML talks to (apps,
  settings, profiles, Jellyfin rows, search, ambient mode); `apprunner.cpp`
  keeps one app in the foreground; `remoteserver.cpp` serves the phone remote
  and the JSON API; `jellyfin.cpp` talks to Jellyfin; `ctl.cpp` and `cec.cpp`
  are `bsdtv-ctl` and the HDMI-CEC bridge. Everything is one binary, `bsdtv`.
- `web/remote.html`: the phone remote page (built into the binary).
- `i18n/`: translations.
- `system/`: session files (xinitrc, openbox config, autologin, Firefox policies).
- `tools/adblock-update.sh`: builds the unbound block list.
- `install.sh` / `uninstall.sh`: setup and undo.

## Development

BSDTV builds with Qt 6.8 or newer and CMake on FreeBSD, Linux and macOS, which
is handy for working on the UI:

```sh
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.3/gcc_64   # if Qt is not system-wide
cmake --build build
./build/bsdtv --windowed          # arrow keys + Enter + Esc; F12 (Home) only works under openbox
ctest --test-dir build            # unit tests and offscreen UI tests
```

On FreeBSD: `pkg install qt6-base qt6-declarative qt6-tools cmake-core ninja`.
A build run from the source tree finds `themes/` and `etc/` there.

Apps only appear when their binary is on `PATH`, so on a dev machine you may see
an empty grid until you install e.g. `mpv` or `firefox`.

## Security notes

- The phone remote listens on all interfaces (port 8765). LAN clients must
  present the pairing code; localhost is trusted. Set
  `"remote": {"bind": "127.0.0.1"}` to turn LAN access off, or
  `"enabled": false` to disable it.
- The `tv` account has no password and can only log in on the console. Its
  only root privilege is `doas /sbin/shutdown`.
- To change the pairing code, delete `~tv/.config/bsdtv/remote-token` and
  restart the home screen.
- A Jellyfin API key has admin rights. Put it in
  `~tv/.config/bsdtv/config.json` (`chmod 600`), not in the world-readable
  `/usr/local/etc/bsdtv/config.json`. "Play now (mpv)" passes it in the stream
  URL, so it is visible in the process list while a film plays.

## Status and roadmap

BSDTV is young. Ideas for what comes next:

- prebuilt USB image (poudriere / `release(7)`), so no FreeBSD install is needed
- more translations (`i18n/`, see docs/CONFIG.md)
- a FreeBSD port, so `pkg install bsdtv` replaces building in `install.sh`
- app icons; the same "continue watching" rows for Kodi libraries
- playback progress sync for "Play now (mpv)" (the Jellyfin web app already syncs)
- Wayland session (cage or labwc) once FreeBSD's Wayland apps are mature enough
- Bluetooth remotes and game controllers out of the box

## Netflix and other DRM services

Netflix, Disney+, Prime Video, Max and Spotify only play in a browser with the
Widevine CDM. Firefox on FreeBSD has none, but the Linux build of Brave
(`linux-brave`, running on the Linuxulator) can load it from the
`www/linux-widevine-cdm` port. `install.sh --drm` sets this up:

1. enables Linux compatibility (`linux_enable=YES`)
2. installs `linux-brave`
3. builds `www/linux-widevine-cdm` from the ports tree. It has no binary
   package because Widevine may not be redistributed, so get the ports tree
   first: `git clone --depth 1 https://git.FreeBSD.org/ports.git /usr/ports`
4. installs `bsdtv-drm-browser` (Brave in kiosk mode with its own profile) and
   sets `vars.drm_browser`, which makes the streaming tiles appear

Limits: this is software Widevine (L3), so services stream in reduced
resolution (Netflix in a browser typically tops out at 720p, no 4K or HDR).
It depends on Linux binaries that FreeBSD does not support officially, so a
Brave or Widevine update can break it. If Brave will not start, try
`BSDTV_BRAVE_FLAGS=--no-sandbox` in `~tv/.xinitrc` (this weakens Brave's
sandbox).

## License

BSD 2-Clause, see [LICENSE](LICENSE).
