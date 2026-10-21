import pytest

from bsdtv import apps, jellyfin
from fake_jellyfin import API_KEY, FakeJellyfin


@pytest.fixture(scope="module")
def server():
    srv = FakeJellyfin()
    yield srv
    srv.stop()


def test_picks_first_visible_user_and_reads_resume(server):
    client = jellyfin.Client(server.url, API_KEY)
    assert client.user_id() == "u1"
    resume = client.resume()
    assert [i["mediaId"] for i in resume] == ["m1", "e1"]
    movie, episode = resume
    assert movie["title"] == "Big Buck Bunny" and movie["subtitle"] == "2008"
    assert movie["minutes"] == 10 and movie["progress"] == pytest.approx(0.5)
    assert movie["resumeSeconds"] == 300
    assert episode["title"] == "Northern Lights" and episode["subtitle"] == "S2:E4 The Signal"
    assert episode["backdrop"].endswith("/Items/s1/Images/Backdrop?maxWidth=1920&quality=90")
    assert episode["image"].startswith(server.url + "/Items/e1/Images/Primary")


def test_user_by_name_and_unknown_user(server):
    assert jellyfin.Client(server.url, API_KEY, user="martin").user_id() == "u1"
    with pytest.raises(jellyfin.JellyfinError):
        jellyfin.Client(server.url, API_KEY, user="nobody").user_id()


def test_wrong_key_and_dead_server_raise(server):
    with pytest.raises(jellyfin.JellyfinError):
        jellyfin.Client(server.url, "wrong").resume()
    with pytest.raises(jellyfin.JellyfinError):
        jellyfin.Client("http://127.0.0.1:9", API_KEY, timeout=1).resume()


def test_search_and_latest(server):
    client = jellyfin.Client(server.url, API_KEY)
    assert [i["title"] for i in client.search("sintel")] == ["Sintel"]
    latest = client.latest()
    assert latest[1]["image"].endswith("/Items/m2/Images/Thumb?maxWidth=640&quality=90")
    assert latest[2]["progress"] == 0.0


def test_slides_need_a_backdrop_and_are_unique(server):
    client = jellyfin.Client(server.url, API_KEY)
    items = client.latest() + client.resume()
    slides = jellyfin.slides(items)
    assert [s["mediaId"] for s in slides] == ["e1", "m2", "m3", "m1"]
    assert slides[0]["appId"] == "jellyfin" and slides[0]["image"]


def test_from_config_needs_url_and_key():
    assert jellyfin.from_config({"jellyfin": {"api_key": "k"}}) is None
    assert jellyfin.from_config({"jellyfin": {"url": "http://x"}}) is None
    client = jellyfin.from_config({"jellyfin": {"api_key": "k", "user": "a"}, "vars": {"jellyfin_url": "http://x/"}})
    assert (client.url, client.api_key, client.user) == ("http://x", "k", "a")
    assert client.web_url("abc") == "http://x/web/#/details?id=abc"
    assert client.stream_url("abc") == "http://x/Videos/abc/stream?static=true&api_key=k"


def test_media_rows_are_placed_around_app_rows():
    rows = [{"key": "streaming"}, {"key": "your_apps"}, {"key": "games"}]
    merged = apps.with_media(rows, [{"mediaId": "r"}], [{"mediaId": "l"}])
    assert [r["key"] for r in merged] == ["continue", "streaming", "your_apps", "latest", "games"]
    assert apps.with_media(rows, [], []) == rows
