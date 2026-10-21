"""Loads the real QML home screen offscreen and drives it with key presses (tests run in order)."""

import os

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
QtCore = pytest.importorskip("PySide6.QtCore")
QtGui = pytest.importorskip("PySide6.QtGui")

from PySide6.QtCore import Qt, qInstallMessageHandler  # noqa: E402
from PySide6.QtTest import QTest  # noqa: E402


@pytest.fixture(scope="module")
def qapp():
    app = QtGui.QGuiApplication.instance() or QtGui.QGuiApplication([])
    yield app


@pytest.fixture(scope="module")
def ui(qapp):
    # One engine per process: QML singletons can only be registered once.
    monkeypatch = pytest.MonkeyPatch()
    import shiboken6
    from PySide6.QtQml import QQmlApplicationEngine, qmlRegisterSingletonInstance
    from PySide6.QtQuick import QQuickWindow

    from bsdtv import apps
    from bsdtv.backend import Backend

    real_load = apps.load_apps
    monkeypatch.setattr(apps, "load_apps",
                        lambda cfg: real_load(cfg, which=lambda n: f"/bin/{n}" if n in ("kodi", "firefox") else None))
    started = []
    monkeypatch.setattr("bsdtv.runner.AppRunner.start", lambda self, app_id, argv: started.append(argv))

    messages = []
    qInstallMessageHandler(lambda mode, ctx, msg: messages.append(msg))
    backend = Backend()
    qmlRegisterSingletonInstance(Backend, "BSDTV", 1, 0, "TV", backend)
    engine = QQmlApplicationEngine()
    engine.setInitialProperties({"windowed": True})
    engine.load(os.path.join(os.path.dirname(__file__), "..", "bsdtv", "qml", "Main.qml"))
    assert engine.rootObjects(), messages
    window = shiboken6.wrapInstance(shiboken6.getCppPointer(engine.rootObjects()[0])[0], QQuickWindow)
    QTest.qWait(200)
    yield window, backend, started, messages
    qInstallMessageHandler(None)
    window.close()
    backend.shutdown()
    monkeypatch.undo()


def focused_name(window):
    item = window.activeFocusItem()
    return item.property("name") or item.property("label") or item.property("text")


def test_home_screen_loads_without_warnings(ui):
    window, _, _, messages = ui
    assert focused_name(window) == "Kodi"
    assert [m for m in messages if "qml" in m.lower()] == []


def shelf_title(window):
    from PySide6.QtQuick import QQuickItem
    return window.findChild(QQuickItem, "home").property("focusedName")


def test_arrow_keys_and_launch(ui):
    window, _, started, _ = ui
    QTest.keyClick(window, Qt.Key_Right)
    assert focused_name(window) == "YouTube"
    assert shelf_title(window) == "YouTube"
    QTest.keyClick(window, Qt.Key_Return)
    assert started and started[0][0] == "firefox"


def test_settings_change_theme_and_close(ui):
    window, backend, _, _ = ui
    QTest.keyClick(window, Qt.Key_Up)
    assert focused_name(window) == "Settings"
    QTest.keyClick(window, Qt.Key_Return)
    QTest.qWait(50)
    assert focused_name(window) == "Theme"
    before = backend.settings["theme"]
    QTest.keyClick(window, Qt.Key_Right)
    assert backend.settings["theme"] != before
    QTest.keyClick(window, Qt.Key_Escape)
    assert focused_name(window) == "Settings"
