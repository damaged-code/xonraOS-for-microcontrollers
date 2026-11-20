//Knob Test|2|Knob
// Shows the potentiometer on A0: raw value 0-1023, percent and a bar.
// Tap = sound on/off (pitch follows the knob). Hold = reset min/max.
let sound = 0;
let lo = 1023;
let hi = 0;
let draw = function() {
  let v = knob();
  let pct = div(v * 100, 1023);
  if (v < lo) { lo = v; }
  if (v > hi) { hi = v; }
  text(0, 0, 1, 'Knob test');
  ctext(12, 3, str(pct) + '%');
  text(0, 38, 1, 'raw ' + str(v) + ' min ' + str(lo) + ' max ' + str(hi));
  rect(0, 48, 128, 8, 0);
  rect(2, 50, div(v * 124, 1023), 4, 1);
  let hint = 'Tap: sound on';
  if (sound === 1) { hint = 'Tap: sound off'; beep(200 + v, 60); }
  text(0, 58, 1, hint);
};
let tap = function() { sound = 1 - sound; if (sound === 0) { beep(0, 0); } };
let hold = function() { lo = 1023; hi = 0; };
