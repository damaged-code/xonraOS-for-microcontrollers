//Lane Dodge|1|Dodge
// Switch between two lanes to dodge incoming blocks. Tap switches lanes, hold starts; best score is saved.
let state = 0;
let lane = 0;
let obstacle = 0;
let x = 120;
let points = 0;
let best = 0;
let ready = 0;
let stamp = 0;
let seed = 71;
let draw = function() {
  if (ready === 0) { best = num(load()); if (best < 0 || best > 9999) { best = 0; } ready = 1; }
  if (state === 1) {
    let now = ms(); if (now < stamp) { stamp = now; }
    if (now - stamp >= 40) {
      stamp = now; let speed = 3 + div(points, 5); if (speed > 8) { speed = 8; } x = x - speed;
      if (x <= 25 && x + 8 >= 16 && lane === obstacle) {
        state = 2; beep(180, 250); if (points > best) { best = points; save(str(best)); }
      }
      if (x < 0 && state === 1) { points = points + 1; x = 120; seed = mod(seed * 251 + mod(now, 65521) + 13849, 65521); obstacle = mod(seed, 2); }
    }
  }
  if (state === 1) {
    text(1, 0, 1, 'Score ' + str(points)); line(0, 34, 127, 34);
    rect(16, 19 + lane * 24, 9, 8, 1); rect(x, 19 + obstacle * 24, 8, 8, 0);
    ctext(56, 1, 'Tap switch lanes');
  } else {
    ctext(2, 2, 'LANE DODGE'); let label = 'Dodge the blocks'; if (state === 2) { label = 'Score ' + str(points); }
    ctext(26, 1, label); ctext(40, 1, 'Best ' + str(best)); ctext(56, 1, 'Hold start Tap lane');
  }
};
let tap = function() { lane = 1 - lane; };
let prev = function() { lane = 1 - lane; };
let hold = function() { state = 1; lane = 0; x = 120; points = 0; seed = mod(seed + ms(), 65521); obstacle = mod(seed, 2); stamp = ms(); };
