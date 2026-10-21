"""Loads the real QML home screen offscreen and drives it with key presses (tests run in order)."""

import os

import pytest

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
QtCore = pytest.importorskip("PySide6.QtCore")
QtGui = pytest.importorskip("PySide6.QtGui")

from PySide6.QtCore import Qt, qInstallMessageHandler  # noqa: E402
from PySide6.QtTest import QTest  # noqa: E402

INSTALLED = ("kodi", "firefox", "moonlight")


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
                        lambda cfg, lang="en": real_load(cfg, lang=lang,
                                                         which=lambda n: f"/bin/{n}" if n in INSTALLED else None))
    started = []
    monkeypatch.setattr("bsdtv.runner.AppRunner.start", lambda self, app_id, argv: started.append(app_id))

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


def focused(window):
    item = window.activeFocusItem()
    data = item.property("modelData")
    if data:
        return data["name"]
    for prop in ("name", "label", "text"):
        if item.property(prop):
            return item.property(prop)
    return item.metaObject().className()


def press(window, *keys):
    for key in keys:
        QTest.keyClick(window, key)
    QTest.qWait(50)


def test_tv_home_loads_without_warnings(ui):
    window, backend, _, messages = ui
    assert backend.settings["layout"] == "tv"
    assert "HeroBanner" in focused(window)
    assert [m for m in messages if "qml" in m.lower()] == []


def test_banner_rows_and_launch(ui):
    window, _, started, _ = ui
    press(window, Qt.Key_Return)
    assert started == ["kodi"]  # first featured slide

    press(window, Qt.Key_Down, Qt.Key_Right)
    assert focused(window) == "YouTube"  # "Your apps" row
    press(window, Qt.Key_Down)
    assert focused(window) == "YouTube"  # "Streaming" row
    press(window, Qt.Key_Down, Qt.Key_Down)
    assert focused(window) == "Moonlight"  # "Movies & music" then "Games"
    press(window, Qt.Key_Return)
    assert started[-1] == "moonlight"


def test_back_goes_to_tabs_and_apps_tab(ui):
    window, _, started, _ = ui
    press(window, Qt.Key_Escape)
    assert focused(window) == "For you"
    press(window, Qt.Key_Right, Qt.Key_Down)
    assert focused(window) == "Kodi"  # Apps tab grid
    press(window, Qt.Key_Right, Qt.Key_Return)
    assert started[-1] == "youtube"


def test_settings_switch_to_classic_layout(ui):
    window, backend, _, _ = ui
    press(window, Qt.Key_Escape, Qt.Key_Right, Qt.Key_Return)
    assert focused(window) == "Theme"
    press(window, Qt.Key_Down)
    assert focused(window) == "Home screen"
    press(window, Qt.Key_Right)
    assert backend.settings["layout"] == "classic"
    press(window, Qt.Key_Escape)
    from PySide6.QtQuick import QQuickItem
    home = window.findChild(QQuickItem, "home").property("item")
    assert home.metaObject().className().startswith("HomeScreen")


def test_classic_layout_navigation(ui):
    window, _, started, _ = ui
    from PySide6.QtQuick import QQuickItem
    home = window.findChild(QQuickItem, "home").property("item")
    assert focused(window) == "Kodi"  # the classic home starts on the dock
    press(window, Qt.Key_Right)
    assert focused(window) == "YouTube"
    assert home.property("focusedName") == "YouTube"
    press(window, Qt.Key_Return)
    assert started[-1] == "youtube"
