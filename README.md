# esp8266-oled-os

A tiny phone-style operating system for an **ESP8266 (NodeMCU)** with a **128x64 OLED** and **two buttons**. It has a Home screen, a Wi-Fi clock, two games, Settings, and an **app Store** that downloads small JavaScript apps from your own server. Anyone can write an app in a few lines and publish it by copying a file.

> Status: made by a beginner as a learning project and tested on one NodeMCU (ESP-12E, CP2102) board. Issues and pull requests are welcome.

## What is in it

- **Home:** apps in a smooth scrolling grid.
- **Clock:** time and date from the internet (NTP), 12 or 24 hour.
- **Jump** and **Snake:** two games with high scores and sound.
- **Settings:** time format, brightness, seconds, animations, what the second button does, sound, and **Wi-Fi setup from your phone** (the board opens its own network and a web page).
- **Store:** lists the apps on your server. Hold a button to install, update or remove one. Installed apps appear on Home.
- **Apps are JavaScript** ([Elk](https://github.com/cesanta/elk) engine). They can draw, read the buttons and a knob, play beeps, fetch web pages and save a value. See [docs/APPS.md](docs/APPS.md).
- Apps are loaded into memory only while open, and many apps can be installed at once.

Sample apps in `server/apps/`: Counter, Stopwatch, Notes (scrolling text from your server), Knob Test, Bounce, Calculator.

## Hardware

| Part | Connects to |
|---|---|
| ESP8266 NodeMCU (ESP-12E) | USB |
| 0.96" SSD1306 OLED, I2C address 0x3C | VCC to 3V3, GND to GND, SCL to D1, SDA to D2 |
| Main button | D7 and GND |
| Second button | D6 and GND |
| Buzzer (optional) | + to D8, - to GND |
| Potentiometer (optional) | left pin 3V3, middle pin A0, right pin GND |

Buttons use the internal pull-up, so no resistors are needed. A wiring diagram for a breadboard is in [docs/wiring.html](docs/wiring.html) (open it in a browser). An editable SVG copy for Figma or Inkscape is in [docs/wiring.svg](docs/wiring.svg).

## Controls

- **Main button (D7):** tap = next / action, hold 0.6 s = select, hold 1.5 s = back to Home.
- **Second button (D6):** set in Settings > D6 button: Off, Home, or Back (previous item, turn left, jump).

## Build and flash

1. Install the [Arduino IDE](https://www.arduino.cc/en/software) or Arduino CLI, and the **esp8266** board package (3.1.2 was used).
2. Install the libraries **Adafruit SSD1306**, **Adafruit GFX Library** (with Adafruit BusIO) and **elk** (2.2.0) from the Library Manager.
3. Copy `firmware/WifiClock/secrets.example.h` to `secrets.h` in the same folder and fill in your Wi-Fi name, password and app server address. (The ESP8266 supports 2.4 GHz Wi-Fi only.)
4. Edit `TIME_ZONE` near the top of `WifiClock.ino` if you are not in UTC+3.
5. Board: **NodeMCU 1.0 (ESP-12E Module)**. Compile and upload, for example:

```
arduino-cli compile --upload -p COM5 --fqbn esp8266:esp8266:nodemcuv2 firmware/WifiClock
```

If an upload fails with a write error, try again. If the board is not detected, install the CP210x USB driver.

## Run the app server

```
cd server
python -m http.server 8266
```

Use the computer's LAN address as `STORE_URL` in `secrets.h`. Details: [server/README.md](server/README.md).

## Write an app

```js
//Hello|1|Hello
let n = 0;
let draw = function() {
  ctext(8, 1, 'Hello');
  ctext(24, 3, str(n));
  ctext(54, 1, 'Tap = +1');
};
let tap = function() { n = n + 1; };
```

Save it as `server/apps/hello.js`, add `hello|Hello|1` to `server/manifest.txt`, and install it from the Store. Read [docs/APPS.md](docs/APPS.md) first: the script engine has some bugs that fail silently, and there is a test harness (`tools/harness.c`) to check your app on your computer.

## Repository layout

```
firmware/WifiClock/   the OS (Arduino sketch)  - GPL-3.0
server/               app server files and sample apps - MIT
tools/harness.c       test an app on your computer
docs/                 app guide and wiring diagram
```

## License and credits

- The firmware and tools are under the **GNU GPL v3** (`LICENSE`), because they include the [Elk](https://github.com/cesanta/elk) JavaScript engine by Cesanta (AGPL/GPL). The sample apps and server files in `server/` are under the **MIT** license (`server/LICENSE`), so you can license your own apps as you like.
- Built on [Elk](https://github.com/cesanta/elk), the [ESP8266 Arduino core](https://github.com/esp8266/Arduino), and Adafruit's SSD1306, GFX and BusIO libraries.
