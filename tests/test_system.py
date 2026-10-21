from bsdtv import system


def test_unknown_actions_do_nothing(monkeypatch):
    calls = []
    monkeypatch.setattr(system, "_run", lambda argv: calls.append(argv) or True)
    assert system.send_key("selfdestruct") is False
    assert system.volume("louder") is False
    assert system.power("format") is False
    assert calls == []


def test_volume_uses_freebsd_mixer_syntax(monkeypatch):
    calls = []
    monkeypatch.setattr(system, "_run", lambda argv: calls.append(argv) or True)
    system.volume("up")
    system.volume("mute")
    assert calls == [["mixer", "vol.volume=+5%"], ["mixer", "vol.mute=^"]]


def test_power_goes_through_doas(monkeypatch):
    calls = []
    monkeypatch.setattr(system, "_run", lambda argv: calls.append(argv) or True)
    system.power("poweroff")
    assert calls == [["doas", "/sbin/shutdown", "-p", "now"]]


def test_missing_tool_is_reported_not_raised(monkeypatch):
    monkeypatch.setattr(system.shutil, "which", lambda name: None)
    assert system.send_key("up") is False
