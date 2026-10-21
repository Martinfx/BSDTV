#!/bin/sh
# Undo install.sh using the record kept in /var/db/bsdtv.

set -eu

PREFIX=/usr/local
STATE=${BSDTV_STATE:-/var/db/bsdtv}
REMOVE_PACKAGES=no
REMOVE_USER=no
DRY_RUN=no

usage() {
	cat <<EOF
Usage: sh uninstall.sh [options]

  --remove-packages   also remove the packages BSDTV installed
  --remove-user       also delete the TV account and its home directory
                      (only if BSDTV created it)
  --dry-run           print what would be done, change nothing
EOF
}

while [ $# -gt 0 ]; do
	case $1 in
	--remove-packages) REMOVE_PACKAGES=yes ;;
	--remove-user) REMOVE_USER=yes ;;
	--dry-run) DRY_RUN=yes ;;
	-h | --help) usage; exit 0 ;;
	*) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
	esac
	shift
done

say() { printf '\033[1;34m==>\033[0m %s\n' "$*"; }
run() {
	echo "  + $*"
	[ "$DRY_RUN" = yes ] || "$@"
}

[ -d "$STATE" ] || { echo "Nothing to undo: $STATE does not exist."; exit 0; }
[ "$DRY_RUN" = yes ] || [ "$(id -u)" -eq 0 ] || { echo "run as root" >&2; exit 1; }

say "Restoring rc.conf / loader.conf values"
if [ -f "$STATE/rc.orig" ]; then
	while read -r file var old; do
		if [ "$old" = __unset__ ]; then
			run sysrc -f "$file" -x "$var" || true
		else
			run sysrc -f "$file" "$var=$old" >/dev/null
		fi
	done < "$STATE/rc.orig"
fi

say "Removing BSDTV blocks from config files"
if [ -f "$STATE/blocks" ]; then
	sort -u "$STATE/blocks" | while read -r f; do
		[ -f "$f" ] || continue
		echo "  + strip BSDTV block from $f"
		if [ "$DRY_RUN" = no ]; then
			sed -i '' '/^# BSDTV begin/,/^# BSDTV end/d' "$f"
			[ -s "$f" ] || rm -f "$f"
		fi
	done
fi
if grep -q '^ttyv0.*getty bsdtv' /etc/ttys 2>/dev/null; then
	echo "  + /etc/ttys: ttyv0 -> getty Pc"
	[ "$DRY_RUN" = yes ] || sed -i '' 's|"/usr/libexec/getty bsdtv"|"/usr/libexec/getty Pc"|' /etc/ttys
fi

say "Restoring replaced files"
if [ -d "$STATE/backup" ]; then
	(cd "$STATE/backup" && find . -type f) | while read -r f; do
		run cp -p "$STATE/backup/${f#./}" "/${f#./}"
	done
fi
if [ -f "$STATE/created" ]; then
	while read -r f; do
		run rm -f "$f"
	done < "$STATE/created"
fi

say "Removing BSDTV"
run rm -rf "$PREFIX/share/bsdtv"
echo "  = keeping $PREFIX/etc/bsdtv (your settings); delete it by hand if you want"

if [ "$REMOVE_USER" = yes ] && [ -f "$STATE/created-user" ]; then
	user=$(cat "$STATE/created-user")
	say "Deleting user $user"
	run pw userdel "$user" -r
fi

if [ "$REMOVE_PACKAGES" = yes ] && [ -f "$STATE/packages" ]; then
	say "Removing packages installed by BSDTV"
	# shellcheck disable=SC2046 # one package name per word
	run env ASSUME_ALWAYS_YES=yes pkg delete $(sort -u "$STATE/packages")
	run env ASSUME_ALWAYS_YES=yes pkg autoremove
fi

run rm -rf "$STATE"
say "Done. Reboot to get the normal console back."
