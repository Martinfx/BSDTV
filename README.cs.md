# BSDTV

**Chytrá televize, která vás nešpehuje. Postavená na FreeBSD.**

BSDTV udělá z libovolného malého PC (NUC, mini PC, starý notebook) s FreeBSD
čistý TV box bez reklam: klidná domovská obrazovka ve stylu Apple TV, vaše
aplikace, ovladač v mobilu, HDMI-CEC pro ovladač od televize a volitelné
blokování reklam pro celou domácí síť. Žádné účty, žádná telemetrie, žádné
placené „doporučené" řádky.

[English](README.md)

![Domovská obrazovka](docs/img/home.png)

Inspirováno projektem [CleanYourTV](https://github.com/Gathin23/cleanyourtv),
který čistí Android TV od bloatwaru. BSDTV jde opačnou cestou: místo čištění
systému televize si přinese vlastní, postavený na FreeBSD, kde od začátku
nic zbytečného není.

## Co dostanete

- **Domovská obrazovka**: velké hodiny, dok se čtyřmi oblíbenými aplikacemi,
  mřížka všech aplikací o jedno ↓ níž, barevná záře podle vybrané aplikace.
  Ovládání šipkami, OK, Zpět a Domů. Čtyři barevné motivy.
- **Aplikace**: Kodi, YouTube (TV rozhraní, s uBlock Origin), Jellyfin,
  prohlížeč, IPTV a hudba v mpv, Moonlight (streamování her), RetroArch.
  Zobrazí se jen nainstalované; vlastní aplikace přidáte jedním záznamem v JSON.
- **Vždy jedna aplikace**, jako u skutečné TV: tlačítko Domů ji zavře a vrátí
  domovskou obrazovku.
- **Ovladač v mobilu**: otevřete adresu (spárovanou kódem z TV) a máte
  šipky, přehrávání, hlasitost, psaní do vyhledávání i zkratky aplikací.
  Nic se neinstaluje.
- **HDMI-CEC**: ovladač od televize přes USB-CEC adaptér (libcec).
- **Blokování reklam pro celou domácnost** (volitelné): `local_unbound`
  z FreeBSD s veřejnými seznamy, aktualizace každý týden.
- **Startuje rovnou do TV**: automatické přihlášení, žádný display manager,
  2 s v boot menu, tichý start.
- **Vratné**: každá změna se zapisuje do `/var/db/bsdtv`, `uninstall.sh`
  vrátí systém do původního stavu.

## Instalace

Potřebujete 64bitové PC s FreeBSD 14 nebo novějším (stačí minimální instalace;
co funguje dobře, popisuje [docs/HARDWARE.md](docs/HARDWARE.md)). Jako root:

```sh
pkg install -y git
git clone https://github.com/martinfx/BSDTV.git
cd BSDTV
sh install.sh            # volitelně --adblock, --ssh, --apps "kodi firefox mpv moonlight-qt"
shutdown -r now
```

Instalátor se jednou zeptá, než cokoli změní. `sh install.sh --dry-run`
ukáže všechny kroky bez provedení.

| Volba | Význam |
| --- | --- |
| `--apps "SEZNAM"` | balíčky aplikací (výchozí `kodi firefox mpv`) |
| `--gpu TYP` | `auto`, `intel`, `amd`, `nvidia` nebo `generic` |
| `--adblock` | blokování reklam v celé síti přes `local_unbound` |
| `--ssh` | zapne sshd pro vzdálenou správu |
| `--audio-unit N` | zvuk na `pcmN` (HDMI často není 0, viz `cat /dev/sndstat`) |
| `--user JMÉNO` | účet, pod kterým TV běží (výchozí `tv`) |

### S pomocí AI asistenta

Stejně jako u CleanYourTV můžete nastavení svěřit asistentovi s přístupem
k terminálu (Claude Code, Codex, Gemini CLI). Na boxu zapněte SSH a vložte
asistentovi obsah [PROMPT.md](PROMPT.md).

## Ovládání

| Ovladač / klávesnice | Domovská obrazovka | V aplikaci |
| --- | --- | --- |
| Šipky, OK (Enter) | pohyb, spuštění | předá se aplikaci |
| Zpět (Esc) | odchod z mřížky, zavření panelů | předá se aplikaci |
| Domů (`XF86HomePage`, `F12`, `Super+H`) | – | zavře aplikaci |
| Menu | nastavení | předá se aplikaci |
| Hlasitost | hlasitost (`mixer`) | hlasitost |

Ovladač v mobilu: na TV otevřete **Settings** a adresu odtud zadejte v mobilu
ve stejné síti. Část `?t=` je párovací kód.

Z příkazové řádky (jako uživatel `tv` nebo přes SSH):

```sh
bsdtv-ctl apps
bsdtv-ctl launch kodi
bsdtv-ctl key down down ok
bsdtv-ctl text "dokumenty o přírodě"
bsdtv-ctl home
bsdtv-ctl theme aurora
bsdtv-ctl url
```

## Nastavení

- `/usr/local/etc/bsdtv/config.json`: systémové výchozí hodnoty (dok, motiv,
  adresa Jellyfinu, IPTV playlist, složka s hudbou, CEC, port ovladače).
- `~tv/.config/bsdtv/config.json`: uživatelské změny; sem zapisuje panel
  Settings.
- `apps.json`: katalog aplikací, `themes/<název>/theme.json`: motivy.

Všechny volby, přidání aplikace a tvorbu motivů popisuje
[docs/CONFIG.md](docs/CONFIG.md).

## Vývoj

Launcher běží všude, kde je PySide6:

```sh
python3 -m venv .venv && .venv/bin/pip install PySide6-Essentials pytest
.venv/bin/python -m bsdtv --windowed
.venv/bin/python -m pytest
```

## Licence

BSD 2-Clause, viz [LICENSE](LICENSE).
