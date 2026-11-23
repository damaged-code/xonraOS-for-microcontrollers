// xonraOS board + compact case: board outline and part placement, a preview model of the board,
// and a two-part printable case around it.
//
//   npm install @jscad/modeling@2.12.5 @jscad/stl-serializer@2.1.20
//   node xonra-board-case.js
// writes: base.stl, lid.stl (print these), board-preview.stl (for viewing only), layout.svg (Figma)
//
// Board coordinates: millimetres, origin at the board's bottom-left corner, x to the right, y up,
// z = 0 on the board's top surface. Edit PARTS / BOARD and re-run; the case follows the board.
// License: CERN-OHL-S-2.0 (see ../LICENSE).

const fs = require('fs');
const path = require('path');
const { primitives, booleans, transforms, measurements } = require('@jscad/modeling');
const stl = require('@jscad/stl-serializer');
const { cuboid, cylinder, roundedCuboid, polygon } = primitives;
const { union, subtract } = booleans;
const { translate, rotateX, rotateY, rotateZ, mirrorX } = transforms;
const { extrudeLinear } = require('@jscad/modeling').extrusions;
const { hull } = require('@jscad/modeling').hulls;

// ---------- look ----------
// The logo is cut from a font outline. Engravers MT (ENGR.TTF, on Windows with Office) is a stand-in
// for the xonra logo; point LOGO.font at another .ttf/.otf, or set it to '' to leave the logo out.
// The name sits on the front, centred below the buttons (lidY = board coordinates), engraved into the lid top.
const LOGO = { text: 'XONRA', font: 'C:/Windows/Fonts/ENGR.TTF', lidCap: 3.6, lidY: 11, depth: 0.5, onLid: false };  // onLid: also the logo mark
// The xonra mark, traced from the logo art (units = pixels of the 1024 px logo image):
// a tall bar on the left, a square on the right, and a long bar along the bottom.
const MARK = { bbox: [212, 205, 810, 748], bars: [[212, 205, 394, 608], [212, 636, 810, 748]], square: [451, 262, 752, 549] };
const FACETS = { depth: 0.5, size: 6, gap: 0.7, z0: 3.4, h: 6.6 };  // triangle band on the walls
const BEVEL = 1;                                                        // chamfer around the screen window

function textShape(text, capH) {  // font outline -> 2D shape, centred on 0,0, cap height capH mm
  if (!LOGO.font || !fs.existsSync(LOGO.font)) { console.warn('logo font not found, logo skipped'); return null; }
  const ot = require('opentype.js');
  const font = ot.loadSync(LOGO.font);
  const scale = capH / ((font.tables.os2 && font.tables.os2.sCapHeight) || font.unitsPerEm * 0.7);
  const cmds = font.getPath(text, 0, 0, font.unitsPerEm * scale).commands;
  const contours = []; let cur = [], px = 0, py = 0;
  const add = (x, y) => { cur.push([x, -y]); px = x; py = y; };
  for (const c of cmds) {
    if (c.type === 'M') { if (cur.length > 2) contours.push(cur); cur = []; add(c.x, c.y); }
    else if (c.type === 'L') add(c.x, c.y);
    else if (c.type === 'Q') { const [x0, y0] = [px, py]; for (let t = 1; t <= 6; t++) { const u = t / 6, a = (1 - u) ** 2, b = 2 * (1 - u) * u, d = u * u; add(a * x0 + b * c.x1 + d * c.x, a * y0 + b * c.y1 + d * c.y); } }
    else if (c.type === 'C') { const [x0, y0] = [px, py]; for (let t = 1; t <= 8; t++) { const u = t / 8, a = (1 - u) ** 3, b = 3 * (1 - u) ** 2 * u, e = 3 * (1 - u) * u * u, d = u ** 3; add(a * x0 + b * c.x1 + e * c.x2 + d * c.x, a * y0 + b * c.y1 + e * c.y2 + d * c.y); } }
    else if (c.type === 'Z') { if (cur.length > 2) contours.push(cur); cur = []; }
  }
  const area = (p) => p.reduce((s, [x, y], i) => { const [x2, y2] = p[(i + 1) % p.length]; return s + x * y2 - x2 * y; }, 0) / 2;
  const inside = (pt, p) => { let r = false; for (let i = 0, j = p.length - 1; i < p.length; j = i++) { const [xi, yi] = p[i], [xj, yj] = p[j]; if ((yi > pt[1]) !== (yj > pt[1]) && pt[0] < (xj - xi) * (pt[1] - yi) / (yj - yi) + xi) r = !r; } return r; };
  const clean = contours.map((p) => p.filter((q, i) => i === 0 || Math.hypot(q[0] - p[i - 1][0], q[1] - p[i - 1][1]) > 1e-4)).filter((p) => Math.abs(area(p)) > 1e-3);
  const solids = [], holes = [];
  for (const p of clean) {
    const depth = clean.filter((o) => o !== p && inside(p[0], o)).length;  // inside an odd number of contours = hole
    const ccw = area(p) > 0 ? p : p.slice().reverse();
    (depth % 2 ? holes : solids).push(polygon({ points: ccw }));
  }
  let g = union(solids);
  if (holes.length) g = subtract(g, union(holes));
  const [[a, b], [c, d]] = measurements.measureBoundingBox(g);
  return translate([-(a + c) / 2, -(b + d) / 2, 0], g);
}

