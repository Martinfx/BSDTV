"""The QML-facing backend: app models, settings, theme and the remote controller."""

from PySide6.QtCore import (QAbstractListModel, QByteArray, QCoreApplication, QModelIndex, QObject,
                            Property, QTimer, Qt, Signal, Slot)

from . import __version__, apps, config, remote, system
from .runner import AppRunner


class AppModel(QAbstractListModel):
    ROLES = {
        Qt.ItemDataRole.UserRole + 1: b"appId",
        Qt.ItemDataRole.UserRole + 2: b"name",
        Qt.ItemDataRole.UserRole + 3: b"caption",
        Qt.ItemDataRole.UserRole + 4: b"color",
        Qt.ItemDataRole.UserRole + 5: b"icon",
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
                b"color": app.color, b"icon": app.icon}[name]


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
    message = Signal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.runner = AppRunner(self)
        self.runner.runningChanged.connect(self._on_running_changed)
        self.runner.failed.connect(lambda app_id, err: self.message.emit(f"{app_id}: {err}"))
        self._apps_model = AppModel(self)
        self._dock_model = AppModel(self)
        self.app_list = []
        self._config = {}
        self._theme = {}
        self._ip = ""
        self._stats = ""
        self.token = remote.load_token()
        self.bridge = _RemoteBridge(self)
        self._server = None
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

    # --- properties ------------------------------------------------------

    @Property(QObject, constant=True)
    def appModel(self):
        return self._apps_model

    @Property(QObject, constant=True)
    def dockModel(self):
        return self._dock_model

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

    @Property(str, constant=True)
    def version(self):
        return __version__

    # --- actions ---------------------------------------------------------

    @Slot()
    def reload(self):
        self._config = config.load_config()
        self._theme = config.load_theme(self._config.get("theme", "midnight"))
        self.app_list = apps.load_apps(self._config)
        dock = apps.order_dock(self.app_list, self._config.get("dock", []))
        dock_ids = {a.id for a in dock}
        self._dock_model.set_apps(dock)
        self._apps_model.set_apps([a for a in self.app_list if a.id not in dock_ids])
        self.settingsChanged.emit()
        self.themeChanged.emit()

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
