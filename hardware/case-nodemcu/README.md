# xonraOS printable case

A two-part desk case for the build without a breadboard: NodeMCU, 0.96" OLED, two buttons, buzzer and knob, wired with jumper wires.

| File | What it is |
|---|---|
| `base.stl` | the box: holds the NodeMCU, has the USB opening and sound slots |
| `lid.stl` | the top: screen window, two button holes, knob hole |
| `case-drawing.svg` | top and side drawing with hole sizes (opens in Figma) |
| `xonra-case.js` | the editable model. Change a number, run it, get new STL files |

Outside size: **80 × 56 × 38.5 mm**. Uses about 45 g of filament.

## Print it

- **Material:** PLA or PETG. **Layer height** 0.2 mm, **infill** 15–20 %, **no supports**.
- **Base:** open side up. **Lid:** already lying top face down in the file. Print it exactly as it loads.
- Time: about 3–4 hours for both parts on a normal printer.

## Parts to buy for the case

| Part | Note |
|---|---|
| 2 × tactile buttons **6×6 mm with a tall stem (12 or 13 mm)** | short breadboard buttons don't reach through the lid |
| 10k **panel** potentiometer (B10K, 16 mm, with nut) + a knob cap | small breadboard trim-pots don't fit the hole |
| 3 × M3 screws, 8 mm (self-tapping) | optional: the lid also holds by friction |
| Female-to-female jumper wires | the NodeMCU's pins point up inside |
| Hot glue | holds the screen, buttons and buzzer |

## Put it together

1. **Screen:** press the OLED, face down, into the frame under the lid so the screen sits in the window. Pins toward the back. Add a dot of hot glue at each corner.
2. **Buttons:** push each button into its square pocket under the lid so the stem comes out of the hole, then glue. The right hole is the main button (D7), the left one is D6.
3. **Knob:** push the potentiometer through the round hole from below, tighten its nut on top, put the knob cap on.
4. **Buzzer:** glue it inside the left wall, against the sound slots.
5. **NodeMCU:** lay it **upside down** (pins pointing up) on the four little blocks, with the USB port in the opening on the left.
6. **Wire** everything exactly as in the wiring table (`docs/wiring-table.svg`), using female-to-female wires on the pins.
7. Close the lid and add the 3 screws.

## Before you print: measure

Parts from different sellers differ by a millimetre or two. Measure yours and change these values at the top of `xonra-case.js` if needed:

- `nodeL`, `nodeW`: NodeMCU length and width (the CH340 "V3" board is bigger, about 58 × 31 mm).
- `oledW`, `oledH`, `winOffY`: OLED board size and where the screen sits on it.
- `fit`: raise it to 0.35 if the lid is too tight, lower it if it is loose.

Then rebuild:

```
npm install @jscad/modeling@2.12.5 @jscad/stl-serializer@2.1.20
node xonra-case.js
```

The model was checked on screen (parts, holes and lid alignment). **It has not been printed yet**, so print the lid first (it's quick) and test-fit the screen and buttons.

License: CERN-OHL-S-2.0, same as the rest of `hardware/`.
