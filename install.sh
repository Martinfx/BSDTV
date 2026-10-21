#!/bin/sh
# BSDTV installer: turns a fresh FreeBSD 14+ machine into a TV box.
#
#   fetch -o - https://github.com/martinfx/BSDTV/archive/main.tar.gz | tar xzf -
#   cd BSDTV-main && sh install.sh
#
# Everything this script changes is recorded under /var/db/bsdtv so that
# uninstall.sh can put the system back the way it was.

set -eu

VERSION=0.1.0
SRC=$(cd "$(dirname "$0")" && pwd)
PREFIX=/usr/local
ETC=$PREFIX/etc/bsdtv
STATE=${BSDTV_STATE:-/var/db/bsdtv}

TV_USER=tv
APPS="kodi firefox mpv"
GPU=auto
ADBLOCK=no
DRM=no
SSH=keep
AUDIO_UNIT=
DRY_RUN=no
ASSUME_YES=no

usage() {
	cat <<EOF
BSDTV $VERSION installer

Usage: sh install.sh [options]

  --apps "LIST"     packages for TV apps (default: "$APPS")
                    available: kodi firefox mpv moonlight-qt retroarch
  --gpu TYPE        auto | intel | amd | nvidia | generic (default: auto)
  --user NAME       account that runs the TV session (default: $TV_USER)
  --adblock         block ads and trackers for the whole network via local_unbound
  --drm             Netflix, Disney+, Prime Video, Max and Spotify through Brave
                    (Linux build) with Widevine; needs the ports tree for Widevine
  --ssh             enable sshd for remote administration
  --audio-unit N    send sound to pcmN (see: cat /dev/sndstat)
  --dry-run         print what would be done, change nothing
  -y, --yes         do not ask for confirmation
  -h, --help        show this help
EOF
}

while [ $# -gt 0 ]; do
	case $1 in
	--apps) APPS=$2; shift ;;
	--gpu) GPU=$2; shift ;;
	--user) TV_USER=$2; shift ;;
	--adblock) ADBLOCK=yes ;;
	--drm) DRM=yes ;;
	--ssh) SSH=yes ;;
	--audio-unit) AUDIO_UNIT=$2; shift ;;
	--dry-run) DRY_RUN=yes ;;
	-y | --yes) ASSUME_YES=yes ;;
	-h | --help) usage; exit 0 ;;
	*) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
	esac
	shift
done

say() { printf '\033[1;34m==>\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33mwarning:\033[0m %s\n' "$*" >&2; }
die() { printf '\033[1;31merror:\033[0m %s\n' "$*" >&2; exit 1; }

run() {
	echo "  + $*"
	[ "$DRY_RUN" = yes ] || "$@"
}

# Remember that we touched a file. Existing files are copied once into the
# backup tree; files we create are listed so uninstall can delete them.
track_file() {
	f=$1
	[ "$DRY_RUN" = yes ] && return 0
	if [ -e "$f" ]; then
		[ -e "$STATE/backup$f" ] && return 0
		mkdir -p "$STATE/backup$(dirname "$f")"
		cp -p "$f" "$STATE/backup$f"
	else
		grep -qxF "$f" "$STATE/created" 2>/dev/null || echo "$f" >> "$STATE/created"
	fi
}

# Install a file from the source tree, recording what it replaces.
put() {
	mode=$1 src=$2 dest=$3
	track_file "$dest"
	run install -m "$mode" "$src" "$dest"
}

# sysrc wrapper that records the previous value once, for uninstall.
set_rc() {
	file=$1 assignment=$2
	var=${assignment%%[+=]*}
	if [ "$DRY_RUN" = no ] && ! grep -q "^$file $var " "$STATE/rc.orig" 2>/dev/null; then
		old=$(sysrc -f "$file" -n -q "$var" 2>/dev/null) || old=__unset__
		echo "$file $var $old" >> "$STATE/rc.orig"
	fi
	run sysrc -f "$file" "$assignment"
}

# Append a "# BSDTV begin ... # BSDTV end" block to a file, once.
# uninstall.sh removes exactly that block, keeping any later edits.
add_block() {
	snippet=$1 dest=$2
	if [ -f "$dest" ] && grep -q '^# BSDTV begin' "$dest"; then
		echo "  = $dest already has the BSDTV block"
		return 0
	fi
	[ "$DRY_RUN" = yes ] || echo "$dest" >> "$STATE/blocks"
	echo "  + add BSDTV block to $dest"
	[ "$DRY_RUN" = yes ] || { mkdir -p "$(dirname "$dest")"; cat "$snippet" >> "$dest"; }
}

