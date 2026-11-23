# xonraOS case (for the xonraOS board)

A small two-part case for the custom xonraOS board ([../README.md](../README.md)). Outside size **64.6 × 92.6 × 12.6 mm** (portrait), about 28 g of filament. On top only the 1.8" colour screen and two big round buttons; the scroll wheel sticks out of the right side.

> For the NodeMCU + jumper-wire build there is a bigger case in [../case-nodemcu](../case-nodemcu).

| File | What it is |
|---|---|
| `base.stl` | the bottom: 3 posts with pins that locate the board, clip grooves, micro-USB opening (left side), wheel slot and thumb scoop in the right side |
| `lid.stl` | the top: screen panel opening, two round button openings, six snap-fit clips. No screw or vent holes |
| `button-cap-main.stl` | main button cap, 16 mm (print in light grey) |
| `button-cap-second.stl` | second button cap, 12 mm (print in black) |
| `lid-name.stl` | optional insert that fills the engraved XONRA, for printing the name in a second colour (light grey) on a multi-colour printer |
| `screen-panel.stl` | black panel around the screen, set flush into the lid (glossy look, like a phone front) |
| `screen-preview.stl` | the screen glass, for viewing only |
| `cap-main-preview.stl`, `cap-second-preview.stl` | caps in place, for viewing only |
| `wheel-preview.stl` | the scroll wheel, for viewing only |
| `board-preview.stl` | a block model of the board, for viewing only (don't print) |
| `layout.svg` | where each part sits on the board (opens in Figma) |
| `xonra-board-case.js` | the editable model: board outline, part positions and the case |

## Look

Designed from research on premium product design: see [DESIGN-NOTES.md](DESIGN-NOTES.md).

- **Colours:** one matte graphite colour for lid and base (tone-on-tone), a glossy black screen panel, a light-grey main button as the single accent, black second button, silver scroll wheel.
- **Shape:** rounded corners and filleted edges, a shadow line where the lid meets the base, a thumb scoop around the side scroll wheel, a pill-shaped micro-USB opening with a recess for the plug, no decorative patterns.
- **XONRA** is engraved on the front, centred below the buttons (Engravers MT font as a stand-in; change `LOGO.font` in the script). The underside is completely plain.

## Print it

PLA or PETG, 0.2 mm layers, 15–20 % infill, no supports. Print every part the way it loads: the lid is already upside down. About 1.5 hours in total. A textured PEI bed gives the lid top a premium matte finish.

## Put it together

Before you start: **trim the solder legs** of the screen header, the two big buttons and the thumbwheel to about 1.5 mm under the board (flush cutters), so the board sits flat on the low posts. Use **low-profile 12 × 12 × 4.3 mm** buttons.


1. Lay the carrier board (`../carrier`) on the three posts in the base, the D1 mini's USB toward the opening in the left side.
2. Glue the black screen panel into its opening in the lid (a few dots of superglue on the ledge). Drop the button caps (big one on the right, small one on the left) into their openings in the lid (flange underneath), then put the lid on. Check the wheel rim comes out of the right-side slot.
3. Press the lid straight down until all six clips click into the base. **No screws**: the underside stays smooth. The lid's posts hold the board down on the pins of the base.

To open it: push a fingernail or a plastic card into the seam at the bottom end and lift. The clips are ramped so they let go; they wear a little if you open it very often.

RESET and FLASH are inside, not reachable when closed: flashing over USB resets the board automatically. The buzzer has no holes of its own; its sound comes out through the wheel slot and the gaps around the buttons (a little quieter than with holes).

## Status

The board is a **placement draft**: the copper routing still has to be done in KiCad using this outline and these positions. The case was checked on screen against the board model (holes, screen window, USB opening), but **nothing has been printed or built yet**. When the real board exists, update the part positions in `xonra-board-case.js` to match it and run:

```
npm install @jscad/modeling@2.12.5 @jscad/stl-serializer@2.1.20
node xonra-board-case.js
```

License: CERN-OHL-S-2.0, same as the rest of `hardware/`.