// The xonra mark as a 2D shape, centred on 0,0. height = mm. withText: XONRA inside the square.
function logoMark(height, withText) {
  const [bx0, by0, bx1, by1] = MARK.bbox, k = height / (by1 - by0);
  const X = (x) => (x - (bx0 + bx1) / 2) * k, Y = (y) => ((by0 + by1) / 2 - y) * k;  // image y points down
  const rect = ([x0, y0, x1, y1]) => polygon({ points: [[X(x0), Y(y1)], [X(x1), Y(y1)], [X(x1), Y(y0)], [X(x0), Y(y0)]] });
  const parts = MARK.bars.map(rect);
  const sq = MARK.square, stroke = Math.max(0.6, height * 0.035) / k;  // square drawn as a frame
  parts.push(subtract(rect(sq), rect([sq[0] + stroke, sq[1] + stroke, sq[2] - stroke, sq[3] - stroke])));
  if (withText) {
    const t = textShape(LOGO.text, (sq[3] - sq[1]) * k * 0.16);
    if (t) parts.push(translate([X((sq[0] + sq[2]) / 2), Y((sq[1] + sq[3]) / 2), 0], t));
  }
  return union(parts);
}

function facetBand(len, wallY) {  // alternating triangles cut into a wall, built for the back wall (+y)
  // a zig-zag row: triangles pointing up and down; every other one is cut, like the facets in the logo art
  const half = FACETS.size / 2, n = Math.floor(len / half) - 1, tris = [];
  const z0 = FACETS.z0, z1 = FACETS.z0 + FACETS.h, g = FACETS.gap;
  for (let i = 0; i < n; i += 2) {
    const x = -((n - 1) * half) / 2 + i * half;          // centre of an upward triangle
    tris.push(polygon({ points: [[x - half + g, z0 + g / 2], [x + half - g, z0 + g / 2], [x, z1 - g]] }));  // counter-clockwise
  }
  const solid = extrudeLinear({ height: FACETS.depth + 1 }, union(tris));
  return translate([0, wallY + 1, 0], rotateX(Math.PI / 2, solid));
}

// ---------- board ----------
// Built for the v0.3 carrier board (hardware/carrier): a Wemos D1 mini soldered flat, micro-USB on the left side.
const BOARD = { w: 60, d: 88, t: 1.6, holeD: 2.2, holes: [[3, 31], [57, 3], [3, 46]] };  // portrait: screen on top, big buttons below, wheel on the right side

