//Breathe|1|Calm
// Follow a gentle expanding circle: breathe in for 4 seconds and out for 6. Tap to pause.
let active = 0;
let elapsed = 0;
let stamp = 0;
let tick = function() {
  if (active === 1) { let now = ms(); if (now < stamp) { stamp = now; } elapsed = elapsed + now - stamp; stamp = now; }
};
let draw = function() {
  tick();
  let part = mod(elapsed, 10000);
  let label = 'Breathe in';
  let radius = 5 + div(part * 13, 4000);
  if (part >= 4000) { label = 'Breathe out'; radius = 18 - div((part - 4000) * 13, 6000); }
  if (active === 0) { label = 'Ready / paused'; }
  ctext(0, 1, label);
  circle(64, 31, radius, 0);
  circle(64, 31, 2, 1);
  text(2, 26, 1, str(div(elapsed, 10000)));
  ctext(56, 1, 'Tap go/pause Hold rst');
};
let tap = function() { tick(); active = 1 - active; stamp = ms(); };
let hold = function() { elapsed = 0; active = 0; };
let prev = function() { tick(); active = 0; };
