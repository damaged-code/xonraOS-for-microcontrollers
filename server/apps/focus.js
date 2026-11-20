//Focus|1|Focus
// A 25-minute focus session followed by a 5-minute break. Keep this app open to time it.
let phase = 0;
let left = 1500000;
let active = 0;
let stamp = 0;
let rounds = 0;
let pad = function(v) { let s = str(v); if (v < 10) { s = '0' + s; } return s; };
let advance = function() {
  if (phase === 0) { phase = 1; left = 300000; rounds = rounds + 1; }
  else { phase = 0; left = 1500000; }
  active = 0; beep(1320, 400);
};
let tick = function() {
  if (active === 1) {
    let now = ms(); if (now < stamp) { stamp = now; }
    left = left - (now - stamp); stamp = now;
    if (left <= 0) { advance(); }
  }
};
let draw = function() {
  tick();
  let title = 'FOCUS 25 MIN'; if (phase === 1) { title = 'BREAK 5 MIN'; }
  ctext(2, 1, title);
  let secs = div(left + 999, 1000);
  ctext(18, 3, pad(div(secs, 60)) + ':' + pad(mod(secs, 60)));
  let status = 'Ready'; if (active === 1) { status = 'Running'; }
  ctext(44, 1, status + '  Done ' + str(rounds));
  ctext(56, 1, 'Tap go/pause Hold rst');
};
let tap = function() { tick(); active = 1 - active; stamp = ms(); };
let hold = function() { phase = 0; left = 1500000; active = 0; rounds = 0; };
let prev = function() { active = 0; if (phase === 0) { left = 1500000; } else { left = 300000; } };
