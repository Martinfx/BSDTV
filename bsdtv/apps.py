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

    def command(self, variables):
        """Return the argv to run, with {placeholders} filled from ``variables``."""
        return [expand(arg, variables) for arg in self.exec]


def expand(text, variables):
    def repl(match):
        value = variables.get(match.group(1))
        return os.path.expanduser(str(value)) if value is not None else match.group(0)

    return _VAR.sub(repl, text)


def parse_apps(data):
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
            name=raw.get("name", raw["id"]),
            exec=list(raw["exec"]),
            caption=raw.get("caption", ""),
            color=raw.get("color", "#3a3f55"),
            icon=raw.get("icon", ""),
            pkg=raw.get("pkg", ""),
            requires=list(raw.get("requires", [])),
        ))
    return apps


def is_installed(app, variables, which=shutil.which):
    """An app is usable when its binary exists and every required variable is set."""
    if not app.exec or not which(app.exec[0]):
        return False
    return all(variables.get(name) for name in app.requires)


def load_apps(cfg, which=shutil.which):
    """Return installed, non-hidden apps in catalogue order (system apps.json merged with the user's)."""
    data = {"apps": []}
    for path in (config.find_data_file("apps.json"), config.user_dir() / "apps.json"):
        if path:
            # User entries come first so they win over system entries with the same id.
            data["apps"] = config.read_json(path).get("apps", []) + data["apps"]
    variables = cfg.get("vars", {})
    hidden = set(cfg.get("hidden", []))
    return [a for a in parse_apps(data) if a.id not in hidden and is_installed(a, variables, which)]


def order_dock(apps, dock_ids):
    by_id = {a.id: a for a in apps}
    return [by_id[i] for i in dock_ids if i in by_id]
