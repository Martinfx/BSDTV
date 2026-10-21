"""Thin wrappers around FreeBSD system tools (mixer, shutdown, sysctl, xdotool)."""

import os
import shutil
import socket
import subprocess

# Remote key name -> X keysym sent with xdotool.
KEYSYMS = {
    "up": "Up",
    "down": "Down",
    "left": "Left",
    "right": "Right",
    "ok": "Return",
    "back": "Escape",
    "menu": "Menu",
    "playpause": "space",
    "stop": "XF86AudioStop",
    "next": "XF86AudioNext",
    "prev": "XF86AudioPrev",
    "rewind": "Left",
    "forward": "Right",
    "info": "i",
    "backspace": "BackSpace",
}

POWER_COMMANDS = {
    "reboot": ["doas", "/sbin/shutdown", "-r", "now"],
    "poweroff": ["doas", "/sbin/shutdown", "-p", "now"],
    "display_off": ["xset", "dpms", "force", "off"],
}


def _run(argv):
    if not shutil.which(argv[0]):
        print(f"bsdtv: {argv[0]} not found, skipping {' '.join(argv)}")
        return False
    try:
        return subprocess.run(argv, check=False, timeout=10,
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
    except (OSError, subprocess.SubprocessError) as exc:
        print(f"bsdtv: {' '.join(argv)} failed: {exc}")
        return False


def send_key(name):
    keysym = KEYSYMS.get(name)
    if keysym is None:
        return False
    return _run(["xdotool", "key", "--clearmodifiers", keysym])


def send_text(text):
    return _run(["xdotool", "type", "--delay", "40", "--", text])


def volume(action):
    """Change the default mixer volume with FreeBSD's mixer(8): up, down or mute."""
    arg = {"up": "vol.volume=+5%", "down": "vol.volume=-5%", "mute": "vol.mute=^"}.get(action)
    return bool(arg) and _run(["mixer", arg])


def power(action):
    argv = POWER_COMMANDS.get(action)
    return bool(argv) and _run(argv)


def ip_address():
    """Best-effort LAN address (no packets are sent for a UDP connect)."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.connect(("192.0.2.1", 9))
        return sock.getsockname()[0]
    except OSError:
        return ""
    finally:
        sock.close()


def _sysctl(name):
    try:
        out = subprocess.run(["sysctl", "-n", name], capture_output=True, text=True, timeout=2)
    except (OSError, subprocess.SubprocessError):
        return None
    return out.stdout.strip() if out.returncode == 0 else None


def stats():
    """Short CPU temperature / free memory / free disk line for the home screen."""
    parts = []
    temp = _sysctl("dev.cpu.0.temperature")
    if temp:
        parts.append(f"CPU {temp.rstrip('C').split('.')[0]}°C")
    free_pages = _sysctl("vm.stats.vm.v_free_count")
    page_size = _sysctl("hw.pagesize")
    if free_pages and page_size and free_pages.isdigit() and page_size.isdigit():
        parts.append(f"RAM {int(free_pages) * int(page_size) // (1 << 20)} MB free")
    try:
        st = os.statvfs(os.path.expanduser("~"))
        parts.append(f"Disk {st.f_bavail * st.f_frsize // (1 << 30)} GB free")
    except OSError:
        pass
    return "  ·  ".join(parts)