// part: [designator, label, x, y, w, d, h, colour]  (x, y = centre on the board; h = height above the board)
const PARTS = [
  ['U1', 'D1 mini', 17.1, 14.5, 34.2, 25.6, 4.6, '#2b6cb0'],   // Wemos D1 mini lying flat, USB on the left edge
  ['J1', 'micro-USB', 3.0, 14.5, 6, 8, 4.6, '#c0c0c0'],        // its micro-USB port (opening in the left wall)
  ['SW2', 'D6', 19, 35.2, 12, 12, 4.3, '#333333'],              // low-profile 12x12x4.3 mm tactile switches under big printed caps
  ['SW1', 'D3', 41, 35.2, 12, 12, 4.3, '#333333'],
  ['LS1', 'Buzzer', 45, 15, 5, 5, 2, '#111111'],
  ['R1', '220R', 45, 21.5, 7.6, 1.8, 1.8, '#d9b26f'],
  ['RV1', 'Wheel', 55, 38, 8, 8, 2.5, '#2a6fdb'],               // flat thumbwheel pot; its wheel rim sticks out of the right side
  ['J2', 'TFT hdr', 2.5, 69.25, 2.5, 20.3, 2.5, '#111111'],
];
// 1.8" ST7735 TFT module (160x128, landscape), soldered on J2 (8 pins on its left short edge) with a 2.5 mm header.
// Sizes are typical for the common red 1.8" SPI module: measure yours. glass and win are relative to the module's corner.
const SCREEN = { x0: 1, y0: 52, w: 58, d: 34.5, z: 2.5, pcb: 1.6,
  glass: { x: 8.5, y: 0.25, w: 45.8, d: 34, h: 2.6 },        // panel outline
  win: { x: 31.4, y: 17.25, w: 36, d: 29 } };                 // active area 35 x 28 mm, plus a small margin
// Big round button caps (printed) on 12x12x5 mm tactile switches
// Clear hierarchy: the main button is bigger than the second one. Flange = cap + 2 mm, sits just under the lid.
const CAP = { main: 16, second: 12, flangeT: 0.8, postD: 5, postH: 0.5, z0: 4.5, top: 8.8, clearance: 0.6 };
const capSize = (id) => (id === 'SW1' ? CAP.main : CAP.second);
// Side scroll wheel: flat 16 mm thumbwheel potentiometer (10k) lying on the board; its rim sticks out of a slot in the right wall.
const WHEEL = { d: 16, t: 3, z: 4.0, x: 56, ridges: 32, slotL: 15, slotH: 3.6 };  // slot = wheel thickness + 0.3 mm each side

// ---------- case ----------
const C = {
  wall: 2.0, floor: 1.2, gap: 0.3, corner: 4.5,   // 2 mm walls: the usual minimum for stiff FDM enclosures
  edgeBottom: 1.0, edgeTop: 1.4, seam: 0.4,       // rounded (filleted) bottom and top edges; small chamfer on the lid's lower edge
  shadowH: 0.8, shadowD: 0.5,                     // shadow line: recessed step at the top of the base, under the lid
  scoopR: 14, scoopD: 1.0,                        // thumb scoop in the right side around the scroll wheel
  panelW: 54.4, panelD: 37, panelR: 3, ledge: 0.6,  // black screen panel set into the lid, centred left-right
  post: 2.0, postD: 4.6, pilot: 1.8,   // board rests on posts (trim solder legs to 1.5 mm); M2 screws come in from below into the lid
  above: 5.8,                          // board top to lid underside; the screen glass rises into a pocket in the lid
  glassPocket: 1.15,                   // how deep that pocket goes into the 2 mm lid (0.25 mm clearance over the glass)
  lid: 2, lipH: 2, lipT: 1.0, fit: 0.25,
  snapH: 0.6, snapL: 8, grooveD: 0.75,  // snap-fit bumps on the lid lip click into grooves in the base walls (no screws)
  pinD: 1.8,                           // pins on the posts locate the board through its mounting holes
  tubeD: 4.4, screwHole: 2.4, headD: 4.2, headH: 1.2, topSkin: 0.8,  // flat-head M2: countersunk under the base; lid top stays closed
  usbW: 9.0, usbH: 4.2,                // micro-USB plug metal opening (pill)
  usbRecessW: 11.8, usbRecessH: 7.0, usbRecessD: 1.4,  // outside recess so the cable's plastic body can reach in
};

