// xonraOS case for the 0 SAR build: your NodeMCU on its breadboard, the 0.96" OLED, two 6x6 mm buttons,
// a buzzer and a 16 mm knob potentiometer, all connected with male-to-female jumper wires.
// Same premium look as hardware/case (rounded edges, shadow line, snap-fit lid with no screws, big round
// buttons, a side wheel, XONRA on the front). It is thick because jumper wires need room; a soldered build can be much thinner.
//
//   npm install @jscad/modeling@2.12.5 @jscad/stl-serializer@2.1.20 opentype.js@1.3.4
//   node xonra-case.js            (CHECK=1 node xonra-case.js also runs a fit check)
// Measure your parts and change P below first. World coordinates: mm, x right, y up (screen end), z up, base bottom at z = 0.
// License: CERN-OHL-S-2.0 (see ../LICENSE).

const fs = require('fs');
const path = require('path');
const { primitives, booleans, transforms, measurements } = require('@jscad/modeling');
const stl = require('@jscad/stl-serializer');
const { cuboid, cylinder, polygon } = primitives;
const { union, subtract, intersect } = booleans;
const { translate, rotateX, rotateY, mirrorX } = transforms;
const { extrudeLinear } = require('@jscad/modeling').extrusions;
const { hull } = require('@jscad/modeling').hulls;

// ---------- your parts (measure them) ----------
const P = {
  bb: { w: 55, d: 82.5, h: 8.5 },                    // 400-point "half" breadboard, lying flat, long side along y
  node: { w: 31.5, d: 58, y0: -40.6, lift: 2.5, pcb: 1.6, h: 4.0 },  // NodeMCU v3 on the breadboard, USB end at the bottom (y0)
  oled: { w: 27.6, d: 28.0, t: 3.0, y: 22, winW: 23, winD: 12.5, winOffY: 1.0 },  // 0.96" module; window centre = module centre + winOffY (towards the pins)
  btn: { body: 6.0, bodyH: 3.5, total: 5.0, stem: 3.5 },  // 6x6 tactile switch: body height and total height including the stem
  caps: { main: { x: 11, y: -10, d: 16 }, second: { x: -11, y: -10, d: 12 } },
  pot: { y: 2, d: 16.5, depth: 9.5, bushing: 7.4, shaft: 6, flat: 4.5 },   // WH148-style 16 mm pot, 7 mm bushing, 6 mm D-shaft, through the right wall
  buzzer: { y: 26, d: 12 },                          // 12 mm buzzer glued inside the left wall, behind a grille
};
const LOGO = { text: 'XONRA', font: 'C:/Windows/Fonts/ENGR.TTF', cap: 3.6, y: -28, depth: 0.5, front: false, bottom: false };  // front / bottom: engrave the name there

// ---------- case ----------
const C = {
  gap: 0.4, wall: 2.0, floor: 1.6, inner: 34, corner: 6,
  edgeBottom: 3.0, edgeTop: 2.0, seam: 0.4, shadowH: 0.8, shadowD: 0.5,   // big soft bottom edge makes the box look thinner
  accentZ: 7, accentH: 1.0, accentD: 0.5,                                  // styling line around the base
  panelW: 38, panelD: 27, panelR: 3, ledge: 0.6,                           // glossy black screen panel set into the lid
  ring: 0.4,                                                               // depth of the trim ring around each button
  usbRecessW: 17, usbRecessH: 12, usbRecessD: 0.8,                         // rounded pocket around the USB opening
  feetD: 8.2, feetDepth: 0.8, feetInset: 9,                                // pockets for 8 mm stick-on rubber feet
  bottomCap: 6,                                                            // XONRA under the base
  lid: 2.0, lipH: 2.5, lipT: 1.2, fit: 0.25, snapH: 0.6, snapL: 8, grooveD: 0.75,
  usbW: 12, usbH: 8,                    // opening for the micro-USB plug body
  scoopR: 16, scoopD: 1.0,              // thumb scoop around the wheel
  bevel: 1,
  capClear: 0.6, capRise: 1.2, flangeT: 0.8, boss: 3.0,
};

