# xonraOS on Raspberry Pi 3B

The same OS and the same JavaScript apps as the ESP8266 board, running on a Raspberry Pi 3 Model B with a 0.96" SSD1306 OLED. Apps from the Store work on both, because the Pi uses the same Elk engine, the same 12 KB app memory and the same functions ([docs/APPS.md](../docs/APPS.md)).

What you get: Home, Clock, Store and every installed app. What is different from the ESP8266 build: no Snake or Jump, and no Settings screen (edit `config.txt` instead). Wi-Fi is set up in Raspberry Pi OS, not in xonraOS.

## Wiring

BCM numbers are the GPIO names; pin numbers are the physical header position.

| Part | Connect to |
|---|---|
| OLED GND / VCC | pin 6 (GND) / pin 1 (3.3 V) |
| OLED SDA / SCL | pin 3 (GPIO 2) / pin 5 (GPIO 3) |
| Main button | GPIO 17 (pin 11) to GND |
| Second button | GPIO 27 (pin 13) to GND |
| Buzzer + | GPIO 18 (pin 12), buzzer - to GND |
| Knob (optional) | MCP3008 on SPI0: CLK GPIO 11, MISO GPIO 9, MOSI GPIO 10, CS GPIO 8 (CE0), knob middle pin on channel 0 |

Buttons need no resistors (the Pi's internal pull-ups are used). The Pi has no analog input, so `knob()` returns 512 unless you add an MCP3008 chip and set `knob=0`. Use a passive buzzer.

## Install

1. In `sudo raspi-config` enable **I2C** (and **SPI** if you use the knob), then reboot.
2. Get the code and Elk (Elk is AGPL-3.0, so it is fetched, not bundled):
   ```
   sudo apt install -y gcc make git
   git clone https://github.com/damaged-code/xonraOS-for-microcontrollers
   git clone https://github.com/cesanta/elk
   cd xonraOS-for-microcontrollers/pi
   make ELK=~/elk
   ```
3. Create the config: `mkdir -p ~/.xonraos && cp config.example.txt ~/.xonraos/config.txt`, then set `server=` to your app server.
4. Run `./xonra`. Controls are the same as the ESP8266: tap = next, hold 0.6 s = open, hold 1.5 s = Home, second button = back.

Add your user to the `gpio`, `i2c` and `spi` groups if a permission error appears. Start at boot with [xonraos.service](xonraos.service). Apps live in `~/.xonraos/apps`, app data in `~/.xonraos/d_<id>.txt`.

## Try it without a Pi

`make sim ELK=path/to/elk` builds a text-screen version for any computer. It reads one letter per command: `t` tap, `h` hold, `H` hold 1.5 s, `b` second button, `.` next frame, `p` print the screen, `q` quit. With `XONRA_SIM_SERVER=../server` the Store reads this repo's `server/` folder instead of a web server.

```
XONRA_HOME=./simhome XONRA_SIM_SERVER=../server sh -c "printf 'thhtp' | ./xonra-sim"
```

## Notes

- The Pi version speaks plain `http://` only. Use the server's IP or an http address, not an https one.
- Written for the Pi 3B (BCM2837). The pin code talks to `/dev/gpiomem` directly and will not work on a Pi 5.
- The hardware part (OLED, buttons, buzzer, knob) has not yet been run on a real Pi; the apps, Store and screen drawing were tested with the simulator.
