"""Configuration loading.

Configuration is layered: built-in defaults, then the system file
(/usr/local/etc/bsdtv/config.json), then the per-user file
(~/.config/bsdtv/config.json). Settings changed from the TV are written to
the user file only, so the system file can be managed by the installer.
"""

import json
import os
from pathlib import Path

PACKAGE_DIR = Path(__file__).resolve().parent
SOURCE_ROOT = PACKAGE_DIR.parent

DEFAULTS = {
    "theme": "midnight",
    "layout": "tv",
    "language": "auto",
    "featured": [],
    "dock": ["kodi", "youtube", "jellyfin", "browser"],
    "hidden": [],
    "columns": 5,
    "fullscreen": True,
    "reduced_motion": False,
    "show_stats": False,
    "greeting": True,
    "clock_24h": True,
    "remote": {"enabled": True, "bind": "0.0.0.0", "port": 8765},
    "cec": {"enabled": False, "device": ""},
    "vars": {
        "jellyfin_url": "",
        "iptv_playlist": "",
        "music_dir": "",
        "firefox_profile": "~/.local/share/bsdtv/firefox",
        "youtube_profile": "~/.local/share/bsdtv/firefox-youtube",
        "youtube_url": "https://www.youtube.com/tv",
        "drm_browser": "",
    },
}


def system_dir():
    return Path(os.environ.get("BSDTV_ETC", "/usr/local/etc/bsdtv"))


def user_dir():
    base = os.environ.get("XDG_CONFIG_HOME") or os.path.expanduser("~/.config")
    return Path(base) / "bsdtv"


def find_data_file(name):
    """Return the first existing path for a data file (system dir, then source tree)."""
    for candidate in (system_dir() / name, SOURCE_ROOT / "etc" / name):
        if candidate.exists():
            return candidate
    return None


def themes_dirs():
    return [system_dir() / "themes", SOURCE_ROOT / "themes"]


def deep_merge(base, override):
    """Merge ``override`` into a copy of ``base``; nested dicts merge, everything else replaces."""
    result = dict(base)
    for key, value in override.items():
        if isinstance(value, dict) and isinstance(result.get(key), dict):
            result[key] = deep_merge(result[key], value)
        else:
            result[key] = value
    return result


def read_json(path):
    try:
        with open(path, encoding="utf-8") as fh:
            data = json.load(fh)
    except FileNotFoundError:
        return {}
    except (OSError, ValueError) as exc:
        print(f"bsdtv: ignoring unreadable {path}: {exc}")
        return {}
    return data if isinstance(data, dict) else {}


def load_config():
    config = DEFAULTS
    for path in (find_data_file("config.json"), user_dir() / "config.json"):
        if path:
            config = deep_merge(config, read_json(path))
    return config


def save_user_setting(key, value):
    """Persist one top-level setting to the user config file."""
    path = user_dir() / "config.json"
    data = read_json(path)
    data[key] = value
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(".tmp")
    with open(tmp, "w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=2)
        fh.write("\n")
    os.replace(tmp, path)


THEME_DEFAULTS = {
    "name": "Midnight",
    "background": "#05060a",
    "background2": "#101320",
    "surface": "#1a1d2b",
    "text": "#f4f5fa",
    "subtext": "#9aa0b4",
    "accent": "#7aa2ff",
    "focus": "#ffffff",
    "wallpaper": "",
}


def list_themes():
    """Return {theme_id: theme_dict} for every theme folder found."""
    themes = {}
    for directory in reversed(themes_dirs()):
        if not directory.is_dir():
            continue
        for entry in sorted(directory.iterdir()):
            theme_file = entry / "theme.json"
            if theme_file.is_file():
                theme = deep_merge(THEME_DEFAULTS, read_json(theme_file))
                wallpaper = theme.get("wallpaper")
                if wallpaper and not os.path.isabs(wallpaper):
                    theme["wallpaper"] = str(entry / wallpaper)
                themes[entry.name] = theme
    return themes


def load_theme(theme_id):
    themes = list_themes()
    return themes.get(theme_id) or themes.get("midnight") or dict(THEME_DEFAULTS)
