import json

from bsdtv import config


def test_deep_merge_merges_nested_dicts_and_replaces_lists():
    base = {"a": 1, "nested": {"x": 1, "y": 2}, "list": [1, 2]}
    merged = config.deep_merge(base, {"nested": {"y": 3}, "list": [9]})
    assert merged == {"a": 1, "nested": {"x": 1, "y": 3}, "list": [9]}
    assert base["nested"]["y"] == 2


def test_user_config_overrides_system_config(isolated_dirs):
    etc = isolated_dirs / "etc"
    etc.mkdir()
    (etc / "config.json").write_text(json.dumps({"columns": 6, "vars": {"jellyfin_url": "http://sys"}}))
    config.save_user_setting("columns", 4)

    cfg = config.load_config()
    assert cfg["columns"] == 4
    assert cfg["vars"]["jellyfin_url"] == "http://sys"
    assert cfg["vars"]["youtube_url"] == config.DEFAULTS["vars"]["youtube_url"]


def test_broken_json_is_ignored(isolated_dirs, capsys):
    user = config.user_dir()
    user.mkdir(parents=True)
    (user / "config.json").write_text("{nope")
    assert config.load_config()["theme"] == "graphite"
    assert "ignoring unreadable" in capsys.readouterr().out


def test_save_user_setting_keeps_other_keys():
    config.save_user_setting("theme", "aurora")
    config.save_user_setting("columns", 7)
    data = json.loads((config.user_dir() / "config.json").read_text())
    assert data == {"theme": "aurora", "columns": 7}


def test_bundled_themes_are_complete():
    themes = config.list_themes()
    assert {"graphite", "midnight", "aurora", "sunrise", "beastie"} <= set(themes)
    for theme in themes.values():
        assert set(config.THEME_DEFAULTS) <= set(theme)


def test_unknown_theme_falls_back_to_graphite():
    theme = config.load_theme("does-not-exist")
    assert theme["name"] == "Graphite" and theme["tiles"] == "flat"


def test_profiles_override_settings(isolated_dirs):
    etc = isolated_dirs / "etc"
    etc.mkdir()
    (etc / "config.json").write_text(json.dumps({
        "dock": ["kodi"],
        "profiles": [{"id": "dad", "name": "Dad"},
                     {"id": "kids", "name": "Kids", "dock": ["youtube"], "theme": "sunrise",
                      "jellyfin_user": "kids"}],
    }))
    assert config.load_config()["dock"] == ["kodi"]
    assert config.active_profile(config.load_config())["id"] == "dad"

    config.save_user_setting("profile", "kids")
    cfg = config.load_config()
    assert cfg["dock"] == ["youtube"] and cfg["theme"] == "sunrise"
    assert cfg["jellyfin"]["user"] == "kids"


def test_without_profiles_there_is_one_default():
    found = config.profiles(config.load_config())
    assert len(found) == 1 and found[0]["id"] == ""