const segs = 40;
const IW = BOARD.w + 2 * C.gap, ID = BOARD.d + 2 * C.gap;
const OW = IW + 2 * C.wall, OD = ID + 2 * C.wall;
const zBoard = C.floor + C.post;                 // board bottom
const zTop = zBoard + BOARD.t;                   // board top (z = 0 in board coordinates)
const zLid = zTop + C.above;                     // lid underside = top of the walls
const H = zLid + C.lid;
const wx = (x) => x - BOARD.w / 2, wy = (y) => y - BOARD.d / 2;  // board -> world
const part = (id) => PARTS.find((p) => p[0] === id);
const box = (w, d, h, x, y, z) => cuboid({ size: [w, d, h], center: [x, y, z + h / 2] });
const cyl = (dia, h, x, y, z) => cylinder({ radius: dia / 2, height: h, center: [x, y, z + h / 2], segments: segs });

function boardPreview() {
  const items = [box(BOARD.w, BOARD.d, BOARD.t, 0, 0, zBoard)];
  for (const [, , x, y, w, d, h] of PARTS) items.push(box(w, d, h, wx(x), wy(y), zTop));
  const o = SCREEN, g = o.glass;
  items.push(box(o.w, o.d, o.pcb, wx(o.x0 + o.w / 2), wy(o.y0 + o.d / 2), zTop + o.z));
  let b = union(items);
  for (const [x, y] of BOARD.holes) b = subtract(b, cyl(BOARD.holeD, 30, wx(x), wy(y), zBoard - 1));
  return b;
}

// Outer body: rounded corners and rounded (filleted) bottom edge b and top edge t, from z0 to z1
function body(z0, z1, b, t) {
  const r = C.corner, N = 5, L = [];
  const layer = (inset, z) => roundedRect(OW - 2 * inset, OD - 2 * inset, 0.01, 0, 0, z, Math.max(0.6, r - inset));
  for (let i = 0; i <= N; i++) {
    const a = (i / N) * Math.PI / 2;
    L.push(layer(b - b * Math.sin(a), z0 + b - b * Math.cos(a)));          // bottom fillet
    L.push(layer(t - t * Math.cos(a), z1 - 0.01 - t + t * Math.sin(a)));  // top fillet
  }
  return hull(L);
}

// Snap-fit clip positions: x = +-1 on the right/left walls (at world y), y = +-1 on the back/front walls (at world x)
const SNAPS = [['x', 1, -32], ['x', 1, 22], ['x', -1, -10], ['x', -1, 22], ['y', 1, 0], ['y', -1, 12]];  // left clip moved clear of the USB opening