pkg_add() {
	required=$1; shift
	for p in "$@"; do
		if pkg info -e "$p" 2>/dev/null; then
			continue
		fi
		if run env ASSUME_ALWAYS_YES=yes pkg install -q "$p"; then
			[ "$DRY_RUN" = yes ] || echo "$p" >> "$STATE/packages"
		elif [ "$required" = required ]; then
			die "could not install package $p"
		else
			warn "optional package $p is not available, skipping"
		fi
	done
}

# Install the first of several package names that exists (names change between releases).
pkg_add_any() {
	for p in "$@"; do
		if pkg info -e "$p" 2>/dev/null; then
			return 0
		fi
		if run env ASSUME_ALWAYS_YES=yes pkg install -q "$p"; then
			[ "$DRY_RUN" = yes ] || echo "$p" >> "$STATE/packages"
			return 0
		fi
	done
	die "could not install any of: $*"
}

detect_gpu() {
	vendors=$(pciconf -l 2>/dev/null | awk '/^vgapci/ { for (i = 1; i <= NF; i++) if ($i ~ /^vendor=/) print substr($i, 8) }')
	case $vendors in
	*0x10de*) echo nvidia ;;
	*0x1002*) echo amd ;;
	*0x8086*) echo intel ;;
	*) echo generic ;;
	esac
}

# --- preflight ---------------------------------------------------------------

if [ "$DRY_RUN" = no ]; then
	[ "$(uname -s)" = FreeBSD ] || die "BSDTV installs on FreeBSD (try --dry-run elsewhere)"
	[ "$(id -u)" -eq 0 ] || die "run as root"
	major=$(uname -r | cut -d. -f1)
	[ "$major" -ge 14 ] || warn "FreeBSD $major is untested, 14.x or newer is recommended"
fi

[ "$GPU" = auto ] && GPU=$(detect_gpu)
case $GPU in intel | amd | nvidia | generic) ;; *) die "unknown --gpu $GPU" ;; esac

cat <<EOF

  BSDTV $VERSION
  ----------------------------------------------
  TV user:     $TV_USER (logs in automatically on ttyv0)
  Apps:        $APPS
  Graphics:    $GPU
  Ad blocking: $ADBLOCK
  Streaming:   $DRM (Brave + Widevine)
  SSH:         $SSH
  Dry run:     $DRY_RUN

EOF
if [ "$ASSUME_YES" = no ] && [ "$DRY_RUN" = no ]; then
	printf 'Continue? [y/N] '
	read -r answer
	case $answer in [yY]*) ;; *) echo "Aborted."; exit 1 ;; esac
fi

[ "$DRY_RUN" = yes ] || mkdir -p "$STATE/backup"

# --- packages ----------------------------------------------------------------

say "Installing packages"
if [ "$DRY_RUN" = no ] && ! pkg -N >/dev/null 2>&1; then
	run env ASSUME_ALWAYS_YES=yes pkg bootstrap -f
fi
pkg_add required xorg-minimal xinit xauth xset xsetroot xdotool openbox dbus doas
# BSDTV is C++/QML: Qt 6 to run, plus the tools to build it.
pkg_add required qt6-base qt6-declarative qt6-tools ninja
command -v cmake >/dev/null 2>&1 || pkg_add_any cmake-core cmake
pkg_add optional unclutter-xfixes noto-basic libcec
# shellcheck disable=SC2086 # APPS is a word list on purpose
pkg_add optional $APPS

# --- graphics ----------------------------------------------------------------

say "Configuring graphics ($GPU)"
case $GPU in
intel)
	pkg_add required drm-kmod libva-intel-media-driver
	set_rc /etc/rc.conf "kld_list+=i915kms"
	;;
amd)
	pkg_add required drm-kmod
	set_rc /etc/rc.conf "kld_list+=amdgpu"
	;;