const segs = 48;
const IW = P.bb.w + 2 * C.gap, ID = P.bb.d + 2 * C.gap;
const OW = IW + 2 * C.wall, OD = ID + 2 * C.wall;
const zLid = C.floor + C.inner, H = zLid + C.lid;
const zBB = C.floor + P.bb.h;                                  // breadboard top
const zNode = zBB + P.node.lift;                               // NodeMCU board bottom
const usbZ = zNode + P.node.pcb + 1.5;                         // centre of its micro-USB port
const potZ = zLid - P.pot.d / 2 - 3.5;                         // pot axis height (its body stays under the lid lip)
const box = (w, d, h, x, y, z) => cuboid({ size: [w, d, h], center: [x, y, z + h / 2] });
const cyl = (dia, h, x, y, z) => cylinder({ radius: dia / 2, height: h, center: [x, y, z + h / 2], segments: segs });
function roundedRect(w, d, h, x, y, z, r) {
  const c = (dx, dy) => cyl(2 * r, h, x + dx, y + dy, z);
  return hull(c(-w / 2 + r, -d / 2 + r), c(w / 2 - r, -d / 2 + r), c(-w / 2 + r, d / 2 - r), c(w / 2 - r, d / 2 - r));
}
function body(z0, z1, b, t) {  // rounded corners, filleted bottom edge b and top edge t
  const L = [], layer = (i, z) => roundedRect(OW - 2 * i, OD - 2 * i, 0.01, 0, 0, z, Math.max(0.6, C.corner - i));
  for (let k = 0; k <= 5; k++) {
    const a = (k / 5) * Math.PI / 2;
    L.push(layer(b - b * Math.sin(a), z0 + b - b * Math.cos(a)));
    L.push(layer(t - t * Math.cos(a), z1 - 0.01 - t + t * Math.sin(a)));
  }
  return hull(L);
}
function textShape(text, capH) {
  if (!LOGO.font || !fs.existsSync(LOGO.font)) { console.warn('logo font not found, name skipped'); return null; }
  const font = require('opentype.js').loadSync(LOGO.font);
  const scale = capH / ((font.tables.os2 && font.tables.os2.sCapHeight) || font.unitsPerEm * 0.7);
  const cmds = font.getPath(text, 0, 0, font.unitsPerEm * scale).commands;
  const contours = []; let cur = [], px = 0, py = 0;
  const add = (x, y) => { cur.push([x, -y]); px = x; py = y; };
  for (const c of cmds) {
    if (c.type === 'M') { if (cur.length > 2) contours.push(cur); cur = []; add(c.x, c.y); }
    else if (c.type === 'L') add(c.x, c.y);
    else if (c.type === 'Q') { const [x0, y0] = [px, py]; for (let t = 1; t <= 6; t++) { const u = t / 6; add((1 - u) ** 2 * x0 + 2 * (1 - u) * u * c.x1 + u * u * c.x, (1 - u) ** 2 * y0 + 2 * (1 - u) * u * c.y1 + u * u * c.y); } }
    else if (c.type === 'C') { const [x0, y0] = [px, py]; for (let t = 1; t <= 8; t++) { const u = t / 8; add((1 - u) ** 3 * x0 + 3 * (1 - u) ** 2 * u * c.x1 + 3 * (1 - u) * u * u * c.x2 + u ** 3 * c.x, (1 - u) ** 3 * y0 + 3 * (1 - u) ** 2 * u * c.y1 + 3 * (1 - u) * u * u * c.y2 + u ** 3 * c.y); } }
    else if (c.type === 'Z') { if (cur.length > 2) contours.push(cur); cur = []; }
  }
  const area = (p) => p.reduce((s, [x, y], i) => { const [x2, y2] = p[(i + 1) % p.length]; return s + x * y2 - x2 * y; }, 0) / 2;
  const inside = (pt, p) => { let r = false; for (let i = 0, j = p.length - 1; i < p.length; j = i++) { const [xi, yi] = p[i], [xj, yj] = p[j]; if ((yi > pt[1]) !== (yj > pt[1]) && pt[0] < (xj - xi) * (pt[1] - yi) / (yj - yi) + xi) r = !r; } return r; };
  const clean = contours.map((p) => p.filter((q, i) => i === 0 || Math.hypot(q[0] - p[i - 1][0], q[1] - p[i - 1][1]) > 1e-4)).filter((p) => Math.abs(area(p)) > 1e-3);
  const solids = [], holes = [];
  for (const p of clean) (clean.filter((o) => o !== p && inside(p[0], o)).length % 2 ? holes : solids).push(polygon({ points: area(p) > 0 ? p : p.slice().reverse() }));
  let g = union(solids);
  if (holes.length) g = subtract(g, union(holes));
  const [[a, b], [c, d]] = measurements.measureBoundingBox(g);
  return translate([-(a + c) / 2, -(b + d) / 2, 0], g);
}

