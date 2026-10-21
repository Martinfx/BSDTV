# Set up BSDTV with an AI assistant

Paste everything below the line into a coding assistant with shell access
(Claude Code, Codex, Gemini CLI) on a computer in the same network as the
FreeBSD box. Fill in the address first.

---

You are setting up BSDTV (https://github.com/martinfx/BSDTV) on a FreeBSD
machine that will be used as a TV box.

- Box address: `ssh root@<IP-OF-THE-BOX>` (I can type a password if asked)
- Read the BSDTV README.md, docs/HARDWARE.md and docs/CONFIG.md first.

Steps:

1. Connect over SSH and check the system: `freebsd-version`, `uname -m`,
   `pciconf -lv | grep -B3 -A2 display`, `cat /dev/sndstat`, free disk space.
   Tell me what you found and which `--gpu` and `--audio-unit` you suggest.
2. Ask me, in one short checklist:
   - which apps I want (Kodi, YouTube, Jellyfin + its URL, IPTV playlist URL,
     music folder, Moonlight, RetroArch, web browser)
   - which four apps go in the dock
   - whether to block ads for the whole network (`--adblock`)
   - whether I want Netflix / Disney+ / Prime Video / Max / Spotify (`--drm`;
     needs the ports tree for Widevine, see README)
   - whether I use a HDMI-CEC adapter
   - whether SSH should stay enabled afterwards
3. Clone the repository on the box, run `sh install.sh --dry-run` with the
   chosen options and show me the summary. Only after I confirm, run it for
   real with `--yes`.
4. Write my answers into `/usr/local/etc/bsdtv/config.json` (dock, vars,
   cec), keeping it valid JSON. Validate with
   `python3 -m json.tool /usr/local/etc/bsdtv/config.json`.
5. Reboot, wait for the box to come back, then check the session:
   `ps -U tv -o pid,command | grep -E 'Xorg|openbox|bsdtv'` and
   `su tv -c 'bsdtv-ctl status'`. Give me the phone remote address from
   `su tv -c 'bsdtv-ctl url'`.
6. If something fails, read `~tv/.local/share/bsdtv/xsession.log` and
   `/var/log/Xorg.0.log` before changing anything, and explain the fix to me.

Rules: never disable security features to make something work, never delete
data in the tv user's home, and use `sh uninstall.sh` if I ask to undo
everything.
