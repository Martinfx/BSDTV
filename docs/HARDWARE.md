# Hardware

BSDTV needs a 64-bit (amd64) PC that FreeBSD 14+ supports, with a GPU that
FreeBSD's DRM drivers can drive. Video decoding on the GPU matters more than
CPU speed: it is what makes 4K playback smooth and quiet.

## Good choices

| Hardware | Notes |
| --- | --- |
| Intel NUC / mini PCs with Intel UHD / Iris Xe (Gen 9 – Gen 12) | Best supported. `drm-kmod` + `i915kms`, VA-API decoding via `libva-intel-media-driver`. |
| Mini PCs with AMD Ryzen APUs (Vega, RDNA2) | `drm-kmod` + `amdgpu`. Use a recent FreeBSD release for newer chips. |
| Desktop PCs with NVIDIA GPUs | `nvidia-driver`; video decoding through NVDEC in mpv/Kodi. |
| Virtual machines | Work with `--gpu generic` for trying BSDTV out; no video acceleration. |

## Less suitable

- **Raspberry Pi and other arm64 boards**: FreeBSD has no GPU acceleration
  for them, so the home screen and video playback are software rendered.
- **Very new GPUs**: check the `drm-kmod` version in the FreeBSD release you
  install before buying.

## Netflix and other DRM services

With `install.sh --drm` these run in the Linux build of Brave with software
Widevine (L3). That means reduced resolution (Netflix in a browser usually
tops out at 720p) and no HDR, whatever the hardware. A faster CPU helps
because L3 decrypts in software.

## Sound over HDMI

FreeBSD often makes the analog output `pcm0` and HDMI `pcm1` or later.

```sh
cat /dev/sndstat                 # list devices
sysctl hw.snd.default_unit=1     # try it now
sh install.sh --audio-unit 1     # or set it at install time
```

## Remotes

Anything that sends standard keys works:

- **Air-mouse / Bluetooth / 2.4 GHz USB remotes** (the cheap "Android TV box"
  ones): arrows, OK, Back, Home and volume work out of the box.
- **The TV's own remote over HDMI-CEC**: needs a CEC adapter libcec supports,
  for example the Pulse-Eight USB-CEC adapter (PC graphics cards do not expose
  CEC). Install `libcec`, then set `"cec": {"enabled": true}` in
  `/usr/local/etc/bsdtv/config.json`.
- **A keyboard**: arrows, Enter, Esc, F12 (Home), Menu key (Settings).
- **Your phone**: see Settings → Phone remote on the TV.