// Snap clips: x = +-1 on the right/left walls (at y), y = +-1 on the top/bottom walls (at x). Kept clear of the USB, wheel and grille.
const SNAPS = [['x', 1, -26], ['x', 1, 28], ['x', -1, -26], ['x', -1, 8], ['y', 1, 0], ['y', -1, 16]];

function base() {
  let s = body(0, zLid, C.edgeBottom, 0.01);
  s = subtract(s, subtract(box(OW + 2, OD + 2, C.shadowH + 1, 0, 0, zLid - C.shadowH),
    roundedRect(OW - 2 * C.shadowD, OD - 2 * C.shadowD, C.shadowH + 1, 0, 0, zLid - C.shadowH, C.corner - C.shadowD)));
  let b = subtract(s, box(IW, ID, zLid, 0, 0, C.floor));
  for (const [axis, side, pos] of SNAPS) {
    const zg = zLid - C.lipH + 0.15, hg = C.lipH - 0.4, L = C.snapL + 1;
    b = subtract(b, axis === 'x' ? box(C.grooveD * 2, L, hg, side * IW / 2, pos, zg) : box(L, C.grooveD * 2, hg, pos, side * ID / 2, zg));
  }
  // USB: pill-shaped opening in the bottom wall, in line with the NodeMCU's port, inside a shallow rounded pocket
  { const pill = (w, h, len) => { const rr = h / 2, pin = (dx) => translate([dx, -OD / 2, usbZ], rotateX(Math.PI / 2, cylinder({ radius: rr, height: len, segments: 32 })));
      return hull(pin(-(w / 2 - rr)), pin(w / 2 - rr)); };
    b = subtract(b, pill(C.usbW, C.usbH, 2 * C.wall + 4));
    b = subtract(b, pill(C.usbRecessW, C.usbRecessH, 2 * C.usbRecessD)); }
  // styling line all the way round
  b = subtract(b, subtract(box(OW + 2, OD + 2, C.accentH, 0, 0, C.accentZ),
    roundedRect(OW - 2 * C.accentD, OD - 2 * C.accentD, C.accentH, 0, 0, C.accentZ, C.corner - C.accentD)));
  // underside: pockets for rubber feet and XONRA (mirrored so it reads from below)
  for (const sx of [-1, 1]) for (const sy of [-1, 1])
    b = subtract(b, cyl(C.feetD, C.feetDepth + 0.01, sx * (OW / 2 - C.feetInset), sy * (OD / 2 - C.feetInset), -0.01));
  { const t = LOGO.bottom ? textShape(LOGO.text, C.bottomCap) : null;
    if (t) b = subtract(b, translate([0, 0, -1], extrudeLinear({ height: LOGO.depth + 1 }, mirrorX(t)))); }
  // side wheel: thumb scoop and the hole for the pot's threaded bushing
  b = subtract(b, cylinder({ radius: C.scoopR, height: zLid + 2, center: [OW / 2 + C.scoopR - C.scoopD, P.pot.y, zLid / 2 - 1], segments: 96 }));
  b = subtract(b, translate([OW / 2, P.pot.y, potZ], rotateY(Math.PI / 2, cylinder({ radius: P.pot.bushing / 2, height: 2 * C.wall + 4, segments: 32 }))));
  // buzzer grille: 5 rounded vertical slots in the left wall
  for (let i = -2; i <= 2; i++) {
    const slot = (dz) => translate([-OW / 2, P.buzzer.y + i * 2.8, potZ + dz], rotateY(Math.PI / 2, cylinder({ radius: 0.7, height: 2 * C.wall + 4, segments: 16 })));
    b = subtract(b, hull(slot(-3.3), slot(3.3)));
  }
  return b;  // the breadboard fits the inside with 0.4 mm to spare; its sticky back holds it to the floor
}

