import json
import urllib.error
import urllib.request

import pytest

from bsdtv import remote


class FakeController(remote.Controller):
    def __init__(self):
        self.calls = []

    def apps(self):
        return [{"id": "kodi", "name": "Kodi", "color": "#000"}]

    def status(self):
        return {"running": ""}

    def launch(self, app_id):
        self.calls.append(("launch", app_id))
        return app_id == "kodi"

    def home(self):
        self.calls.append(("home",))
        return True

    def key(self, name):
        self.calls.append(("key", name))
        return True


@pytest.fixture
def server():
    controller = FakeController()
    srv = remote.RemoteServer(controller, "secret", bind="127.0.0.1", port=0).start()
    yield srv, controller
    srv.stop()


def request(srv, path, body=None, headers=None):
    data = None if body is None else json.dumps(body).encode()
    req = urllib.request.Request(f"http://127.0.0.1:{srv.port}{path}", data=data, headers=headers or {})
    try:
        with urllib.request.urlopen(req, timeout=5) as resp:
            return resp.status, resp.read()
    except urllib.error.HTTPError as exc:
        return exc.code, exc.read()


def test_serves_remote_page(server):
    srv, _ = server
    status, body = request(srv, "/")
    assert status == 200
    assert b"BSDTV Remote" in body


def test_localhost_is_trusted(server):
    srv, controller = server
    status, body = request(srv, "/api/launch", {"app": "kodi"})
    assert status == 200 and json.loads(body) == {"ok": True}
    assert controller.calls == [("launch", "kodi")]


def test_failed_action_returns_400(server):
    srv, _ = server
    status, body = request(srv, "/api/launch", {"app": "nope"})
    assert status == 400 and json.loads(body) == {"ok": False}


def test_unknown_route_and_bad_json(server):
    srv, _ = server
    assert request(srv, "/api/nope", {})[0] == 404
    req = urllib.request.Request(f"http://127.0.0.1:{srv.port}/api/key", data=b"{bad")
    with pytest.raises(urllib.error.HTTPError) as err:
        urllib.request.urlopen(req, timeout=5)
    assert err.value.code == 400


def test_oversized_body_is_rejected(server):
    srv, _ = server
    assert request(srv, "/api/text", {"text": "x" * 10000})[0] == 413


def test_lan_clients_need_the_token(server, monkeypatch):
    srv, controller = server
    # Pretend the request comes from another machine.
    handler = srv.httpd.RequestHandlerClass
    monkeypatch.setattr(handler, "address_string", lambda self: "192.168.1.20")
    original = handler._authorized

    def as_lan(self, query):
        self.client_address = ("192.168.1.20", 50000)
        return original(self, query)

    monkeypatch.setattr(handler, "_authorized", as_lan)
    assert request(srv, "/api/apps")[0] == 403
    assert request(srv, "/api/apps", headers={"X-BSDTV-Token": "wrong"})[0] == 403
    status, body = request(srv, "/api/apps", headers={"X-BSDTV-Token": "secret"})
    assert status == 200 and json.loads(body)["apps"][0]["id"] == "kodi"
    assert request(srv, "/api/home?t=secret", {})[0] == 200
    assert controller.calls == [("home",)]


def test_token_is_created_once_and_private():
    token = remote.load_token()
    assert len(token) >= 10
    assert remote.load_token() == token
    path = remote.config.user_dir() / "remote-token"
    assert path.stat().st_mode & 0o077 == 0
