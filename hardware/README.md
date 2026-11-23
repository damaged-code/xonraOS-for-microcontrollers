# xonraOS board (open hardware)

> **Current design: [carrier/](carrier/README.md)** — v0.3, low-cost board for a Wemos D1 mini, hand-soldered, fits the case. The factory-assembled v0.2 board is in [pcb/](pcb/README.md). The text below describes the earlier v0.1 draft (0.96" OLED) and is kept for reference.

A single board that runs the xonraOS firmware with no breadboard: ESP8266, USB-C, a 0.96" OLED, two buttons, a buzzer and a knob. It is pin-compatible with the NodeMCU wiring the firmware already uses, so **the same firmware runs unchanged**.

> **Status: design draft, not built yet.** This folder has the circuit (netlist below), the parts list ([bom.csv](bom.csv)) and a schematic drawing ([schematic.svg](schematic.svg)). The PCB layout (KiCad) is the next step. Check every part number on LCSC before ordering; they were chosen from memory and stock changes.

License: CERN-OHL-S-2.0 (see [LICENSE](LICENSE)). Anyone may build and sell it; changed versions must stay open. The name "xonraOS" is not covered: clones should say "compatible with xonraOS".

## Blocks

```
USB-C ──5V──> AMS1117-3.3 ──3V3──> ESP-12F, CH340C, OLED
  │ D+/D-                              │
  └──> CH340C ──TX/RX──> ESP-12F <─────┘
          DTR/RTS ──> auto-reset (Q1, Q2) ──> EN, GPIO0
ESP-12F GPIO4/5 ──I2C──> OLED header
ESP-12F GPIO13 ──> main button      GPIO12 ──> second button
ESP-12F GPIO15 ──> Q3 ──> buzzer    TOUT (A0) <── divider <── knob
```

## Pins (same as the NodeMCU build)

| Function | ESP-12F pin | NodeMCU name |
|---|---|---|
| OLED SDA | GPIO4 | D2 |
| OLED SCL | GPIO5 | D1 |
| Main button | GPIO13 | D7 |
| Second button | GPIO12 | D6 |
| Buzzer | GPIO15 | D8 |
| Knob | TOUT (ADC) | A0 |

## Netlist (the circuit)

Every net and what is on it. Designators match [bom.csv](bom.csv).

**Power**
- `VBUS` (5 V): J1 VBUS pins (A4, A9, B4, B9) → U3 VIN, C1 (+)
- `3V3`: U3 VOUT → C2, C3, C4, C5, C6; U1 VCC; U2 VCC and V3; J2 VCC; pull-ups R1, R2, R3, R4; RV1 top end; D2 (power LED) via R14
- `GND`: J1 GND pins and shell, U3 GND, all capacitors, U1 GND, U2 GND, J2 GND, R5, R6, R7, R12 lower ends, Q3 emitter, RV1 bottom end, all buttons' second leg, D2 cathode

**USB-C**
- J1 CC1 → R6 (5.1k) → GND; J1 CC2 → R7 (5.1k) → GND (tells the charger to give 5 V)
- J1 D+ (A6 and B6 joined) → U2 UD+; J1 D- (A7 and B7 joined) → U2 UD-

**USB-serial (CH340C, powered at 3.3 V so its signals are safe for the ESP)**
- U2 TXD → U1 RXD0 (GPIO3); U2 RXD ← U1 TXD0 (GPIO1)
- U2 VCC and V3 tied to 3V3, C5 100 nF next to them

**Auto-reset (flash without pressing buttons)**
- U2 DTR → R8 (10k) → Q1 base; Q1 emitter → U2 RTS; Q1 collector → `EN`
- U2 RTS → R9 (10k) → Q2 base; Q2 emitter → U2 DTR; Q2 collector → `GPIO0`

**ESP-12F boot pins**
- `EN` → R1 (10k) → 3V3; C7 (100 nF) → GND
- `RST` → R2 (10k) → 3V3; SW3 (RESET) → GND
- `GPIO0` → R3 (10k) → 3V3; SW4 (FLASH) → GND
- `GPIO2` → R4 (10k) → 3V3
- `GPIO15` → R5 (10k) → GND (must be low at boot)

**OLED header J2** (4-pin, 2.54 mm)
- GND, VCC (3V3), SCL (GPIO5), SDA (GPIO4)
- Pull-ups are on the OLED module itself. **OLED modules come in two pin orders (GND-VCC or VCC-GND). Print the order on the silkscreen and check your module.**

**Buttons** (firmware uses the internal pull-ups)
- SW1 (main): GPIO13 → GND
- SW2 (second): GPIO12 → GND

**Buzzer**
- GPIO15 → R10 (1k) → Q3 base; Q3 collector → LS1 (−); LS1 (+) → 3V3; D1 (1N4148) across LS1, cathode to 3V3
- R5 also keeps Q3 off during boot.

**Knob (ADC accepts only 0–1 V)**
- RV1 ends: 3V3 and GND; wiper → R11 (220k) → `TOUT`; `TOUT` → R12 (100k) → GND
- Same divider as the NodeMCU, so `knob()` reads 0–1023 like before.

**Power LED**
- 3V3 → R14 (1k) → D2 → GND

## Board size and part placement

Board: **60 × 88 mm** (portrait), 1.6 mm thick, 3 × M2 holes at (3, 3), (57, 3), (3, 46) from the bottom-left corner. Placement drawing: [case/layout.svg](case/layout.svg); exact positions are the PARTS list in [case/xonra-board-case.js](case/xonra-board-case.js), which also builds the matching case (64 × 92 × 16 mm).

- **Screen:** 1.8" ST7735 colour TFT module (160×128) at the top, landscape, soldered on an 8-pin header (J2) with a 2.5 mm spacer. Only low parts (CH340C, AMS1117, resistors, capacitors) go under it.
- **Middle:** two big round buttons (D6 left, D7 right): 12×12×5 mm tactile switches under printed 14 mm caps.
- **Right side:** the scroll wheel, a flat 16 mm thumbwheel potentiometer lying on the board; its rim sticks out of a slot in the right wall and is rolled up/down with the thumb.
- **Bottom:** ESP-12F turned sideways (antenna on the right edge), buzzer, RESET, FLASH, power LED, and **USB-C** on the bottom edge.
- Parts under the lid must stay below 7 mm. The wheel must sit with its rim 8 mm from its centre, centre 4 mm from the right board edge, about 4 mm above the board.

> **Netlist update pending:** the circuit above still describes the 0.96" OLED on I2C. The TFT uses SPI (SCK GPIO14, MOSI GPIO13, CS GPIO5, DC GPIO4) and moves the main button from GPIO13 to GPIO0; this will be updated together with the firmware.

## Notes for the layout (next step)

- Keep the ESP-12F antenna end over the board edge with **no copper or ground under it**.
- Put C3/C4 close to U1 VCC, C5 close to U2.
- Parts marked "hand" in bom.csv (OLED module, D6/D7 buttons, scroll wheel) are through-hole and soldered by hand; the rest can be machine-assembled (JLCPCB/PCBWay).
- Optional later: battery (LiPo + TP4056 charger + switch).

## Cost estimate (SAR, my estimate, not a quote)

About 70–130 SAR in parts per board, 300–600 SAR for the first prototype run including PCBs and assembly setup, and roughly 60–100 SAR per board at 100+ units.
