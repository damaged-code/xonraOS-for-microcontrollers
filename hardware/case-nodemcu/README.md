# xonraOS case for the NodeMCU build (0 SAR in parts)

A case for the build most people already have: **NodeMCU on a breadboard, 0.96" OLED, two 6×6 mm buttons, a buzzer
and a 16 mm knob potentiometer**, connected with male-to-female jumper wires. Nothing to buy except the print.

Same look as the custom-board case in `../case`: rounded edges, a shadow line where the lid meets the base, a
snap-fit lid (no screws), big round buttons, a scroll wheel on the right side and **XONRA** on the front.

Outside size: **59.8 × 87.3 × 37.6 mm**. It is thick because jumper wires need room. A soldered build can be much thinner (see `../carrier`).

| File | Print | What it is |
|---|---|---|
| `base.stl` | 1 | the box: holds the breadboard, USB opening at the bottom, buzzer grille on the left, wheel hole on the right |
| `lid.stl` | 1 | the top: screen window, button holes, OLED holder underneath (already lying top face down) |
| `button-cap-main.stl`, `button-cap-second.stl` | 1 each | big round caps (main 16 mm, second 12 mm) |
| `button-holder-main.stl`, `button-holder-second.stl` | 1 each | hold the 6×6 buttons under the caps; glued under the lid |
| `wheel-knob.stl` | 1 | knurled wheel that presses onto the pot's shaft |
| `lid-name.stl` | optional | the XONRA letters, for a second colour |
| `inside-preview.stl` | no | the parts inside, for viewing only |
| `case-drawing.svg` | — | top view with sizes (opens in Figma) |
| `xonra-case.js` | — | the editable model |

## Print it

- **Colours:** matte black or dark grey for base and lid; light grey for the main cap; black for the rest.
- PLA or PETG, 0.2 mm layers, 15–20 % infill, **no supports**. Print every part exactly as it loads.
- Cheap options: anyone with a 3D printer, or a Haraj printing seller (often 20–40 SAR for parts this size).

## Put it together

1. **Breadboard:** peel the backing and stick your breadboard to the floor of the base, with the NodeMCU's USB
   end at the **bottom** (next to the opening). Push the NodeMCU as close to that end as it goes.
2. **Screen:** put the OLED face up into the frame under the lid (pins toward the top end), so the screen shows
   through the window. A dot of hot glue at two corners. Female ends of the jumper wires go on its pins.
3. **Buttons:** drop each cap into its hole from underneath (the flange stops it). Glue a 6×6 button into the
   square socket of its holder, stem through the small hole, then glue the holder onto the ring under the lid.
   Bend the button legs sideways and put the female wire ends on them.
4. **Wheel:** push the potentiometer's threaded part through the hole in the right side from inside, tighten its
   nut outside, and press the wheel knob onto the shaft (flat side matching).
5. **Buzzer:** hot-glue it inside the left wall, behind the small holes.
6. Plug the male wire ends into the breadboard exactly as before (wiring: `docs/wiring-table.svg`).
7. Close the lid: it clicks in. To open, push a fingernail into the seam at the bottom end and lift.

## Before you print: measure

Change these at the top of `xonra-case.js` (block `P`), then run it again:

- `bb`: breadboard size (default the 400-point half size, 82.5 × 55 × 8.5 mm).
- `node`: NodeMCU size (default the CH340 "v3", 58 × 31.5 mm; the "Amica" CP2102 one is 49 × 26).
- `oled`: module size and where the picture sits (`winOffY`).
- `btn`: your button's body and total height (6×6×5 mm by default).
- `pot`: the knob pot (WH148-style: 16 mm body, 7 mm thread, 6 mm D-shaft).

```
npm install @jscad/modeling@2.12.5 @jscad/stl-serializer@2.1.20 opentype.js@1.3.4
CHECK=1 node xonra-case.js
```

The fit check (parts inside vs base and lid, caps, holders) passes. **It has not been printed yet**: print the lid
first and test the screen and buttons.

License: CERN-OHL-S-2.0, same as the rest of `hardware/`.
