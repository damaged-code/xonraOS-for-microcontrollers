// xonraOS desk case: a parametric 3D model for the breadboard-free build
// (NodeMCU ESP8266 + 0.96" OLED + 2 buttons + buzzer + 10k panel knob, wired with jumper wires).
//
// Make the STL files:
//   npm install @jscad/modeling@2.12.5 @jscad/stl-serializer@2.1.20
//   node xonra-case.js            -> base.stl, lid.stl, case-drawing.svg in this folder
// Change the numbers in P below to fit your parts (measure them with a ruler or calipers first).
// License: CERN-OHL-S-2.0 (see ../LICENSE).

const fs = require('fs');
const path = require('path');
const { primitives, booleans, transforms } = require('@jscad/modeling');
const { measureBoundingBox, measureVolume } = require('@jscad/modeling').measurements;
const stl = require('@jscad/stl-serializer');
const { cuboid, cylinder, roundedCuboid } = primitives;
const { union, subtract } = booleans;
const { translate } = transforms;

// ---------- parameters (millimetres) ----------
const P = {
  wall: 2, floor: 2, lid: 2.5, corner: 3,
  innerW: 76, innerD: 52, innerH: 34,   // space inside the box
  fit: 0.25,                            // gap between lid lip and walls (raise if the lid is too tight)
  lipH: 3, lipT: 1.5,
  post: 6, pilot: 2.5, screwHole: 3.4,  // M3 self-tapping screws, 3 corners

  // NodeMCU (CP2102 "Amica" v2 is about 49 x 26 mm). It lies upside down, pins pointing up.
  nodeL: 49.5, nodeW: 26.7, nodeLift: 5.5, nodeX: -36.4, // nodeX = left end of the board
  usbW: 12, usbH: 9,

  // OLED module (about 27.3 x 27.8 mm) held under the lid; window over the screen
  oledW: 27.8, oledH: 28.2, oledX: -17, oledY: 2, winW: 25, winH: 14, winOffY: -1.5, oledPocket: 3.5,

  // Buttons: 6x6 mm tactile switches with tall stems (6x6x12 or 6x6x13)
  btnHole: 4, btnBody: 6.4, btnPocket: 3, btnMain: [31, -13], btnSecond: [19, -13],

  // Knob: 16 mm panel potentiometer (B10K) with a 7 mm threaded bushing
  knobHole: 7.4, knob: [26, 12],

  // Buzzer sound slots in the left wall
  slots: 5, slotW: 1.6, slotH: 12, slotY: 15, slotZ: 18,
};

const segs = 48;
const OW = P.innerW + 2 * P.wall, OD = P.innerD + 2 * P.wall, BH = P.floor + P.innerH;
const box = (w, d, h, x, y, z) => cuboid({ size: [w, d, h], center: [x, y, z + h / 2] }); // z = bottom
const hole = (d, h, x, y, z) => cylinder({ radius: d / 2, height: h, center: [x, y, z + h / 2], segments: segs });
const postCenters = [[-(P.innerW / 2 - P.post / 2), P.innerD / 2 - P.post / 2],
                     [P.innerW / 2 - P.post / 2, P.innerD / 2 - P.post / 2],
                     [P.innerW / 2 - P.post / 2, -(P.innerD / 2 - P.post / 2)]];

// ---------- base ----------
function base() {
  let shell = roundedCuboid({ size: [OW, OD, BH + P.corner], center: [0, 0, (BH + P.corner) / 2], roundRadius: P.corner, segments: 24 });
  shell = subtract(shell, box(OW + 2, OD + 2, P.corner + 1, 0, 0, BH));             // flat top
  shell = subtract(shell, box(P.innerW, P.innerD, P.innerH + 1, 0, 0, P.floor));     // hollow

  const parts = [shell];
  for (const [x, y] of postCenters) parts.push(subtract(box(P.post, P.post, P.innerH, x, y, P.floor), hole(P.pilot, 14, x, y, BH - 13)));

  // NodeMCU cradle: corner blocks it rests on, and a low fence on the back and right
  const x0 = P.nodeX, x1 = P.nodeX + P.nodeL, y0 = -P.innerD / 2 + 0.4, y1 = y0 + P.nodeW;
  for (const [x, y] of [[x0 + 2.5, y0 + 2.5], [x1 - 2.5, y0 + 2.5], [x0 + 2.5, y1 - 2.5], [x1 - 2.5, y1 - 2.5]])
    parts.push(box(5, 5, P.nodeLift, x, y, P.floor));
  parts.push(box(1.5, P.nodeW + 3, P.nodeLift + 3, x1 + 0.75 + 0.3, (y0 + y1) / 2 + 1.5, P.floor)); // right end
  parts.push(box(P.nodeL - 16, 1.5, P.nodeLift + 3, (x0 + x1) / 2 + 4, y1 + 0.75 + 0.3, P.floor)); // back side
  let b = union(parts);

  // USB opening in the left wall, centred on the board's short edge
  const usbZ = P.floor + P.nodeLift - P.usbH / 2 + 1.5;
  b = subtract(b, box(P.wall + 4, P.usbW, P.usbH, -OW / 2, (y0 + y1) / 2, usbZ));
  // buzzer slots in the left wall, behind the board
  for (let i = 0; i < P.slots; i++) {
    const y = P.slotY - (P.slots - 1) * 1.6 + i * 3.2;
    b = subtract(b, box(P.wall + 4, P.slotW, P.slotH, -OW / 2, y, P.slotZ - P.slotH / 2));
  }
  return b;
}

