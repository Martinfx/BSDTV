import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))


@pytest.fixture(autouse=True)
def isolated_dirs(tmp_path, monkeypatch):
    """Point system and user config at empty temp dirs so the host never leaks in."""
    monkeypatch.setenv("BSDTV_ETC", str(tmp_path / "etc"))
    monkeypatch.setenv("XDG_CONFIG_HOME", str(tmp_path / "home-config"))
    return tmp_path
