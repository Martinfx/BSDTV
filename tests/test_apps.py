import json

from bsdtv import apps, config


def fake_which(*present):
    return lambda name: f"/usr/local/bin/{name}" if name in present else None


def test_expand_fills_known_placeholders_and_keeps_unknown():
    out = apps.expand("{a}/{missing}", {"a": "x"})
    assert out == "x/{missing}"


def test_expand_expands_home():
    assert not apps.expand("{p}", {"p": "~/x"}).startswith("~")


def test_parse_skips_invalid_and_duplicate_entries():
    data = {"apps": [{"id": "a", "exec": ["a"]}, {"id": "a", "exec": ["b"]}, {"name": "no id"}, "junk",
                     {"id": "b"}]}
    parsed = apps.parse_apps(data)
    assert [a.id for a in parsed] == ["a"]
    assert parsed[0].exec == ["a"]


def test_bundled_catalogue_filters_by_installed_binary_and_vars():
    cfg = config.load_config()
    found = apps.load_apps(cfg, which=fake_which("kodi", "firefox"))
    ids = [a.id for a in found]
    assert ids == ["kodi", "youtube", "browser"]  # jellyfin needs jellyfin_url, mpv apps need mpv

    cfg["vars"]["jellyfin_url"] = "http://media:8096"
    assert "jellyfin" in [a.id for a in apps.load_apps(cfg, which=fake_which("firefox"))]


def test_hidden_apps_are_dropped():
    cfg = config.load_config()
    cfg["hidden"] = ["youtube"]
    assert "youtube" not in [a.id for a in apps.load_apps(cfg, which=fake_which("firefox"))]


def test_user_apps_override_system_apps():
    user = config.user_dir()
    user.mkdir(parents=True)
    (user / "apps.json").write_text(json.dumps({"apps": [
        {"id": "kodi", "name": "My Kodi", "exec": ["kodi"]},
        {"id": "plex", "name": "Plex", "exec": ["plexhtpc"]},
    ]}))
    found = {a.id: a for a in apps.load_apps(config.load_config(), which=fake_which("kodi", "plexhtpc"))}
    assert found["kodi"].name == "My Kodi"
    assert "plex" in found


def test_command_uses_youtube_profile():
    cfg = config.load_config()
    youtube = next(a for a in apps.load_apps(cfg, which=fake_which("firefox")) if a.id == "youtube")
    argv = youtube.command(cfg["vars"])
    assert argv[0] == "firefox"
    assert argv[-1] == "https://www.youtube.com/tv"
    assert argv[argv.index("--profile") + 1].endswith("firefox-youtube")


def test_order_dock_follows_config_and_skips_missing():
    items = apps.parse_apps({"apps": [{"id": i, "exec": [i]} for i in ("a", "b", "c")]})
    assert [a.id for a in apps.order_dock(items, ["c", "x", "a"])] == ["c", "a"]
