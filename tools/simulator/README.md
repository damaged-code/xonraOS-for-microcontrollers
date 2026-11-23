# xonraOS desktop simulator (Windows)

Runs the **real** TFT firmware (`firmware/xonraOS_TFT/xonraOS_TFT.ino`) on your PC, in a window that shows the
1.8" 160 × 128 screen. No board, screen or parts needed. The firmware, Adafruit GFX and the Elk app engine are
compiled unchanged; small stand-ins in `include/` replace the ESP8266 parts (Wi-Fi, flash, EEPROM, HTTP, the TFT driver).

## Controls

| Key / mouse | Does |
|---|---|
| Space or left click | main button (tap = next, hold 0.6 s = open, hold 1.5 s = Home / back) |
| B or right click | second button (D6) |
| Up / Down arrows or mouse wheel | the scroll wheel (knob) |
| Esc | quit |

The buzzer plays through the PC speaker.

## What is simulated

- **Wi-Fi:** always connected ("Simulator Wi-Fi"); time comes from your PC clock.
- **Store:** reads the repository's `server/` folder instead of the internet, so the real Store apps install and run.
- **Flash and settings:** kept in `build/simfs/` (delete that folder to start fresh).
- **Not simulated:** real Wi-Fi setup from a phone, firmware updates, colours/speed of the real screen.

## Build

Needs MSYS2 (`g++`) and the Arduino libraries "Adafruit GFX Library" and "elk" (installed with the Arduino IDE or `arduino-cli lib install`).

```
bash tools/simulator/build.sh
tools/simulator/build/xonraos-sim.exe
```

Set `ARDUINO_LIBS` if your Arduino libraries folder is not in `Documents\Arduino\libraries`.

## Automatic tests

`XONRA_SIM_SCRIPT` drives the buttons and saves screenshots, for example:

```
XONRA_SIM_MUTE=1 XONRA_SIM_SCRIPT="w800 s:home.bmp t t t h w2000 s:store.bmp q" tools/simulator/build/xonraos-sim.exe
```

`wN` wait N ms, `t` tap, `h` hold 0.8 s, `H` hold 1.7 s, `b` second button, `kN` knob value, `s:FILE` screenshot, `q` quit.
`XONRA_SIM_SERVER` points the Store at another folder.