// ---------- lid (printed top face down) ----------
function lid() {
  let top = roundedCuboid({ size: [OW, OD, P.lid + 2 * P.corner], center: [0, 0, P.corner], roundRadius: P.corner, segments: 24 });
  top = subtract(top, box(OW + 2, OD + 2, P.corner * 2, 0, 0, -P.corner * 2));      // flat underside
  top = subtract(top, box(OW + 2, OD + 2, P.corner * 2, 0, 0, P.lid));              // flat top
  // lid modelled with its top face at z = 0..lid; lip and pockets hang below (negative z)
  const lw = P.innerW - 2 * P.fit, ld = P.innerD - 2 * P.fit;
  let lip = subtract(box(lw, ld, P.lipH, 0, 0, -P.lipH), box(lw - 2 * P.lipT, ld - 2 * P.lipT, P.lipH + 2, 0, 0, -P.lipH - 1));
  for (const [x, y] of postCenters) lip = subtract(lip, box(P.post + 1, P.post + 1, P.lipH + 2, x, y, -P.lipH - 1));
  const parts = [top, lip];

  const ring = (w, d, h, x, y) => subtract(box(w + 2.4, d + 2.4, h, x, y, -h), box(w, d, h + 2, x, y, -h - 1));
  parts.push(ring(P.oledW, P.oledH, P.oledPocket, P.oledX, P.oledY));
  for (const [x, y] of [P.btnMain, P.btnSecond]) parts.push(ring(P.btnBody, P.btnBody, P.btnPocket, x, y));
  let l = union(parts);

  l = subtract(l, box(P.winW, P.winH, P.lid + 2, P.oledX, P.oledY + P.winOffY, -1));
  for (const [x, y] of [P.btnMain, P.btnSecond]) l = subtract(l, hole(P.btnHole, P.lid + 2, x, y, -1));
  l = subtract(l, hole(P.knobHole, P.lid + 2, P.knob[0], P.knob[1], -1));
  for (const [x, y] of postCenters) l = subtract(l, hole(P.screwHole, P.lid + 2, x, y, -1));
  // flip so the top face is on the print bed (z = 0) and the lip points up
  return translate([0, 0, P.lid], transforms.rotateX(Math.PI, l));  // a rotation, not a mirror, so holes still match the base
}

