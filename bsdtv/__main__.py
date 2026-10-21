"""Entry point: ``python3 -m bsdtv`` starts the TV launcher, ``python3 -m bsdtv ctl ...`` the CLI."""

import argparse
import signal
import sys
from pathlib import Path


def run_launcher(argv):
    parser = argparse.ArgumentParser(prog="bsdtv", description="BSDTV home screen")
    parser.add_argument("--windowed", action="store_true", help="run in a 1280x720 window (development)")
    parser.add_argument("--no-remote", action="store_true", help="do not start the phone remote server")
    parser.add_argument("--screenshot", metavar="PNG", help="render the home screen to PNG and exit")
    args = parser.parse_args(argv)

    from PySide6.QtCore import QTimer
    from PySide6.QtGui import QFontDatabase, QGuiApplication
    import shiboken6
    from PySide6.QtQml import QQmlApplicationEngine, qmlRegisterSingletonInstance
    from PySide6.QtQuick import QQuickWindow

    from .backend import Backend

    app = QGuiApplication(sys.argv[:1])
    app.setApplicationName("BSDTV")
    app.setOrganizationName("BSDTV")
    fonts = Path(__file__).resolve().parent / "fonts"
    for font in sorted(fonts.glob("*.ttf")) if fonts.is_dir() else []:
        QFontDatabase.addApplicationFont(str(font))

    backend = Backend()
    if not args.no_remote and not args.screenshot:
        backend.start_remote()

    qmlRegisterSingletonInstance(Backend, "BSDTV", 1, 0, "TV", backend)
    engine = QQmlApplicationEngine()
    engine.setInitialProperties({
        "windowed": bool(args.windowed or args.screenshot or not backend.settings.get("fullscreen", True)),
    })
    engine.load(Path(__file__).resolve().parent / "qml" / "Main.qml")
    if not engine.rootObjects():
        return 1
    # Re-wrap the root as a QQuickWindow (rootObjects() only gives a QWindow wrapper).
    window = shiboken6.wrapInstance(shiboken6.getCppPointer(engine.rootObjects()[0])[0], QQuickWindow)

    def bring_to_front():
        window.raise_()
        window.requestActivate()

    backend.raiseRequested.connect(bring_to_front)
    app.aboutToQuit.connect(backend.shutdown)

    # Let Ctrl+C / SIGTERM from the session script stop the Qt loop cleanly.
    signal.signal(signal.SIGINT, lambda *_: app.quit())
    signal.signal(signal.SIGTERM, lambda *_: app.quit())
    tick = QTimer()
    tick.start(250)
    tick.timeout.connect(lambda: None)

    if args.screenshot:
        def grab():
            try:
                window.grabWindow().save(args.screenshot)
            finally:
                app.quit()
        QTimer.singleShot(1500, grab)

    return app.exec()


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    if argv and argv[0] == "ctl":
        from .ctl import main as ctl_main
        return ctl_main(argv[1:])
    if argv and argv[0] == "cec":
        from .cec import main as cec_main
        return cec_main(argv[1:])
    return run_launcher(argv)


if __name__ == "__main__":
    sys.exit(main())
