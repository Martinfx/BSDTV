"""A tiny stand-in for a Jellyfin server, for tests and screenshots."""

import json
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse

API_KEY = "test-key"
HOUR = 36_000_000_000

ITEMS = [
    {"Id": "m1", "Type": "Movie", "Name": "Big Buck Bunny", "ProductionYear": 2008, "OfficialRating": "PG",
     "Overview": "A giant rabbit takes revenge on three bullying rodents.", "RunTimeTicks": HOUR // 6,
     "BackdropImageTags": ["b"], "ImageTags": {"Primary": "p"},
     "UserData": {"PlaybackPositionTicks": HOUR // 12}},
    {"Id": "e1", "Type": "Episode", "Name": "The Signal", "SeriesName": "Northern Lights",
     "ParentIndexNumber": 2, "IndexNumber": 4, "Overview": "The crew follows a strange radio signal.",
     "RunTimeTicks": HOUR * 3 // 4, "ImageTags": {"Primary": "p"}, "ParentBackdropItemId": "s1",
     "UserData": {"PlaybackPositionTicks": HOUR // 2}},
    {"Id": "m2", "Type": "Movie", "Name": "Sintel", "ProductionYear": 2010, "OfficialRating": "12",
     "Overview": "A girl searches for her lost dragon.", "RunTimeTicks": HOUR // 4,
     "BackdropImageTags": ["b"], "ImageTags": {"Primary": "p", "Thumb": "t"}, "UserData": {}},
    {"Id": "m3", "Type": "Movie", "Name": "Tears of Steel", "ProductionYear": 2012,
     "Overview": "Warriors and scientists save the world from robots.", "RunTimeTicks": HOUR // 5,
     "BackdropImageTags": ["b"], "ImageTags": {}, "UserData": {}},
]
COLORS = {"m1": "#4f8a3a", "e1": "#2d4d8a", "s1": "#1d3a6e", "m2": "#8a5a2d", "m3": "#5a2d8a"}


def image(item_id, width, height):
    """A PNG with a colour per item, made with Qt so no extra dependency is needed."""
    from PySide6.QtCore import QBuffer, QByteArray, QIODevice, Qt
    from PySide6.QtGui import QColor, QFont, QImage, QLinearGradient, QPainter

    img = QImage(width, height, QImage.Format.Format_RGB32)
    painter = QPainter(img)
    gradient = QLinearGradient(0, 0, width, height)
    base = QColor(COLORS.get(item_id, "#444444"))
    gradient.setColorAt(0, base.lighter(160))
    gradient.setColorAt(1, base.darker(200))
    painter.fillRect(img.rect(), gradient)
    painter.setPen(QColor(255, 255, 255, 40))
    font = QFont()
    font.setPixelSize(height // 3)
    painter.setFont(font)
    painter.drawText(img.rect(), Qt.AlignmentFlag.AlignCenter, item_id.upper())
    painter.end()
    data = QByteArray()
    buffer = QBuffer(data)
    buffer.open(QIODevice.OpenModeFlag.WriteOnly)
    img.save(buffer, "PNG")
    return bytes(data)


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *args):
        pass

    def handle(self):
        try:
            super().handle()
        except (BrokenPipeError, ConnectionResetError):
            pass  # the client gave up on an image, e.g. after a resize

    def _json(self, data, status=200):
        body = json.dumps(data).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        url = urlparse(self.path)
        query = parse_qs(url.query)
        parts = url.path.strip("/").split("/")
        if parts[:1] == ["Items"] and len(parts) >= 4 and parts[2] == "Images":
            width = int(query.get("maxWidth", ["640"])[0])
            body = image(parts[1], width, width * 9 // 16)
            self.send_response(200)
            self.send_header("Content-Type", "image/png")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        if self.headers.get("X-Emby-Token") != API_KEY:
            return self._json({"error": "unauthorized"}, 401)
        if url.path == "/Users":
            return self._json([{"Name": "admin", "Id": "u0", "Policy": {"IsHidden": True}},
                               {"Name": "Martin", "Id": "u1", "Policy": {}}])
        if url.path == "/Users/u1/Items/Resume":
            return self._json({"Items": ITEMS[:2]})
        if url.path == "/Users/u1/Items/Latest":
            return self._json(ITEMS[1:])
        if url.path == "/Users/u1/Items":
            term = query.get("SearchTerm", [""])[0].lower()
            found = [i for i in ITEMS if term in (i.get("SeriesName", "") + " " + i["Name"]).lower()]
            return self._json({"Items": found})
        return self._json({"error": "not found"}, 404)


class FakeJellyfin:
    def __init__(self):
        self.httpd = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        self.httpd.daemon_threads = True
        threading.Thread(target=self.httpd.serve_forever, daemon=True).start()
        self.url = f"http://127.0.0.1:{self.httpd.server_address[1]}"

    def stop(self):
        self.httpd.shutdown()
        self.httpd.server_close()
