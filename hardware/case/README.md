# xonraOS case (for the xonraOS board)

A small two-part case for the custom xonraOS board ([../README.md](../README.md)). Outside size **64 × 92 × 16 mm** (portrait), about 28 g of filament. On top only the 1.8" colour screen and two big round buttons; the scroll wheel sticks out of the right side.

> For the NodeMCU + jumper-wire build there is a bigger case in [../case-nodemcu](../case-nodemcu).

| File | What it is |
|---|---|
| `base.stl` | the bottom: 3 posts the board sits on, USB-C opening at the bottom edge, wheel slot in the right side |
| `lid.stl` | the top: screen window and two round button openings. No screw or vent holes |
| `button-cap.stl` | big round button cap: **print 2** |
| `screen-preview.stl` | the screen glass, for viewing only |
| `caps-preview.stl` | both caps in place, for viewing only |
| `wheel-preview.stl` | the scroll wheel, for viewing only |
| `board-preview.stl` | a block model of the board, for viewing only (don't print) |
| `layout.svg` | where each part sits on the board (opens in Figma) |
| `xonra-board-case.js` | the editable model: board outline, part positions and the case |

## Look

- **Colours:** matte black base, grey (or silver) lid; the scroll wheel looks best in silver/chrome.
- **Details:** a faceted triangle band on the back and side walls, a bevelled edge around the screen window, and **XONRA** engraved under the base.
- The name uses the Engravers MT font (ENGR.TTF, installed with Microsoft Office) as a stand-in for the real logo lettering. Change `LOGO.font` in the script to use another font file.

## Print it

PLA or PETG, 0.2 mm layers, 15–20 % infill, no supports. Print every part the way it loads: the lid is already upside down. About 1.5 hours in total. A textured PEI bed gives the lid top a premium matte finish.

## Put it together

1. Lay the board on the four posts in the base, USB-C toward the opening at the bottom edge.
2. Drop the two button caps into their openings in the lid (flange underneath), then put the lid on. Check the wheel rim comes out of the right-side slot.
3. Turn it over and fix it with 3 × **M2 × 12 mm flat-head (countersunk) screws** from underneath. They go up through the base and the board into the lid, and their heads sit flush.

RESET and FLASH are inside, not reachable when closed: flashing over USB resets the board automatically. The buzzer has no holes of its own; its sound comes out through the wheel slot and the gaps around the buttons (a little quieter than with holes).

## Status

The board is a **placement draft**: the copper routing still has to be done in KiCad using this outline and these positions. The case was checked on screen against the board model (holes, screen window, USB opening), but **nothing has been printed or built yet**. When the real board exists, update the part positions in `xonra-board-case.js` to match it and run:

```
npm install @jscad/modeling@2.12.5 @jscad/stl-serializer@2.1.20
node xonra-board-case.js
```

License: CERN-OHL-S-2.0, same as the rest of `hardware/`.
