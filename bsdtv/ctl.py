"""bsdtv-ctl: control a running launcher from the shell or from window manager key bindings."""

import argparse
import json
import sys
import urllib.error
import urllib.request

from . import config, remote, system


def call(path, body=None, port=None):
    port = port or int(config.load_config().get("remote", {}).get("port", 8765))
    data = None if body is None else json.dumps(body).encode()
    request = urllib.request.Request(f"http://127.0.0.1:{port}{path}", data=data,
                                     headers={"Content-Type": "application/json"})
    try:
        with urllib.request.urlopen(request, timeout=5) as response:
            return json.load(response)
    except urllib.error.HTTPError as exc:
        return json.load(exc)
    except (urllib.error.URLError, OSError) as exc:
        raise SystemExit(f"bsdtv-ctl: launcher is not reachable on port {port}: {exc}")


def main(argv=None):
    parser = argparse.ArgumentParser(prog="bsdtv-ctl", description="Control the BSDTV launcher")
    sub = parser.add_subparsers(dest="cmd", required=True)
    sub.add_parser("home", help="close the running app and show the home screen")
    sub.add_parser("apps", help="list installed apps")
    sub.add_parser("status", help="show what is running")
    sub.add_parser("reload", help="re-read config, apps and theme")
    sub.add_parser("url", help="print the phone remote URL")
    p = sub.add_parser("launch", help="start an app")
    p.add_argument("app")
    p = sub.add_parser("key", help="send remote keys")
    p.add_argument("keys", nargs="+", choices=sorted(system.KEYSYMS))
    p = sub.add_parser("text", help="type text into the focused app")
    p.add_argument("text")
    p = sub.add_parser("volume")
    p.add_argument("action", choices=["up", "down", "mute"])
    p = sub.add_parser("power")
    p.add_argument("action", choices=sorted(system.POWER_COMMANDS))
    p = sub.add_parser("theme", help="switch theme")
    p.add_argument("theme", choices=sorted(config.list_themes()))
    args = parser.parse_args(argv)

    if args.cmd == "url":
        port = config.load_config().get("remote", {}).get("port", 8765)
        print(f"http://{system.ip_address() or 'TV-IP'}:{port}/?t={remote.load_token()}")
        return 0
    if args.cmd == "theme":
        config.save_user_setting("theme", args.theme)
        result = call("/api/reload", {})
    elif args.cmd in ("apps", "status"):
        result = call(f"/api/{args.cmd}")
    elif args.cmd == "key":
        result = {"ok": all(call("/api/key", {"key": k}).get("ok") for k in args.keys)}
    elif args.cmd == "launch":
        result = call("/api/launch", {"app": args.app})
    elif args.cmd == "text":
        result = call("/api/text", {"text": args.text})
    elif args.cmd in ("volume", "power"):
        result = call(f"/api/{args.cmd}", {"action": args.action})
    else:
        result = call(f"/api/{args.cmd}", {})

    if args.cmd == "apps":
        for app in result.get("apps", []):
            print(f"{app['id']:12} {app['name']}")
    elif args.cmd == "status":
        print(json.dumps(result, indent=2))
    return 0 if result.get("ok", True) else 1


if __name__ == "__main__":
    sys.exit(main())