nvidia)
	pkg_add required nvidia-driver
	set_rc /etc/rc.conf "kld_list+=nvidia-modeset"
	run mkdir -p "$PREFIX/etc/X11/xorg.conf.d"
	put 0644 "$SRC/system/xorg-nvidia.conf" "$PREFIX/etc/X11/xorg.conf.d/20-bsdtv-nvidia.conf"
	;;
generic)
	pkg_add optional xf86-video-scfb xf86-video-vesa
	if [ "$(sysctl -n machdep.bootmethod 2>/dev/null)" = UEFI ]; then
		run mkdir -p "$PREFIX/etc/X11/xorg.conf.d"
		put 0644 "$SRC/system/xorg-scfb.conf" "$PREFIX/etc/X11/xorg.conf.d/20-bsdtv-scfb.conf"
	fi
	;;
esac

# --- TV user -----------------------------------------------------------------

say "Creating the $TV_USER account"
if id "$TV_USER" >/dev/null 2>&1; then
	echo "  = $TV_USER exists"
	run pw groupmod video -m "$TV_USER"
else
	# No password: the account can only log in through the console autologin.
	run pw useradd "$TV_USER" -m -s /bin/sh -G video -c "BSDTV"
	[ "$DRY_RUN" = yes ] || echo "$TV_USER" > "$STATE/created-user"
fi
TV_HOME=$(getent passwd "$TV_USER" 2>/dev/null | cut -d: -f6)
TV_HOME=${TV_HOME:-/home/$TV_USER}

# --- BSDTV files -------------------------------------------------------------

say "Building BSDTV"
BUILD=$STATE/build
run cmake -S "$SRC" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX="$PREFIX" -DBSDTV_BUILD_TESTS=OFF
run cmake --build "$BUILD"
say "Installing BSDTV to $PREFIX"
run cmake --install "$BUILD"
# Remember what was installed, for uninstall.sh.
[ "$DRY_RUN" = yes ] || cp "$BUILD/install_manifest.txt" "$STATE/installed-files"

run mkdir -p "$ETC"
for f in config.json apps.json adblock-lists.txt adblock-allow.txt; do
	run install -m 0644 "$SRC/etc/$f" "$ETC/$f.sample"
	if [ -e "$ETC/$f" ]; then
		echo "  = keeping existing $ETC/$f"
	else
		run install -m 0644 "$SRC/etc/$f" "$ETC/$f"
	fi
done

# --- session -----------------------------------------------------------------

say "Setting up the TV session for $TV_USER"
run mkdir -p "$TV_HOME/.local/share/bsdtv/firefox" "$TV_HOME/.local/share/bsdtv/firefox-youtube" "$TV_HOME/.config/bsdtv"
put 0644 "$SRC/system/xinitrc" "$TV_HOME/.xinitrc"
add_block "$SRC/system/profile.snippet" "$TV_HOME/.profile"
run install -m 0644 "$SRC/system/firefox/user.js" "$TV_HOME/.local/share/bsdtv/firefox/user.js"
if [ "$DRY_RUN" = no ]; then
	cat "$SRC/system/firefox/user.js" "$SRC/system/firefox/user-youtube.js" \
		> "$TV_HOME/.local/share/bsdtv/firefox-youtube/user.js"
fi
run chown -R "$TV_USER:$TV_USER" "$TV_HOME/.local" "$TV_HOME/.config" "$TV_HOME/.xinitrc" "$TV_HOME/.profile"

if [ -d "$PREFIX/lib/firefox" ]; then
	run mkdir -p "$PREFIX/lib/firefox/distribution"
	put 0644 "$SRC/system/firefox/policies.json" "$PREFIX/lib/firefox/distribution/policies.json"
fi

# Autologin on the first console.
gettytab_src=$SRC/system/gettytab.snippet
if [ "$TV_USER" != tv ]; then
	gettytab_src=$(mktemp)
	sed "s/:al=tv:/:al=$TV_USER:/" "$SRC/system/gettytab.snippet" > "$gettytab_src"
fi
add_block "$gettytab_src" /etc/gettytab
if grep -q '^ttyv0.*getty bsdtv' /etc/ttys 2>/dev/null; then
	echo "  = /etc/ttys already uses autologin"
else
	echo "  + /etc/ttys: ttyv0 -> getty bsdtv"
	[ "$DRY_RUN" = yes ] || sed -i '' -E 's|^ttyv0([[:space:]]+)"/usr/libexec/getty [A-Za-z0-9.]+"|ttyv0\1"/usr/libexec/getty bsdtv"|' /etc/ttys
