"""Phone remote: a tiny HTTP server serving the web remote and a JSON API.

Requests from localhost are trusted (bsdtv-ctl, openbox key bindings).
Every other client must send the pairing token shown in the TV settings,
either as the ``X-BSDTV-Token`` header or the ``t`` query parameter.
"""

import hmac
import json
import secrets
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse

from . import config

WEB_DIR = Path(__file__).resolve().parent / "web"
MAX_BODY = 4096


def load_token():
    """Read the pairing token, creating it on first use."""
    path = config.user_dir() / "remote-token"
    try:
        token = path.read_text(encoding="utf-8").strip()
        if token:
            return token
    except FileNotFoundError:
        pass
    token = secrets.token_urlsafe(9)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.touch(mode=0o600)
    path.write_text(token + "\n", encoding="utf-8")
    return token


class Controller:
    """What the remote can do. The launcher backend implements this."""

    def apps(self):
        return []

    def status(self):
        return {}

    def launch(self, app_id):
        return False

    def home(self):
        return False

    def key(self, name):
        return False

    def text(self, value):
        return False

    def volume(self, action):
        return False

    def power(self, action):
        return False

    def reload(self):
        return False


def make_handler(controller, token):
    class Handler(BaseHTTPRequestHandler):
        server_version = "BSDTV"

        def log_message(self, fmt, *args):
            pass

        def _authorized(self, query):
            if self.client_address[0] in ("127.0.0.1", "::1"):
                return True
            supplied = self.headers.get("X-BSDTV-Token") or query.get("t", [""])[0]
            return hmac.compare_digest(supplied.encode(), token.encode())

        def _send(self, status, body, content_type="application/json"):
            data = body if isinstance(body, bytes) else json.dumps(body).encode()
            self.send_response(status)
            self.send_header("Content-Type", content_type)
            self.send_header("Content-Length", str(len(data)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(data)

        def do_GET(self):
            url = urlparse(self.path)
            query = parse_qs(url.query)
            if url.path in ("/", "/index.html"):
                # The page itself is harmless; it asks for the token before calling the API.
                html = (WEB_DIR / "remote.html").read_bytes()
                return self._send(200, html, "text/html; charset=utf-8")
            if not self._authorized(query):
                return self._send(403, {"error": "bad token"})
            if url.path == "/api/apps":
                return self._send(200, {"apps": controller.apps()})
            if url.path == "/api/status":
                return self._send(200, controller.status())
            return self._send(404, {"error": "not found"})

        def do_POST(self):
            url = urlparse(self.path)
            if not self._authorized(parse_qs(url.query)):
                return self._send(403, {"error": "bad token"})
            try:
                length = int(self.headers.get("Content-Length") or 0)
            except ValueError:
                length = -1
            if length < 0 or length > MAX_BODY:
                return self._send(413, {"error": "body too large"})
            try:
                body = json.loads(self.rfile.read(length) or b"{}")
            except ValueError:
                return self._send(400, {"error": "invalid json"})
            if not isinstance(body, dict):
                return self._send(400, {"error": "expected an object"})

            actions = {
                "/api/key": lambda: controller.key(str(body.get("key", ""))),
                "/api/text": lambda: controller.text(str(body.get("text", ""))[:500]),
                "/api/launch": lambda: controller.launch(str(body.get("app", ""))),
                "/api/home": controller.home,
                "/api/volume": lambda: controller.volume(str(body.get("action", ""))),
                "/api/power": lambda: controller.power(str(body.get("action", ""))),
                "/api/reload": controller.reload,
            }
            action = actions.get(url.path)
            if action is None:
                return self._send(404, {"error": "not found"})
            ok = bool(action())
            return self._send(200 if ok else 400, {"ok": ok})

    return Handler


class RemoteServer:
    def __init__(self, controller, token, bind="0.0.0.0", port=8765):
        self.httpd = ThreadingHTTPServer((bind, port), make_handler(controller, token))
        self.httpd.daemon_threads = True
        self.thread = threading.Thread(target=self.httpd.serve_forever, name="bsdtv-remote", daemon=True)

    @property
    def port(self):
        return self.httpd.server_address[1]

    def start(self):
        self.thread.start()
        return self

    def stop(self):
        self.httpd.shutdown()
        self.httpd.server_close()