// lid, built with z = 0 at its underside (then flipped to print top face down)
function lid() {
  const lw = IW - 2 * C.fit, ld = ID - 2 * C.fit;
  let lip = subtract(box(lw, ld, C.lipH, 0, 0, -C.lipH), box(lw - 2 * C.lipT, ld - 2 * C.lipT, C.lipH + 2, 0, 0, -C.lipH - 1));
  lip = subtract(lip, box(6, P.pot.d + 6, C.lipH + 1, lw / 2, P.pot.y, -C.lipH - 1));   // room for the pot body at the right wall
  const parts = [body(0, C.lid, C.seam, C.edgeTop), lip];
  for (const [axis, side, pos] of SNAPS) {
    const face = axis === 'x' ? lw / 2 : ld / 2, L = C.snapL;
    const plate = (off, z0, z1) => axis === 'x' ? box(0.01, L, z1 - z0, side * (face + off), pos, z0) : box(L, 0.01, z1 - z0, pos, side * (face + off), z0);
    parts.push(hull(plate(-0.3, -C.lipH + 0.2, -0.35), plate(C.snapH, -C.lipH + 0.75, -0.8)));
  }
  // OLED holder: a frame under the lid, open on the pin side; the glass presses against the lid underside
  const o = P.oled, ocy = o.y - o.winOffY, fw = o.w + 0.4, fd = o.d + 0.4;
  let frame = subtract(box(fw + 2.4, fd + 2.4, o.t, 0, ocy, -o.t), box(fw, fd, o.t + 2, 0, ocy, -o.t - 1));
  frame = subtract(frame, box(fw - 6, 4, o.t + 2, 0, ocy + fd / 2, -o.t - 1));
  frame = subtract(frame, box(C.panelW, C.panelD, o.t + 2, 0, o.y, -o.t - 1));   // no walls hanging under the panel opening
  parts.push(frame);
  // button bosses: a ring under each cap hole; the button holder (separate print) is glued under it
  for (const k of ['main', 'second']) { const c = P.caps[k]; parts.push(subtract(cyl(c.d + 6, C.boss, c.x, c.y, -C.boss), cyl(c.d + 2.8, C.boss + 2, c.x, c.y, -C.boss - 1))); }
  let l = union(parts);
  // opening for the black screen panel: full size above the ledge, 1 mm smaller below (the ledge holds the panel)
  l = subtract(l, roundedRect(C.panelW, C.panelD, C.lid + 2, 0, o.y, C.ledge, C.panelR));
  l = subtract(l, roundedRect(C.panelW - 2, C.panelD - 2, C.lid + 2, 0, o.y, -1, C.panelR - 1));
  // the panel (separate print, glossy black): flush with the lid top, bevelled window over the picture
  let panel = union(roundedRect(C.panelW - 0.3, C.panelD - 0.3, C.lid - C.ledge, 0, o.y, C.ledge, C.panelR - 0.15),
    roundedRect(C.panelW - 2.4, C.panelD - 2.4, C.ledge + 0.01, 0, o.y, 0, C.panelR - 1.2));
  panel = subtract(panel, box(o.winW, o.winD, C.lid + 2, 0, o.y, -1));
  panel = subtract(panel, hull(box(o.winW, o.winD, 0.01, 0, o.y, C.lid - C.bevel), box(o.winW + 2 * C.bevel + 0.6, o.winD + 2 * C.bevel + 0.6, 0.01, 0, o.y, C.lid + 0.5)));
  for (const k of ['main', 'second']) {  // round button openings with a soft chamfer and a thin trim ring
    const c = P.caps[k], hole = c.d + C.capClear;
    l = subtract(l, cyl(hole, C.lid + 2, c.x, c.y, -1));
    l = subtract(l, hull(cyl(hole, 0.01, c.x, c.y, C.lid - 0.6), cyl(hole + 2.2, 0.01, c.x, c.y, C.lid + 0.5)));
    l = subtract(l, subtract(cyl(hole + 5, C.ring + 1, c.x, c.y, C.lid - C.ring), cyl(hole + 3.6, C.ring + 2, c.x, c.y, C.lid - C.ring - 1)));
  }
  let name = null;
  const nm = LOGO.front ? textShape(LOGO.text, LOGO.cap) : null;
  if (nm) {
    const placed = translate([0, LOGO.y, 0], nm);
    l = subtract(l, translate([0, 0, C.lid - LOGO.depth], extrudeLinear({ height: LOGO.depth + 1 }, placed)));
    name = translate([0, 0, C.lid - LOGO.depth], extrudeLinear({ height: LOGO.depth }, placed));
  }
  return { lid: l, name, panel };
}
const flip = (g) => translate([0, 0, C.lid], rotateX(Math.PI, g));   // print the lid top face down
const toWorld = (g) => translate([0, 0, zLid], g);                  // closed position

