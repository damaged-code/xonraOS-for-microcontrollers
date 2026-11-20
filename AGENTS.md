# AGENTS.md (xonraOS)

Instructions for AI coding agents working in this repository.

## Writing apps (most common task)
Read [docs/APPS.md](docs/APPS.md) before writing or editing any app in `server/apps/`. The Elk JavaScript engine has bugs that fail silently (bare `if (x)`, `return` inside `if`, ternaries). Always run the app through `tools/harness.c` first and check that the drawing calls you expect are printed with no `ERROR`. Bump the version in the app header and in `server/manifest.txt` on every change. Write files with a file tool, not shell heredocs (quote characters get lost). You cannot press the board's buttons, so say plainly that an app is not tested on the real board.

## Firmware (`firmware/WifiClock/WifiClock.ino`)
- ESP8266 NodeMCU, 80 KB RAM, one analog pin (A0), 3.3 V pins. Avoid D3, D4, D8 for buttons (boot pins). The built-in LED is active low.
- Never commit `secrets.h` (Wi-Fi password). It is git-ignored; `secrets.example.h` is the template.
- Build: `arduino-cli compile --upload -p <port> --fqbn esp8266:esp8266:nodemcuv2 firmware/WifiClock`. A mid-write upload error is usually transient: retry.
- Apps and big buffers are loaded only while open (`appEnter` / `appLeave`); free heap memory when an app closes.
- To add a function for apps, register it in `scriptLoad()` with signature letters `i`, `s`, `v` only (doubles break Elk's call convention), then update the harness and `docs/APPS.md`.

## Licenses
Firmware and tools: GPL-3.0. `server/`: MIT. Keep the license headers and credits for Elk.