function base() {
  let s = body(0, zLid, C.edgeBottom, 0.01);
  // shadow line: a recessed step at the top of the base, so the lid seems to float and small misalignments hide
  s = subtract(s, subtract(box(OW + 2, OD + 2, C.shadowH + 1, 0, 0, zLid - C.shadowH),
    roundedRect(OW - 2 * C.shadowD, OD - 2 * C.shadowD, C.shadowH + 1, 0, 0, zLid - C.shadowH, C.corner - C.shadowD)));
  s = subtract(s, box(IW, ID, zLid, 0, 0, C.floor));
  const parts = [s];
  for (const [x, y] of BOARD.holes) {  // posts the board rests on, with a pin through each mounting hole
    parts.push(cyl(C.postD, C.post, wx(x), wy(y), C.floor));
    parts.push(cyl(C.pinD, BOARD.t, wx(x), wy(y), zBoard));
  }
  let b = union(parts);
  for (const [axis, side, pos] of SNAPS) {  // grooves the lid's clips click into
    const zg = zLid - C.lipH + 0.15, hg = C.lipH - 0.4, L = C.snapL + 1;
    if (axis === 'x') b = subtract(b, box(C.grooveD * 2, L, hg, side * IW / 2, pos, zg));
    else b = subtract(b, box(L, C.grooveD * 2, hg, pos, side * ID / 2, zg));
  }
  const rv = part('RV1');  // thumb scoop around the wheel, then the slot for the wheel rim
  b = subtract(b, cylinder({ radius: C.scoopR, height: zLid + 2, center: [OW / 2 + C.scoopR - C.scoopD, wy(rv[3]), zLid / 2 - 1], segments: 96 }));
  b = subtract(b, box(C.wall + 4, WHEEL.slotL, WHEEL.slotH, OW / 2, wy(rv[3]), zTop + WHEEL.z - WHEEL.slotH / 2));
  const usb = part('J1');  // opening in the left wall, centred on the D1 mini's micro-USB port
  { const zc = zTop + 2.4;  // port centre: 1 mm D1 mini board + half the port height
    const pill = (w, h, len, x) => { const rr = h / 2, pin = (dy) => translate([x, wy(usb[3]) + dy, zc], rotateY(Math.PI / 2, cylinder({ radius: rr, height: len, segments: 32 })));
      return hull(pin(-(w / 2 - rr)), pin(w / 2 - rr)); };
    b = subtract(b, pill(C.usbW, C.usbH, C.wall + 4, -OW / 2));                                   // plug opening
    b = subtract(b, pill(C.usbRecessW, C.usbRecessH, 2 * C.usbRecessD, -OW / 2)); }              // recess for the plug body
  return b;
}