// Button cap (lid coordinates): flange under the lid, body through the hole, post down onto the switch stem
const stemTop = -C.boss + 0.3;   // the holder plate sits at -boss, the stem pokes 0.3 mm through it
function buttonCap(d) {
  const top = C.lid + C.capRise;
  return union(cyl(d + 2, C.flangeT, 0, 0, -C.flangeT), cyl(3, C.flangeT - stemTop, 0, 0, stemTop),
    hull(cyl(d, top - 0.9, 0, 0, 0), cyl(d - 1.2, top - 0.3, 0, 0, 0), cyl(d - 3, top, 0, 0, 0)));
}
// Button holder (lid coordinates, glued under the boss): a plate with a hole for the stem, and a socket the switch is glued into
function buttonHolder(d) {
  const plateT = 1.2, sock = P.btn.body + 0.5, sockH = P.btn.bodyH;
  let h = union(cyl(d + 6, plateT, 0, 0, -C.boss - plateT), box(sock + 2.4, sock + 2.4, sockH, 0, 0, -C.boss - plateT - sockH));
  h = subtract(h, cyl(P.btn.stem + 0.7, plateT + 2, 0, 0, -C.boss - plateT - 1));
  h = subtract(h, box(sock, sock, sockH + 0.01, 0, 0, -C.boss - plateT - sockH - 0.01));
  return h;
}
// Side wheel: a knurled knob pressed on the pot's D-shaft, outside the right wall
function wheelKnob() {
  const d = 20, t = 8, bore = 7;
  let k = cylinder({ radius: d / 2, height: t, center: [0, 0, t / 2], segments: 96 });
  for (let i = 0; i < 36; i++) { const a = (i / 36) * 2 * Math.PI; k = subtract(k, cylinder({ radius: 0.55, height: t + 2, center: [Math.cos(a) * d / 2, Math.sin(a) * d / 2, t / 2], segments: 10 })); }
  k = intersect(k, hull(cyl(d - 1.6, 0.01, 0, 0, 0), cyl(d + 0.2, t - 1.6, 0, 0, 0.8), cyl(d - 1.6, 0.01, 0, 0, t - 0.01)));  // chamfered edges
  k = subtract(k, subtract(cyl(d - 5, 0.5, 0, 0, t - 0.5), cyl(d - 6.2, 0.6, 0, 0, t - 0.55)));                          // fine ring on the outer face
  const shaft = intersect(cylinder({ radius: P.pot.shaft / 2 + 0.1, height: bore + 1, center: [0, 0, (bore + 1) / 2 - 1], segments: 32 }),
    box(P.pot.shaft + 2, P.pot.shaft + 2, bore + 1, 0, P.pot.flat + 0.1 - (P.pot.shaft + 2) / 2 - P.pot.shaft / 2, -1));
  return subtract(k, shaft);   // blind D-hole, flat side down; print with the hole facing up
}

