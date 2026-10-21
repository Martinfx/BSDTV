import pytest

from bsdtv import cec


@pytest.mark.parametrize("line, expected", [
    ("DEBUG:   [   12345]\tkey pressed: up (1) current(ff) duration(0)", ("key", "up")),
    ("DEBUG:   [   12345]\tkey pressed: select (0, 0)", ("key", "ok")),
    ("key pressed: root menu (9)", ("home", None)),
    ("key pressed: volume up (41)", ("volume", "up")),
    ("key pressed: Play (44)", ("key", "playpause")),
    ("key released: up (1) D:120ms", None),
    ("key pressed: unknown thing (99)", None),
    ("TRAFFIC: [ 1] >> 01:44:01", None),
])
def test_parse_line(line, expected):
    assert cec.parse_line(line) == expected


def test_every_mapped_key_is_a_known_remote_key():
    from bsdtv import system
    for action, arg in cec.CEC_KEYS.values():
        if action == "key":
            assert arg in system.KEYSYMS
