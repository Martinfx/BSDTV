"""HDMI-CEC bridge: lets the TV's own remote drive BSDTV.

Runs ``cec-client`` from libcec (works with a Pulse-Eight USB-CEC adapter or
any adapter libcec supports on FreeBSD), parses the key presses it logs and
forwards them to the launcher API.
"""

import argparse
import re
import shutil
import subprocess
import sys

from . import config
from .ctl import call

# cec-client logs lines such as "key pressed: up (1)" / "key pressed: select (0, 0)".
KEY_LINE = re.compile(r"key pressed: ([a-z0-9 ._-]+?) \(", re.IGNORECASE)

CEC_KEYS = {
    "up": ("key", "up"),
    "down": ("key", "down"),
    "left": ("key", "left"),
    "right": ("key", "right"),
    "select": ("key", "ok"),
    "exit": ("key", "back"),
    "back": ("key", "back"),
    "root menu": ("home", None),
    "setup menu": ("key", "menu"),
    "contents menu": ("key", "menu"),
    "play": ("key", "playpause"),
    "pause": ("key", "playpause"),
    "stop": ("key", "stop"),
    "fast forward": ("key", "forward"),
    "rewind": ("key", "rewind"),
    "forward": ("key", "next"),
    "backward": ("key", "prev"),
    "electronic program guide": ("key", "info"),
    "display information": ("key", "info"),
    "volume up": ("volume", "up"),
    "volume down": ("volume", "down"),
    "mute": ("volume", "mute"),
}


def parse_line(line):
    """Return (action, argument) for a cec-client log line, or None."""
    match = KEY_LINE.search(line)
    if not match:
        return None
    return CEC_KEYS.get(match.group(1).strip().lower())


def dispatch(action, arg):
    if action == "home":
        call("/api/home", {})
    elif action == "key":
        call("/api/key", {"key": arg})
    elif action == "volume":
        call("/api/volume", {"action": arg})


def main(argv=None):
    parser = argparse.ArgumentParser(prog="bsdtv-cec", description="Forward HDMI-CEC remote keys to BSDTV")
    parser.add_argument("device", nargs="?", default="", help="CEC adapter port (default: autodetect)")
    parser.add_argument("--name", default="BSDTV", help="OSD name shown on the TV")
    parser.add_argument("--if-enabled", action="store_true",
                        help="exit quietly unless cec.enabled is true in the config (used by the X session)")
    args = parser.parse_args(argv)

    cec_config = config.load_config().get("cec", {})
    if args.if_enabled and not cec_config.get("enabled"):
        return 0
    device = args.device or cec_config.get("device", "")

    if not shutil.which("cec-client"):
        print("bsdtv-cec: cec-client not found (pkg install libcec)", file=sys.stderr)
        return 1
    # -t p: register as a playback device; -d 24: traffic + debug, where libcec logs key presses.
    argv = ["cec-client", "-t", "p", "-o", args.name, "-d", "24"]
    if device:
        argv.append(device)
    with subprocess.Popen(argv, stdout=subprocess.PIPE, stdin=subprocess.PIPE, text=True) as proc:
        for line in proc.stdout:
            parsed = parse_line(line)
            if parsed:
                try:
                    dispatch(*parsed)
                except SystemExit as exc:
                    print(exc, file=sys.stderr)
    return proc.returncode


if __name__ == "__main__":
    sys.exit(main())