// Preview of what goes inside (for the fit check and for viewing): breadboard, NodeMCU, OLED, pot, buzzer
function insidePreview() {
  const n = P.node, o = P.oled;
  return union(
    box(P.bb.w, P.bb.d, P.bb.h, 0, 0, C.floor),
    box(n.w, n.d, n.pcb + n.h, 0, n.y0 + n.d / 2, zNode),
    box(o.w, o.d, 1.6 + 1.4, 0, o.y - o.winOffY, zLid - 3.0),
    translate([OW / 2 - C.wall - P.pot.depth / 2, P.pot.y, potZ], rotateY(Math.PI / 2, cylinder({ radius: P.pot.d / 2, height: P.pot.depth, segments: 32 }))),
    translate([-OW / 2 + C.wall + 5, P.buzzer.y, potZ], rotateY(Math.PI / 2, cylinder({ radius: P.buzzer.d / 2, height: 9.5, segments: 32 }))));
}

function drawingSvg() {
  const s = 6, pad = 40, X = (x) => pad + (x + OW / 2) * s, Y = (y) => pad + (OD / 2 - y) * s;
  const o = P.oled;
  let g = `<rect x="${X(-OW / 2)}" y="${Y(OD / 2)}" width="${OW * s}" height="${OD * s}" rx="${C.corner * s}" fill="#2b2b2e" stroke="#888"/>`;
  g += `<rect x="${X(-C.panelW / 2)}" y="${Y(o.y + C.panelD / 2)}" width="${C.panelW * s}" height="${C.panelD * s}" rx="18" fill="#0d0d0f"/>`;
  g += `<rect x="${X(-o.winW / 2)}" y="${Y(o.y + o.winD / 2)}" width="${o.winW * s}" height="${o.winD * s}" fill="#0b2a3a" stroke="#4da3ff"/>`;
  for (const k of ['main', 'second']) { const c = P.caps[k]; g += `<circle cx="${X(c.x)}" cy="${Y(c.y)}" r="${c.d / 2 * s}" fill="${k === 'main' ? '#bdbdc2' : '#111'}" stroke="#666"/>`; }
  g += `<rect x="${X(OW / 2)}" y="${Y(P.pot.y + 10)}" width="${8 * s}" height="${20 * s}" rx="6" fill="#111" stroke="#666"/>`;
  if (LOGO.front) g += `<text x="${X(0)}" y="${Y(LOGO.y) + 7}" text-anchor="middle" font-size="22" letter-spacing="6" fill="#9a9aa0">XONRA</text>`;
  return `<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${OW * s + 2 * pad + 300} ${OD * s + 2 * pad}" font-family="Inter, Arial, sans-serif">
<title>xonraOS NodeMCU case</title>
<rect width="100%" height="100%" fill="#f4f4f5"/>
${g}
<g font-size="14" fill="#333">
<text x="${X(OW / 2) + 70}" y="${pad + 20}" font-weight="700">xonraOS case (NodeMCU + OLED)</text>
<text x="${X(OW / 2) + 70}" y="${pad + 46}">Outside ${OW.toFixed(1)} × ${OD.toFixed(1)} × ${H.toFixed(1)} mm</text>
<text x="${X(OW / 2) + 70}" y="${pad + 70}">Screen window ${o.winW} × ${o.winD} mm</text>
<text x="${X(OW / 2) + 70}" y="${pad + 94}">Buttons ${P.caps.main.d} and ${P.caps.second.d} mm caps</text>
<text x="${X(OW / 2) + 70}" y="${pad + 118}">Side wheel: knob on a 16 mm pot</text>
<text x="${X(OW / 2) + 70}" y="${pad + 142}">USB at the bottom, buzzer grille on the left</text>
</g>
</svg>`;
}

