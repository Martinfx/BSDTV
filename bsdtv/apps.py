"""The app catalogue: which apps exist, how to start them and whether they are installed."""

import os
import re
import shutil
from dataclasses import dataclass, field

from . import config

_VAR = re.compile(r"\{([a-z0-9_]+)\}")


@dataclass
class App:
    id: str
    name: str
    exec: list = field(default_factory=list)
    caption: str = ""
    color: str = "#3a3f55"
    icon: str = ""
    pkg: str = ""
    requires: list = field(default_factory=list)
    category: str = ""
    tagline: str = ""

    def as_dict(self):
        return {"appId": self.id, "name": self.name, "caption": self.caption, "color": self.color,
                "icon": self.icon, "tagline": self.tagline or self.caption, "image": ""}

    def command(self, variables):
        """Return the argv to run, with {placeholders} filled from ``variables``."""
        return [expand(arg, variables) for arg in self.exec]


def expand(text, variables):
    def repl(match):
        value = variables.get(match.group(1))
        return os.path.expanduser(str(value)) if value is not None else match.group(0)

    return _VAR.sub(repl, text)


def localized(value, lang):
    """Text fields may be plain strings or {"en": ..., "cs": ...} maps."""
    if isinstance(value, dict):
        return value.get(lang) or value.get("en") or next(iter(value.values()), "")
    return value or ""


def parse_apps(data, lang="en"):
    apps = []
    seen = set()
    for raw in data.get("apps", []):
        if not isinstance(raw, dict) or not raw.get("id") or not raw.get("exec"):
            continue
        if raw["id"] in seen:
            continue
        seen.add(raw["id"])
        apps.append(App(
            id=raw["id"],
            name=localized(raw.get("name"), lang) or raw["id"],
            exec=list(raw["exec"]),
            caption=localized(raw.get("caption"), lang),
            color=raw.get("color", "#3a3f55"),
            icon=raw.get("icon", ""),
            pkg=raw.get("pkg", ""),
            requires=list(raw.get("requires", [])),
            category=raw.get("category", ""),
            tagline=localized(raw.get("tagline"), lang),
        ))
    return apps


def is_installed(app, variables, which=shutil.which):
    """An app is usable when every required variable is set and its program exists."""
    if not app.exec or not all(variables.get(name) for name in app.requires):
        return False
    return bool(which(expand(app.exec[0], variables)))


def load_apps(cfg, which=shutil.which, lang="en"):
    """Return installed, non-hidden apps in catalogue order (system apps.json merged with the user's)."""
    data = {"apps": []}
    for path in (config.find_data_file("apps.json"), config.user_dir() / "apps.json"):
        if path:
            # User entries come first so they win over system entries with the same id.
            data["apps"] = config.read_json(path).get("apps", []) + data["apps"]
    variables = cfg.get("vars", {})
    hidden = set(cfg.get("hidden", []))
    return [a for a in parse_apps(data, lang) if a.id not in hidden and is_installed(a, variables, which)]


def order_dock(apps, dock_ids):
    by_id = {a.id: a for a in apps}
    return [by_id[i] for i in dock_ids if i in by_id]


# Rows on the "For you" page after "Your apps", in this order.
CATEGORY_ROWS = ["streaming", "media", "games"]


def home_rows(apps, dock_ids):
    """Rows for the Google TV-style home: your apps (favourites first), then one row per category."""
    favourites = order_dock(apps, dock_ids)
    rest = [a for a in apps if a not in favourites]
    rows = [{"key": "your_apps", "apps": [a.as_dict() for a in favourites + rest]}] if apps else []
    for category in CATEGORY_ROWS:
        members = [a.as_dict() for a in apps if a.category == category]
        if members:
            rows.append({"key": category, "apps": members})
    return rows


def featured(apps, cfg):
    """Banner slides: explicit "featured" entries from the config, else the favourite apps."""
    by_id = {a.id: a for a in apps}
    slides = []
    for entry in cfg.get("featured") or [{"app": i} for i in cfg.get("dock", [])]:
        app = by_id.get(entry.get("app")) if isinstance(entry, dict) else None
        if app is None:
            continue
        slide = app.as_dict()
        for key in ("title", "tagline", "image", "color"):
            if entry.get(key):
                slide["name" if key == "title" else key] = entry[key]
        slides.append(slide)
    return slides or [a.as_dict() for a in apps[:5]]