fi

doas_src=$SRC/system/doas.conf.snippet
if [ "$TV_USER" != tv ]; then
	doas_src=$(mktemp)
	sed "s/permit nopass tv /permit nopass $TV_USER /" "$SRC/system/doas.conf.snippet" > "$doas_src"
fi
add_block "$doas_src" "$PREFIX/etc/doas.conf"

# --- system ------------------------------------------------------------------

say "Tuning the system for TV use"
set_rc /etc/rc.conf dbus_enable=YES
set_rc /etc/rc.conf powerd_enable=YES
set_rc /etc/rc.conf ntpd_enable=YES
set_rc /etc/rc.conf ntpd_sync_on_start=YES
set_rc /etc/rc.conf sendmail_enable=NONE
set_rc /etc/rc.conf dumpdev=NO
[ "$SSH" = yes ] && set_rc /etc/rc.conf sshd_enable=YES
set_rc /boot/loader.conf autoboot_delay=2
set_rc /boot/loader.conf boot_mute=YES
if [ -n "$AUDIO_UNIT" ]; then
	sysctl_src=$(mktemp)
	printf '# BSDTV begin\nhw.snd.default_unit=%s\n# BSDTV end\n' "$AUDIO_UNIT" > "$sysctl_src"
	add_block "$sysctl_src" /etc/sysctl.conf
	run sysctl "hw.snd.default_unit=$AUDIO_UNIT"
fi

if [ "$ADBLOCK" = yes ]; then
	say "Enabling network ad blocking"
	set_rc /etc/rc.conf local_unbound_enable=YES
	run service local_unbound start || true
	run "$PREFIX/sbin/bsdtv-adblock-update" || warn "block list download failed, run bsdtv-adblock-update later"
	track_file "$PREFIX/etc/cron.d/bsdtv-adblock"
	echo "  + weekly list refresh in $PREFIX/etc/cron.d/bsdtv-adblock"
	if [ "$DRY_RUN" = no ]; then
		mkdir -p "$PREFIX/etc/cron.d"
		echo "17 4 * * 0 root $PREFIX/sbin/bsdtv-adblock-update >/dev/null" > "$PREFIX/etc/cron.d/bsdtv-adblock"
	fi
fi

if [ "$DRM" = yes ]; then
	say "Setting up Brave with Widevine for Netflix, Disney+, Prime Video, Max, Spotify"
	set_rc /etc/rc.conf linux_enable=YES
	run service linux start || true
	pkg_add optional linux-brave
	if pkg info -e linux-widevine-cdm 2>/dev/null; then
		echo "  = linux-widevine-cdm is installed"
	elif [ -d /usr/ports/www/linux-widevine-cdm ]; then
		# Widevine may not be redistributed, so there is no binary package: build it from ports.
		run env BATCH=yes make -C /usr/ports/www/linux-widevine-cdm install clean
		[ "$DRY_RUN" = yes ] || echo linux-widevine-cdm >> "$STATE/packages"
	else
		warn "Widevine has no binary package (license). Fetch the ports tree and build it:"
		warn "  git clone --depth 1 https://git.FreeBSD.org/ports.git /usr/ports"
		warn "  make -C /usr/ports/www/linux-widevine-cdm install clean"
	fi
	if [ "$DRY_RUN" = yes ] || [ -x "$PREFIX/bin/brave-browser" ]; then
		echo "  + enable streaming tiles in $ETC/config.json"
		run "$PREFIX/bin/bsdtv" config-set "$ETC/config.json" vars.drm_browser "$PREFIX/bin/bsdtv-drm-browser"
	else
		warn "linux-brave did not install, streaming tiles stay hidden"
	fi
fi

[ "$DRY_RUN" = yes ] || echo "$VERSION" > "$STATE/version"

cat <<EOF

BSDTV is installed. Reboot to start the TV:

    shutdown -r now

  Phone remote:  shown in Settings on the TV (or run: su $TV_USER -c 'bsdtv-ctl url')
  Configuration: $ETC/config.json  (per-user: $TV_HOME/.config/bsdtv/config.json)
  Undo:          sh $SRC/uninstall.sh
EOF