const write = (name, geom) => {
  fs.writeFileSync(path.join(__dirname, name), Buffer.concat(stl.serialize({ binary: true }, geom).map((a) => Buffer.from(a))));
  const [[a, b, c], [d, e, f]] = measurements.measureBoundingBox(geom);
  console.log(`${name}: ${(d - a).toFixed(1)} x ${(e - b).toFixed(1)} x ${(f - c).toFixed(1)} mm, ${(measurements.measureVolume(geom) / 1000).toFixed(1)} cm3`);
};
const B = base(), L = lid();
write('base.stl', B);
write('lid.stl', flip(L.lid));
if (L.name) write('lid-name.stl', flip(L.name));
for (const k of ['main', 'second']) {
  const d = P.caps[k].d;
  write(`button-cap-${k}.stl`, translate([0, 0, C.lid + C.capRise], rotateX(Math.PI, buttonCap(d))));  // top face down on the bed
  write(`button-holder-${k}.stl`, translate([0, 0, -C.boss], rotateX(Math.PI, buttonHolder(d))));       // plate on the bed, socket up
}
write('screen-panel.stl', flip(L.panel));   // glossy black, drops into the lid from above
write('wheel-knob.stl', wheelKnob());
write('inside-preview.stl', insidePreview());
fs.writeFileSync(path.join(__dirname, 'case-drawing.svg'), drawingSvg());
console.log(`case outside: ${OW.toFixed(1)} x ${OD.toFixed(1)} x ${H.toFixed(1)} mm`);

if (process.env.CHECK) {
  const lidW = translate([0, 0, 0.02], toWorld(L.lid));   // lifted 0.02 mm so parts that only touch the lid do not count
  const capsW = union(['main', 'second'].map((k) => { const c = P.caps[k]; return toWorld(translate([c.x, c.y, 0], buttonCap(c.d))); }));
  const holdersW = union(['main', 'second'].map((k) => { const c = P.caps[k]; return toWorld(translate([c.x, c.y, 0], buttonHolder(c.d))); }));
  const items = { inside: insidePreview(), caps: capsW, holders: holdersW };
  for (const [n, g] of Object.entries(items)) for (const [sn, sg] of Object.entries({ base: B, lid: lidW })) {
    if (n === 'holders' && sn === 'lid') continue;  // glued to the lid on purpose
    const v = measurements.measureVolume(intersect(g, sg));
    console.log(`fit check: ${n} vs ${sn}: ${v < 0.05 ? 'clear' : v.toFixed(2) + ' mm3 overlap'}`);
  }
  for (const [a, b, n] of [[capsW, holdersW, 'caps vs holders'], [holdersW, insidePreview(), 'holders vs inside'], [lidW, B, 'lid vs base']]) {
    const v = measurements.measureVolume(intersect(a, b));
    console.log(`fit check: ${n}: ${v < 0.05 ? 'clear' : v.toFixed(2) + ' mm3 overlap'}`);
  }
}
