"""The QML-facing backend: app models, settings, theme and the remote controller."""

import os
import random
import shutil
import threading
from pathlib import Path

from PySide6.QtCore import (QAbstractListModel, QByteArray, QCoreApplication, QLocale, QModelIndex,
                            QObject, Property, QTimer, QUrl, Qt, Signal, Slot)

from . import __version__, apps, config, jellyfin, remote, system
from .runner import AppRunner

PHOTO_SUFFIXES = {".jpg", ".jpeg", ".png", ".webp"}
MAX_PHOTOS = 300


def find_photos(folder):
    """Shuffled image URLs from a folder tree, for the ambient screen."""
    if not folder:
        return []
    root = Path(os.path.expanduser(folder))
    found = []
    if root.is_dir():
        for dirpath, _dirs, files in os.walk(root):
            found.extend(Path(dirpath) / f for f in files if Path(f).suffix.lower() in PHOTO_SUFFIXES)
            if len(found) > MAX_PHOTOS * 4:
                break
    random.shuffle(found)
    return [QUrl.fromLocalFile(str(p)).toString() for p in found[:MAX_PHOTOS]]


LANGUAGES = ("en", "cs")


def resolve_language(setting):
    """"auto" follows the system locale; anything we have no translation for falls back to English."""
    lang = setting if setting and setting != "auto" else QLocale.system().name().split("_")[0]
    return lang if lang in LANGUAGES else "en"


class AppModel(QAbstractListModel):
    ROLES = {
        Qt.ItemDataRole.UserRole + 1: b"appId",
        Qt.ItemDataRole.UserRole + 2: b"name",
        Qt.ItemDataRole.UserRole + 3: b"caption",
        Qt.ItemDataRole.UserRole + 4: b"color",
        Qt.ItemDataRole.UserRole + 5: b"icon",
        Qt.ItemDataRole.UserRole + 6: b"textColor",
    }

    def __init__(self, parent=None):
        super().__init__(parent)
        self._apps = []

    def set_apps(self, items):
        self.beginResetModel()
        self._apps = list(items)
        self.endResetModel()

    def rowCount(self, parent=QModelIndex()):
        return 0 if parent.isValid() else len(self._apps)

    def roleNames(self):
        return {role: QByteArray(name) for role, name in self.ROLES.items()}

    def data(self, index, role=Qt.ItemDataRole.DisplayRole):
        if not index.isValid() or not 0 <= index.row() < len(self._apps):
            return None
        name = self.ROLES.get(role)
        if name is None:
            return None
        app = self._apps[index.row()]
        return {b"appId": app.id, b"name": app.name, b"caption": app.caption,
                b"color": app.color, b"icon": app.icon, b"textColor": app.text_color}[name]


class _RemoteBridge(QObject, remote.Controller):
    """Runs remote requests (which arrive on the HTTP thread) on the Qt thread."""

    call = Signal(object)

    def __init__(self, backend):
        super().__init__(backend)
        self._backend = backend
        self.call.connect(lambda fn: fn(), Qt.ConnectionType.QueuedConnection)

    def apps(self):
        return [{"id": a.id, "name": a.name, "color": a.color} for a in self._backend.app_list]

    def status(self):
        return {"running": self._backend.runner.app_id, "version": __version__}

    def launch(self, app_id):
        if app_id not in {a.id for a in self._backend.app_list}:
            return False
        self.call.emit(lambda: self._backend.launch(app_id))
        return True

    def home(self):
        self.call.emit(self._backend.home)
        return True

    def key(self, name):
        return system.send_key(name)

    def text(self, value):
        return bool(value) and system.send_text(value)

    def volume(self, action):
        return system.volume(action)

    def power(self, action):
        if action not in system.POWER_COMMANDS:
            return False
        self.call.emit(lambda: self._backend.power(action))
        return True

    def reload(self):
        self.call.emit(self._backend.reload)
        return True