function lid() {
  const top = body(0, C.lid, C.seam, C.edgeTop);
  const lw = IW - 2 * C.fit, ld = ID - 2 * C.fit;
  let lip = subtract(box(lw, ld, C.lipH, 0, 0, -C.lipH), box(lw - 2 * C.lipT, ld - 2 * C.lipT, C.lipH + 2, 0, 0, -C.lipH - 1));
  { const rv = part('RV1');  // gap in the lip where the scroll wheel passes under it (it would jam the wheel)
    lip = subtract(lip, box(WHEEL.d + 4, WHEEL.d + 4, C.lipH + 1, wx(WHEEL.x), wy(rv[3]), -C.lipH - 1)); }  // stops at the lid underside
  { const u = part('U1');  // gap in the lip where the D1 mini reaches the left edge
    lip = subtract(lip, box(6, u[5] + 1, C.lipH + 1, -IW / 2 + 1, wy(u[3]), -C.lipH - 1)); }
  const parts = [top, lip];
  for (const [x, y] of BOARD.holes) parts.push(cyl(C.tubeD, C.above, wx(x), wy(y), -C.above));  // presses the board onto the posts
  for (const [axis, side, pos] of SNAPS) {  // clip: a ramped bump on the outside of the lip (ramps make it click in and pry out)
    const face = axis === 'x' ? lw / 2 : ld / 2, L = C.snapL;
    const plate = (off, z0, z1) => axis === 'x'
      ? box(0.01, L, z1 - z0, side * (face + off), pos, z0)
      : box(L, 0.01, z1 - z0, pos, side * (face + off), z0);
    parts.push(hull(plate(-0.3, -C.lipH + 0.2, -0.35), plate(C.snapH, -C.lipH + 0.75, -0.8)));
  }
  let l = union(parts);
  const o = SCREEN, wn = o.win;
  const wcx = wx(o.x0 + wn.x), wcy = wy(o.y0 + wn.y);
  const g = o.glass, gcx = wx(o.x0 + g.x + g.w / 2), gcy = wy(o.y0 + g.y + g.d / 2);
  // opening for the black screen panel: full size from the ledge up, 1 mm smaller below (the ledge holds the panel)
  l = subtract(l, roundedRect(C.panelW, C.panelD, C.lid + 2, 0, gcy, C.ledge, C.panelR));
  l = subtract(l, roundedRect(C.panelW - 2, C.panelD - 2, C.lid + 2, 0, gcy, -1, C.panelR - 1));
  // the panel: flush with the lid top, a bevelled window over the picture, a pocket underneath for the glass
  // 2 mm thick (the minimum most print services accept): the top sits on the ledge, a step below drops into the ledge opening
  let panel = union(roundedRect(C.panelW - 0.3, C.panelD - 0.3, C.lid - C.ledge, 0, gcy, C.ledge, C.panelR - 0.15),
    roundedRect(C.panelW - 2.4, C.panelD - 2.4, C.ledge + 0.01, 0, gcy, 0, C.panelR - 1.2));
  panel = subtract(panel, box(wn.w, wn.d, C.lid + 2, wcx, wcy, -1));
  panel = subtract(panel, hull(box(wn.w, wn.d, 0.01, wcx, wcy, C.lid - BEVEL), box(wn.w + 2 * BEVEL + 1, wn.d + 2 * BEVEL + 1, 0.01, wcx, wcy, C.lid + 0.5)));
  panel = subtract(panel, box(g.w + 0.6, g.d + 0.6, C.glassPocket + 1, gcx, gcy, -1));
  for (const id of ['SW1', 'SW2']) {  // big round button openings with a soft chamfer
    const p = part(id), x = wx(p[2]), y = wy(p[3]);
    const hole = capSize(id) + CAP.clearance;
    l = subtract(l, cyl(hole, C.lid + 2, x, y, -1));
    l = subtract(l, hull(cyl(hole, 0.01, x, y, C.lid - 0.6), cyl(hole + 2.2, 0.01, x, y, C.lid + 0.5)));
  }
  // logo engraved in the top, under the screen; 'logo' is the matching insert for two-colour printing
  // the name on the front, below the buttons; 'name' is a matching insert for two-colour printing
  let name = null;
  const nm = textShape(LOGO.text, LOGO.lidCap);
  if (nm) {
    const placed = translate([0, wy(LOGO.lidY), 0], nm);
    l = subtract(l, translate([0, 0, C.lid - LOGO.depth], extrudeLinear({ height: LOGO.depth + 1 }, placed)));
    name = translate([0, 0, C.lid - LOGO.depth], extrudeLinear({ height: LOGO.depth }, placed));
  }
  const t = LOGO.onLid ? logoMark(11.5, false) : null;
  let logo = null;
  if (t) {
    const placed = translate([wx(12), wy(8.6), 0], t);
    l = subtract(l, translate([0, 0, C.lid - LOGO.depth], extrudeLinear({ height: LOGO.depth + 1 }, placed)));
    logo = translate([0, 0, C.lid - LOGO.depth], extrudeLinear({ height: LOGO.depth }, placed));
  }
  const flip = (g) => translate([0, 0, C.lid], rotateX(Math.PI, g));  // print top face down (a rotation, so holes still line up)
  return { lid: flip(l), logo: logo && flip(logo), panel: flip(panel), name: name && flip(name) };
}

// rounded rectangle prism: w along x, d along y, from z to z+h
function roundedRect(w, d, h, x, y, z, r) {
  const c = (dx, dy) => cyl(2 * r, h, x + dx, y + dy, z);
  return hull(c(-w / 2 + r, -d / 2 + r), c(w / 2 - r, -d / 2 + r), c(-w / 2 + r, d / 2 - r), c(w / 2 - r, d / 2 - r));
}

// knurled thumbwheel (preview only), lying flat
function wheel(x, y, z) {
  let w = cylinder({ radius: WHEEL.d / 2, height: WHEEL.t, segments: 64 });
  for (let i = 0; i < WHEEL.ridges; i++) {
    const a = (i / WHEEL.ridges) * 2 * Math.PI;
    w = subtract(w, cylinder({ radius: 0.45, height: WHEEL.t + 1, center: [Math.cos(a) * WHEEL.d / 2, Math.sin(a) * WHEEL.d / 2, 0], segments: 8 }));
  }
  return translate([x, y, z], w);  // axis vertical: the rim is rolled up/down along the right side
}

