# Configuration

BSDTV reads, in order (later wins):

1. built-in defaults (`bsdtv/config.py`)
2. `/usr/local/etc/bsdtv/config.json`
3. `~tv/.config/bsdtv/config.json` (the Settings panel writes here)

Nested objects (`vars`, `remote`, `cec`) are merged key by key; everything else
is replaced. After editing by hand, run `bsdtv-ctl reload`.

## config.json

```json
{
  "theme": "midnight",
  "layout": "tv",
  "language": "auto",
  "featured": [
    {"app": "jellyfin", "title": "Movie night", "tagline": "New on your server", "image": "/home/tv/banner.jpg"},
    {"app": "youtube"}
  ],
  "dock": ["kodi", "youtube", "jellyfin", "browser"],
  "hidden": [],
  "columns": 5,
  "reduced_motion": false,
  "show_stats": false,
  "greeting": true,
  "clock_24h": true,
  "fullscreen": true,
  "remote": {"enabled": true, "bind": "0.0.0.0", "port": 8765},
  "cec": {"enabled": false, "device": ""},
  "vars": {
    "jellyfin_url": "http://192.168.1.10:8096",
    "iptv_playlist": "https://example.com/my-playlist.m3u",
    "music_dir": "/home/tv/Music",
    "firefox_profile": "~/.local/share/bsdtv/firefox",
    "youtube_profile": "~/.local/share/bsdtv/firefox-youtube",
    "youtube_url": "https://www.youtube.com/tv",
    "drm_browser": "/usr/local/bin/bsdtv-drm-browser"
  }
}
```

| Key | Meaning |
| --- | --- |
| `theme` | folder name under `themes/` |
| `layout` | `tv`: tabs, banner and rows (default); `classic`: clock, dock and grid |
| `language` | `auto` (system language), `cs` or `en` |
| `featured` | banner slides: `app` plus optional `title`, `tagline`, `image`, `color`; empty = the `dock` apps |
| `dock` | favourite apps: first in "Your apps" (and the dock in the classic layout) |
| `hidden` | app ids never shown |
| `columns` | tiles per row, 3 to 7 |
| `reduced_motion` | no animations |
| `show_stats` | CPU temperature, free RAM and disk in the corner |
| `greeting`, `clock_24h` | clock area |
| `fullscreen` | `false` runs in a window (debugging) |
| `remote.bind` / `remote.port` | phone remote; `127.0.0.1` makes it local only |
| `cec.enabled` / `cec.device` | start the HDMI-CEC bridge; empty device = autodetect |
| `vars` | values for `{placeholders}` in `apps.json` |
| `vars.drm_browser` | browser with Widevine for Netflix & co.; empty hides those tiles. `install.sh --drm` sets it |

## Apps

An app is shown when its program is on `PATH` and every variable listed in
`requires` is non-empty. Add your own in `~tv/.config/bsdtv/apps.json`; an
entry with the same `id` as a built-in app replaces it.

```json
{
  "apps": [
    {
      "id": "plex",
      "name": "Plex",
      "caption": "Plex HTPC",
      "color": "#e5a00d",
      "exec": ["firefox", "--kiosk", "--profile", "{firefox_profile}", "https://app.plex.tv/desktop"]
    },
    {
      "id": "radio",
      "name": "Radio",
      "caption": "Czech Radio Vltava",
      "color": "#2d6cdf",
      "exec": ["mpv", "--force-window=yes", "--fs", "https://rozhlas.stream/vltava_high.aac"]
    }
  ]
}
```

| Field | Meaning |
| --- | --- |
| `id` | unique id, used by `dock`, `hidden` and `bsdtv-ctl launch` |
| `name`, `caption` | text on the tile and above the dock; a string or `{"en": "...", "cs": "..."}` |
| `tagline` | banner text (falls back to `caption`); can be translated like `caption` |
| `category` | `streaming`, `media` or `games`: puts the app in that row on the home screen |
| `color` | tile colour and ambient glow |
| `icon` | optional image path (absolute, or a URL) shown instead of the name |
| `exec` | program and arguments; `{name}` is replaced from `vars`, `~` is expanded |
| `requires` | `vars` that must be set for the tile to appear |
| `pkg` | FreeBSD package name (documentation for now) |

Apps run in their own process group. Home sends `SIGTERM` to the whole group
and `SIGKILL` three seconds later, so apps with helper processes (Firefox, Kodi)
close cleanly.

## Themes

A theme is a folder with a `theme.json`:

```json
{
  "name": "Forest",
  "background": "#030805",
  "background2": "#0d2416",
  "surface": "#11281a",
  "text": "#f0fff4",
  "subtext": "#94b8a0",
  "accent": "#5fdc8c",
  "focus": "#ffffff",
  "wallpaper": "wallpaper.jpg"
}
```

Put it in `/usr/local/etc/bsdtv/themes/forest/` and pick it in Settings or with
`bsdtv-ctl theme forest`. `wallpaper` is optional, relative to the theme folder.

## Phone remote API

All endpoints are on port 8765. Requests from localhost need no token; others
need the header `X-BSDTV-Token: <code>` or `?t=<code>`.

| Method | Path | Body |
| --- | --- | --- |
| GET | `/api/apps` | |
| GET | `/api/status` | |
| POST | `/api/launch` | `{"app": "kodi"}` |
| POST | `/api/home` | `{}` |
| POST | `/api/key` | `{"key": "up"}`: up, down, left, right, ok, back, menu, playpause, stop, next, prev, rewind, forward, info, backspace |
| POST | `/api/text` | `{"text": "search words"}` |
| POST | `/api/volume` | `{"action": "up" \| "down" \| "mute"}` |
| POST | `/api/power` | `{"action": "display_off" \| "reboot" \| "poweroff"}` |
| POST | `/api/reload` | `{}` |

Keys are typed with `xdotool`, so they reach whichever app is in front.

## Ad blocking

With `--adblock`, `bsdtv-adblock-update` turns the lists in
`/usr/local/etc/bsdtv/adblock-lists.txt` into
`/var/unbound/conf.d/bsdtv-adblock.conf` every Sunday. Domains in
`adblock-allow.txt` are never blocked. Point your router's DHCP DNS setting at
the TV box to protect every device at home.

## Translations

UI strings live in `bsdtv/i18n/bsdtv_<lang>.ts`. After changing QML text:

```sh
pyside6-lupdate bsdtv/qml/*.qml -ts bsdtv/i18n/bsdtv_cs.ts -no-obsolete
# translate the new entries (Qt Linguist or a text editor), then:
pyside6-lrelease bsdtv/i18n/bsdtv_cs.ts -qm bsdtv/i18n/bsdtv_cs.qm
```

To add a language, create `bsdtv_<lang>.ts` the same way and add the code to
`LANGUAGES` in `bsdtv/backend.py` and to the language list in
`SettingsPanel.qml`.