// ---------- drawing (SVG, editable in Figma) ----------
function drawing() {
  const s = 5, pad = 40, W = OW * s, D = OD * s;
  const X = (x) => pad + (x + OW / 2) * s, Y = (y) => pad + (OD / 2 - y) * s;
  const r = (x, y, w, h, extra) => `<rect x="${X(x - w / 2)}" y="${Y(y + h / 2)}" width="${w * s}" height="${h * s}" ${extra}/>`;
  const c = (x, y, d, extra) => `<circle cx="${X(x)}" cy="${Y(y)}" r="${d / 2 * s}" ${extra}/>`;
  const t = (x, y, txt, extra = '') => `<text x="${X(x)}" y="${Y(y)}" ${extra}>${txt}</text>`;
  const sideY = pad + D + 90, SH = (BH + P.lid) * s;
  return `<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${W + 2 * pad + 260} ${sideY + SH + 80}" font-family="Inter, Arial, sans-serif" font-size="13">
<title>xonraOS case drawing</title>
<rect width="100%" height="100%" fill="#000"/>
<text x="${pad}" y="26" font-size="18" font-weight="700" fill="#fff">xonraOS case — top of lid (${OW} × ${OD} mm)</text>
<rect x="${pad}" y="${pad}" width="${W}" height="${D}" rx="${P.corner * s}" fill="#1c1c1c" stroke="#ddd" stroke-width="2"/>
${r(P.oledX, P.oledY, P.oledW, P.oledH, 'fill="none" stroke="#555" stroke-dasharray="6 4"')}
${r(P.oledX, P.oledY + P.winOffY, P.winW, P.winH, 'fill="#0b3d5c" stroke="#4da3ff" stroke-width="2"')}
${t(P.oledX - 7, P.oledY + P.winOffY - 1, 'OLED', 'fill="#7fdbff"')}
${c(P.btnMain[0], P.btnMain[1], 8, 'fill="#4da3ff"')}${t(P.btnMain[0] - 4, P.btnMain[1] - 7, 'D7 main', 'fill="#ddd"')}
${c(P.btnSecond[0], P.btnSecond[1], 8, 'fill="#4da3ff"')}${t(P.btnSecond[0] - 4, P.btnSecond[1] - 7, 'D6', 'fill="#ddd"')}
${c(P.knob[0], P.knob[1], 16, 'fill="#333" stroke="#ddd" stroke-width="2"')}${c(P.knob[0], P.knob[1], P.knobHole, 'fill="#000"')}${t(P.knob[0] - 4, P.knob[1] - 11, 'Knob', 'fill="#ddd"')}
${postCenters.map(([x, y]) => c(x, y, P.screwHole, 'fill="#000" stroke="#888"')).join('')}
${r(P.nodeX + P.nodeL / 2, -P.innerD / 2 + 0.4 + P.nodeW / 2, P.nodeL, P.nodeW, 'fill="none" stroke="#e9c46a" stroke-dasharray="3 4"')}
${t(P.nodeX + 3, -P.innerD / 2 + 4, 'NodeMCU inside (upside down)', 'fill="#e9c46a" font-size="11"')}
<g fill="#aaa" font-size="12">
<text x="${pad + W + 24}" y="${pad + 20}" fill="#fff" font-weight="700">Holes</text>
<text x="${pad + W + 24}" y="${pad + 44}">Screen window ${P.winW} × ${P.winH}</text>
<text x="${pad + W + 24}" y="${pad + 64}">Buttons Ø${P.btnHole} (6×6 tall stem)</text>
<text x="${pad + W + 24}" y="${pad + 84}">Knob Ø${P.knobHole} (panel pot)</text>
<text x="${pad + W + 24}" y="${pad + 104}">Screws Ø${P.screwHole} (M3)</text>
<text x="${pad + W + 24}" y="${pad + 124}">USB ${P.usbW} × ${P.usbH} (left side)</text>
<text x="${pad + W + 24}" y="${pad + 144}">Sound slots (left side)</text>
</g>
<text x="${pad}" y="${sideY - 14}" font-size="16" font-weight="700" fill="#fff">Left side (${OD} × ${BH + P.lid} mm)</text>
<rect x="${pad}" y="${sideY}" width="${D}" height="${SH}" rx="6" fill="#1c1c1c" stroke="#ddd" stroke-width="2"/>
<line x1="${pad}" y1="${sideY + P.lid * s}" x2="${pad + D}" y2="${sideY + P.lid * s}" stroke="#555"/>
${(() => { const yc = -P.innerD / 2 + 0.4 + P.nodeW / 2, zc = P.floor + P.nodeLift + 1.5 - P.usbH / 2;
  const sx = (y) => pad + (y + OD / 2) * s, sz = (z) => sideY + SH - z * s; // viewed from the left: front (−y) on the left
  let out = `<rect x="${sx(yc - P.usbW / 2)}" y="${sz(zc + P.usbH)}" width="${P.usbW * s}" height="${P.usbH * s}" fill="#000" stroke="#4da3ff" stroke-width="2"/><text x="${sx(yc - 3)}" y="${sz(zc) + 18}" fill="#7fdbff">USB</text>`;
  for (let i = 0; i < P.slots; i++) { const y = P.slotY - (P.slots - 1) * 1.6 + i * 3.2;
    out += `<rect x="${sx(y - P.slotW / 2)}" y="${sz(P.slotZ + P.slotH / 2)}" width="${P.slotW * s}" height="${P.slotH * s}" fill="#000" stroke="#888"/>`; }
  return out + `<text x="${sx(P.slotY - 7)}" y="${sz(P.slotZ + P.slotH / 2) - 8}" fill="#aaa">buzzer</text>`; })()}
</svg>`;
}

const out = __dirname;
const write = (name, geom) => {
  fs.writeFileSync(path.join(out, name), Buffer.concat(stl.serialize({ binary: true }, geom).map((a) => Buffer.from(a))));
  const [[a, b, c], [d, e, f]] = measureBoundingBox(geom);
  console.log(`${name}: ${(d - a).toFixed(1)} x ${(e - b).toFixed(1)} x ${(f - c).toFixed(1)} mm, ${(measureVolume(geom) / 1000).toFixed(1)} cm3`);
};
write('base.stl', base());
write('lid.stl', lid());
fs.writeFileSync(path.join(out, 'case-drawing.svg'), drawing());
console.log('case-drawing.svg written');