// button cap, in place above a switch (board coordinates z) or flipped for printing
function buttonCap(x = 0, y = 0, z = 0, d = CAP.main) {
  const h = CAP.top - CAP.z0, fz = CAP.postH;
  const parts = [cyl(CAP.postD, fz, 0, 0, 0), cyl(d + 2, CAP.flangeT, 0, 0, fz),
    hull(cyl(d, h - fz - 1.1, 0, 0, fz), cyl(d - 1.2, h - fz - 0.35, 0, 0, fz), cyl(d - 3, h - fz, 0, 0, fz))];   // soft, rounded top
  return translate([x, y, z], union(parts));
}

function layoutSvg() {
  const s = 9, pad = 50, W = BOARD.w * s, D = BOARD.d * s;
  const X = (x) => pad + x * s, Y = (y) => pad + (BOARD.d - y) * s;
  let g = '';
  const o = SCREEN;
  for (const [id, label, x, y, w, d, , col] of PARTS) {
    g += `<rect x="${X(x - w / 2)}" y="${Y(y + d / 2)}" width="${w * s}" height="${d * s}" rx="3" fill="${col}" stroke="#fff" stroke-opacity=".35"/>`;
    const light = ['#c0c0c0', '#b8b8b8', '#40ff70'].includes(col);
    const small = w < 7 && id !== 'SW1' && id !== 'SW2' || id === 'J2';
    if (small) g += `<text x="${X(x)}" y="${Y(y - d / 2) + 14}" text-anchor="middle" font-size="10" fill="#ddd">${id === 'J2' ? 'J2' : id + ' ' + label}</text>`;
    else g += `<text x="${X(x)}" y="${Y(y) + 4}" text-anchor="middle" font-size="12" fill="${light ? '#000' : '#fff'}">${id} ${label}</text>`;
  }
  g += `<rect x="${X(o.x0)}" y="${Y(o.y0 + o.d)}" width="${o.w * s}" height="${o.d * s}" fill="#4da3ff" fill-opacity=".12" stroke="#4da3ff" stroke-width="2" stroke-dasharray="8 5"/>`;
  g += `<text x="${X(o.x0 + 4)}" y="${Y(o.y0 + o.d - 7)}" font-size="13" fill="#7fdbff">1.8" TFT module above (on J2)</text>`;
  for (const [x, y] of BOARD.holes) g += `<circle cx="${X(x)}" cy="${Y(y)}" r="${BOARD.holeD / 2 * s}" fill="#000" stroke="#e9c46a" stroke-width="2"/>`;
  g += `<rect x="${X(53)}" y="${Y(22)}" width="${7 * s}" height="${16 * s}" fill="none" stroke="#ff6b6b" stroke-dasharray="4 4"/><text x="${X(56.5)}" y="${Y(22) - 6}" text-anchor="middle" font-size="11" fill="#ff6b6b">antenna: no copper</text>`;
  return `<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${W + 2 * pad + 250} ${D + 2 * pad + 40}" font-family="Inter, Arial, sans-serif">
<title>xonraOS board layout</title>
<rect width="100%" height="100%" fill="#000"/>
<text x="${pad}" y="32" font-size="20" font-weight="700" fill="#fff">xonraOS board — part placement (top view, ${BOARD.w} × ${BOARD.d} mm)</text>
<rect x="${pad}" y="${pad}" width="${W}" height="${D}" rx="8" fill="#1f6f43" stroke="#fff" stroke-width="2"/>
${g}
<g font-size="13" fill="#bbb">
<text x="${pad + W + 24}" y="${pad + 20}" fill="#fff" font-weight="700">Case</text>
<text x="${pad + W + 24}" y="${pad + 44}">Outside ${OW.toFixed(1)} × ${OD.toFixed(1)} × ${H.toFixed(1)} mm</text>
<text x="${pad + W + 24}" y="${pad + 66}">4 × M2 screws, 12–14 mm</text>
<text x="${pad + W + 24}" y="${pad + 88}">Buttons: 12×12×5 mm + 14 mm caps</text>
<text x="${pad + W + 24}" y="${pad + 110}">Side wheel: flat 16 mm thumbwheel pot</text>
<text x="${pad + W + 24}" y="${pad + 132}">micro-USB on the left side</text>
<text x="${pad + W + 24}" y="${pad + 154}">Yellow rings: M2 mounting holes</text>
</g>
<text x="${pad}" y="${pad + D + 30}" font-size="12" fill="#888">Draft placement for the KiCad layout. Netlist and parts: hardware/README.md, hardware/bom.csv.</text>
</svg>`;
}