class Backend(QObject):
    settingsChanged = Signal()
    themeChanged = Signal()
    runningAppChanged = Signal()
    infoChanged = Signal()
    raiseRequested = Signal()
    languageChanged = Signal(str)
    message = Signal(str)
    rowsChanged = Signal()
    mediaRequested = Signal("QVariantMap")
    searchChanged = Signal()
    ambientChanged = Signal()
    # Results of background Jellyfin requests: (generation, payload).
    _mediaFetched = Signal(int, object)
    _searchFetched = Signal(int, object)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.runner = AppRunner(self)
        self.runner.runningChanged.connect(self._on_running_changed)
        self.runner.failed.connect(lambda app_id, err: self.message.emit(f"{app_id}: {err}"))
        self._apps_model = AppModel(self)
        self._dock_model = AppModel(self)
        self._all_model = AppModel(self)
        self.app_list = []
        self._rows = []
        self._app_rows = []
        self._featured = []
        self._app_featured = []
        self._media = {"resume": [], "latest": []}
        self._search = {"term": "", "apps": [], "media": [], "busy": False}
        self._jellyfin = None
        self._generation = 0
        self._search_generation = 0
        self._ambient = False
        self._photos = []
        self._lang = ""
        self._config = {}
        self._theme = {}
        self._ip = ""
        self._stats = ""
        self.token = remote.load_token()
        self.bridge = _RemoteBridge(self)
        self._server = None
        self._mediaFetched.connect(self._on_media, Qt.ConnectionType.QueuedConnection)
        self._searchFetched.connect(self._on_search, Qt.ConnectionType.QueuedConnection)

        self._idle = QTimer(self)
        self._idle.setSingleShot(True)
        self._idle.timeout.connect(self._on_idle)
        self._media_timer = QTimer(self)
        self._media_timer.setInterval(5 * 60_000)
        self._media_timer.timeout.connect(self.refreshMedia)
        self._media_timer.start()

        self.reload()

        self._info_timer = QTimer(self)
        self._info_timer.setInterval(30_000)
        self._info_timer.timeout.connect(self._refresh_info)
        self._info_timer.start()
        self._refresh_info()

    # --- lifecycle -------------------------------------------------------

    def start_remote(self):
        opts = self._config.get("remote", {})
        if not opts.get("enabled", True):
            return
        try:
            self._server = remote.RemoteServer(self.bridge, self.token,
                                               opts.get("bind", "0.0.0.0"),
                                               int(opts.get("port", 8765))).start()
        except OSError as exc:
            print(f"bsdtv: phone remote disabled: {exc}")

    def shutdown(self):
        self.runner.stop()
        if self._server:
            self._server.stop()
            self._server = None

    def _refresh_info(self):
        self._ip = system.ip_address()
        self._stats = system.stats() if self._config.get("show_stats") else ""
        self.infoChanged.emit()

    def _on_running_changed(self, app_id):
        self.runningAppChanged.emit()
        if not app_id:
            self.raiseRequested.emit()
            # Coming back from watching something: progress has changed.
            self.refreshMedia()
            self.touch()

    # --- properties ------------------------------------------------------

    @Property(QObject, constant=True)
    def appModel(self):
        return self._apps_model

    @Property(QObject, constant=True)
    def dockModel(self):
        return self._dock_model

    @Property(QObject, constant=True)
    def allAppsModel(self):
        return self._all_model

    @Property("QVariantMap", notify=settingsChanged)
    def settings(self):
        return self._config

    @Property("QVariantMap", notify=themeChanged)
    def theme(self):
        return self._theme

    @Property("QVariantList", notify=themeChanged)
    def themes(self):
        return [{"id": tid, "name": t.get("name", tid), "accent": t.get("accent"),
                 "background": t.get("background")}
                for tid, t in config.list_themes().items()]

    @Property(str, notify=runningAppChanged)
    def runningApp(self):
        return self.runner.app_id

    @Property(str, notify=infoChanged)
    def ipAddress(self):
        return self._ip

    @Property(str, notify=infoChanged)
    def stats(self):
        return self._stats

    @Property(str, notify=infoChanged)
    def remoteUrl(self):
        opts = self._config.get("remote", {})
        if not opts.get("enabled", True) or not self._ip:
            return ""
        return f"http://{self._ip}:{opts.get('port', 8765)}/?t={self.token}"

    @Property("QVariantList", notify=rowsChanged)
    def rows(self):
        return self._rows

    @Property("QVariantList", notify=rowsChanged)
    def featured(self):
        return self._featured

    @Property("QVariantList", notify=settingsChanged)
    def profiles(self):
        return config.profiles(self._config)

    @Property("QVariantMap", notify=settingsChanged)
    def profile(self):
        return config.active_profile(self._config)

    @Property("QVariantMap", notify=searchChanged)
    def searchResults(self):
        return self._search

    @Property(bool, notify=ambientChanged)
    def ambientActive(self):
        return self._ambient

    @Property("QVariantList", notify=ambientChanged)
    def ambientPhotos(self):
        backdrops = [i["backdrop"] for i in self._media["latest"] + self._media["resume"] if i["backdrop"]]
        return self._photos or list(dict.fromkeys(backdrops))

    @Property(bool, notify=settingsChanged)
    def hasMpv(self):
        return shutil.which("mpv") is not None and self._jellyfin is not None

    @Property(str, notify=settingsChanged)
    def language(self):
        return self._lang

    @Property(str, constant=True)
    def version(self):
        return __version__

    # --- actions ---------------------------------------------------------

    @Slot()
    def reload(self):
        self._config = config.load_config()
        self._theme = config.load_theme(self._config.get("theme", "midnight"))
        lang = resolve_language(self._config.get("language"))
        self.app_list = apps.load_apps(self._config, lang=lang)
        self._app_rows = apps.home_rows(self.app_list, self._config.get("dock", []))
        self._app_featured = apps.featured(self.app_list, self._config)
        client = jellyfin.from_config(self._config)
        identity = (lambda c: c and (c.url, c.api_key, c.user))
        if identity(client) != identity(self._jellyfin):
            self._jellyfin = client
            self._media = {"resume": [], "latest": []}
            self.refreshMedia()
        self._photos = find_photos((self._config.get("ambient") or {}).get("photos"))
        self._update_rows()
        dock = apps.order_dock(self.app_list, self._config.get("dock", []))
        dock_ids = {a.id for a in dock}
        self._dock_model.set_apps(dock)
        self._all_model.set_apps(self.app_list)
        self._apps_model.set_apps([a for a in self.app_list if a.id not in dock_ids])
        self.settingsChanged.emit()
        self.themeChanged.emit()
        if lang != self._lang:
            self._lang = lang
            self.languageChanged.emit(lang)
        self.touch()

    def _update_rows(self):
        self._rows = apps.with_media(self._app_rows, self._media["resume"], self._media["latest"])
        # Explicit "featured" entries win; otherwise films from Jellyfin, then apps.
        media_slides = jellyfin.slides(self._media["latest"] + self._media["resume"])
        explicit = bool(self._config.get("featured"))
        self._featured = self._app_featured if explicit or not media_slides else media_slides
        self.rowsChanged.emit()
        self.ambientChanged.emit()

    # --- Jellyfin --------------------------------------------------------

    def _in_background(self, signal, generation, work):
        def run():
            try:
                result = work()
            except jellyfin.JellyfinError as exc:
                print(f"bsdtv: jellyfin: {exc}")
                result = None
            signal.emit(generation, result)
        threading.Thread(target=run, name="bsdtv-jellyfin", daemon=True).start()

    @Slot()
    def refreshMedia(self):
        client = self._jellyfin
        if client is None:
            if any(self._media.values()):
                self._media = {"resume": [], "latest": []}
                self._update_rows()
            return
        self._generation += 1
        self._in_background(self._mediaFetched, self._generation,
                            lambda: {"resume": client.resume(), "latest": client.latest()})

    def _on_media(self, generation, result):
        if generation == self._generation and result is not None:
            self._media = result
            self._update_rows()

    def _find_media(self, media_id):
        pool = self._media["resume"] + self._media["latest"] + self._search["media"]
        return next((m for m in pool if m["mediaId"] == media_id), None)

    @Slot(str)
    def openMedia(self, media_id):
        item = self._find_media(media_id)
        if item:
            self.mediaRequested.emit(item)

    @Slot(str)
    def watchInJellyfin(self, media_id):
        app = next((a for a in self.app_list if a.id == "jellyfin"), None)
        if app is None or self._jellyfin is None:
            self.message.emit("Jellyfin: no Jellyfin app (set vars.jellyfin_url and install Firefox)")
            return
        variables = dict(self._config.get("vars", {}), jellyfin_url=self._jellyfin.web_url(media_id))
        self.runner.start(app.id, app.command(variables))

    @Slot(str)
    def playMedia(self, media_id):
        item = self._find_media(media_id)
        if item is None or not self.hasMpv:
            return
        argv = ["mpv", "--fs", f"--force-media-title={item['title']}"]
        if item["resumeSeconds"]:
            argv.append(f"--start={item['resumeSeconds']}")
        self.runner.start("jellyfin", argv + [self._jellyfin.stream_url(media_id)])

    # --- search ----------------------------------------------------------

    @Slot(str)
    def search(self, term):
        term = term.strip()
        self._search_generation += 1
        self._search = {"term": term, "apps": apps.search(self.app_list, term), "media": [],
                        "busy": bool(term and self._jellyfin)}
        self.searchChanged.emit()
        if term and self._jellyfin:
            client = self._jellyfin
            self._in_background(self._searchFetched, self._search_generation, lambda: client.search(term))

    def _on_search(self, generation, result):
        if generation == self._search_generation:
            self._search = dict(self._search, media=result or [], busy=False)
            self.searchChanged.emit()

    # --- profiles --------------------------------------------------------

    @Slot(str)
    def setProfile(self, profile_id):
        config.save_user_setting("profile", profile_id)
        self.reload()

    # --- ambient mode ----------------------------------------------------

    @Slot()
    def touch(self):
        """User activity: leave the ambient screen and restart the idle countdown."""
        if self._ambient:
            self._ambient = False
            self.ambientChanged.emit()
        opts = self._config.get("ambient") or {}
        if opts.get("enabled", True):
            self._idle.start(int(max(0.1, float(opts.get("after_minutes", 10))) * 60_000))
        else:
            self._idle.stop()

    @Slot()
    def startAmbient(self):
        if not self._ambient:
            self._ambient = True
            self.ambientChanged.emit()

    def _on_idle(self):
        # Never cover a running app (someone may be watching a film).
        if self.runner.app_id:
            self.touch()
        else:
            self.startAmbient()

    @Slot(str)
    def launch(self, app_id):
        app = next((a for a in self.app_list if a.id == app_id), None)
        if app is None:
            return
        self.runner.start(app.id, app.command(self._config.get("vars", {})))

    @Slot()
    def home(self):
        self.runner.stop()
        self.raiseRequested.emit()

    @Slot(str)
    def power(self, action):
        if action == "restart_launcher":
            # The session script restarts the launcher when it exits.
            QCoreApplication.exit(0)
            return
        if action != "display_off":
            self.runner.stop()
        system.power(action)

    @Slot(str)
    def volume(self, action):
        system.volume(action)

    @Slot(str, "QVariant")
    def setSetting(self, key, value):
        config.save_user_setting(key, value)
        self.reload()
        self._refresh_info()
