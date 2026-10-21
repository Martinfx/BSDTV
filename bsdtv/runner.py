"""Runs one foreground app at a time, like a TV does."""

import os
import signal
import subprocess

from PySide6.QtCore import QObject, QTimer, Signal


class AppRunner(QObject):
    runningChanged = Signal(str)
    failed = Signal(str, str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._process = None
        self._app_id = ""
        self._watch = QTimer(self)
        self._watch.setInterval(500)
        self._watch.timeout.connect(self._poll)

    @property
    def app_id(self):
        return self._app_id

    def start(self, app_id, argv):
        self.stop()
        try:
            # Own session/process group, so stopping the app also stops its helpers
            # (Firefox content processes, Kodi add-ons, ...).
            self._process = subprocess.Popen(argv, start_new_session=True, stdin=subprocess.DEVNULL)
        except OSError as exc:
            self.failed.emit(app_id, str(exc))
            return
        self._app_id = app_id
        self._watch.start()
        self.runningChanged.emit(app_id)

    def stop(self):
        process = self._process
        if process is None:
            return
        self._clear()
        if process.poll() is None:
            self._signal_group(process.pid, signal.SIGTERM)
            # Anything left in the group after the grace period gets killed.
            QTimer.singleShot(3000, lambda: self._reap(process))

    def _reap(self, process):
        self._signal_group(process.pid, signal.SIGKILL)
        try:
            process.wait(timeout=1)
        except subprocess.TimeoutExpired:
            pass

    @staticmethod
    def _signal_group(pid, sig):
        try:
            os.killpg(pid, sig)
        except (ProcessLookupError, PermissionError):
            pass

    def _poll(self):
        if self._process is not None and self._process.poll() is not None:
            self._clear()

    def _clear(self):
        self._process = None
        self._app_id = ""
        self._watch.stop()
        self.runningChanged.emit("")