const write = (name, geom) => {
  fs.writeFileSync(path.join(__dirname, name), Buffer.concat(stl.serialize({ binary: true }, geom).map((a) => Buffer.from(a))));
  const [[a, b, c], [d, e, f]] = measurements.measureBoundingBox(geom);
  console.log(`${name}: ${(d - a).toFixed(1)} x ${(e - b).toFixed(1)} x ${(f - c).toFixed(1)} mm, ${(measurements.measureVolume(geom) / 1000).toFixed(1)} cm3`);
};
write('board-preview.stl', boardPreview());
{ const o = SCREEN, g = o.glass; write('screen-preview.stl', box(g.w, g.d, g.h, wx(o.x0 + g.x + g.w / 2), wy(o.y0 + g.y + g.d / 2), zTop + o.z + o.pcb)); }
{ const rv = part('RV1'); write('wheel-preview.stl', wheel(wx(WHEEL.x), wy(rv[3]), zTop + WHEEL.z)); }  // wheel() centres the disc on z
for (const [id, name] of [['SW1', 'main'], ['SW2', 'second']]) {
  const p = part(id);
  write(`cap-${name}-preview.stl`, buttonCap(wx(p[2]), wy(p[3]), zTop + CAP.z0, capSize(id)));
  write(`button-cap-${name}.stl`, translate([0, 0, CAP.top - CAP.z0], rotateX(Math.PI, buttonCap(0, 0, 0, capSize(id)))));  // top face down
}
write('base.stl', base());
const L = lid();
write('lid.stl', L.lid);
write('screen-panel.stl', L.panel);  // print in black, glue into the lid
if (L.name) write('lid-name.stl', L.name);  // optional: print together with the lid in a second colour
if (L.logo) write('lid-logo.stl', L.logo);
fs.writeFileSync(path.join(__dirname, 'layout.svg'), layoutSvg());
console.log(`case outside: ${OW.toFixed(1)} x ${OD.toFixed(1)} x ${H.toFixed(1)} mm; lid sits at z=${zLid.toFixed(1)}`);

// Fit check: the scroll wheel must not touch the closed lid or the base (run with CHECK=1)
if (process.env.CHECK) {
  const rv = part('RV1');
  const o = SCREEN, g = o.glass;
  const inside = {
    wheel: wheel(wx(WHEEL.x), wy(rv[3]), zTop + WHEEL.z),
    board: boardPreview(),
    screen: box(g.w, g.d, g.h, wx(o.x0 + g.x + g.w / 2), wy(o.y0 + g.y + g.d / 2), zTop + o.z + o.pcb),
    caps: union(['SW1', 'SW2'].map((id) => { const p = part(id); return buttonCap(wx(p[2]), wy(p[3]), zTop + CAP.z0, capSize(id)); })),
  };
  const closedLid = translate([0, 0, H], rotateX(Math.PI, L.lid));  // undo the print flip
  const shell = { base: base(), lid: closedLid };
  for (const [pn, pg] of Object.entries(inside)) for (const [sn, sg] of Object.entries(shell)) {
    const v = measurements.measureVolume(booleans.intersect(pg, sg));
    console.log(`fit check: ${pn} vs ${sn}: ${v < 0.05 ? 'clear' : v.toFixed(2) + ' mm3 overlap'}`);
  }
  const lidVsBase = measurements.measureVolume(booleans.intersect(closedLid, shell.base));
  console.log(`fit check: lid vs base: ${lidVsBase < 0.05 ? 'clear' : lidVsBase.toFixed(2) + ' mm3 overlap'}`);
}
