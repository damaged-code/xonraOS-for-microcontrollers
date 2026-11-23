# App server

The board's **Store** app downloads apps from a plain web server. No special software is needed: any computer on the same Wi-Fi can serve this folder.

```
server/
  index.html       the website that lists the apps with live previews
  manifest.txt     one line per app:  id|Name|version|Category
  apps/<id>.js     the apps (first line is the header //Name|version|Short)
  notes.txt        used by the Notes sample app
```

## Run it

From this `server/` folder:

```
python -m http.server 8266
```

(`python3` on Linux and macOS.) Then put the computer's address in `firmware/xonraOS/secrets.h`, for example `#define STORE_URL "http://192.168.1.50:8266"`, and flash the firmware. Find the address with `ipconfig` (Windows) or `ip addr` / `ifconfig`. The board and the computer must be on the same network, and the computer's firewall must allow port 8266.

Check it from a browser: `http://<address>:8266/manifest.txt` should show the list.

For an always-on server, run the same command on a Raspberry Pi, a home server or a NAS (for example as a systemd service).

## Add or update an app

1. Put `<id>.js` in `apps/` (see `../docs/APPS.md` for how to write one).
2. Add a line to `manifest.txt`: `id|Name|version|Category`.
3. On the board: Store, find the app, hold the main button. `GET` installs, `UPD` updates, `OK` removes.

**Bump the version in both the app header and the manifest every time you change an app**, or the board will not offer the update.

`Category` is `Tools`, `Games`, `Timers` or `Sound` (anything else, or leaving it out, puts the app under `Other`). The Store groups apps under those headings and the website shows them as sections. Firmware older than V2.0.5 ignores the fourth field and shows one flat list.

## Security

The board downloads plain `http://` and runs whatever script it gets, but scripts only draw on the screen, read the buttons and the knob, make web requests and save one short string. Still, only serve apps you trust, and do not expose this server to the internet.

The files in this folder are under the MIT license (see `LICENSE`).
