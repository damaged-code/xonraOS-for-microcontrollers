//Countdown|1|Timer
// Set 1-60 minutes, pause or resume, and hear a completion tone. Keep the app open.
let mins = 5;
let left = 300000;
let active = 0;
let stamp = 0;
let done = 0;
let pad = function(v) { let s = str(v); if (v < 10) { s = '0' + s; } return s; };
let tick = function() {
  if (active === 1) {
    let now = ms();
    if (now < stamp) { stamp = now; }
    left = left - (now - stamp); stamp = now;
    if (left <= 0) { left = 0; active = 0; done = 1; beep(1568, 700); }
  }
};
let draw = function() {
  tick();
  ctext(2, 1, 'COUNTDOWN');
  let secs = div(left + 999, 1000);
  ctext(18, 3, pad(div(secs, 60)) + ':' + pad(mod(secs, 60)));
  let label = 'Ready / paused';
  if (active === 1) { label = 'Running - stay here'; }
  if (done === 1) { label = 'Time is up!'; }
  ctext(44, 1, label);
  ctext(56, 1, 'Tap +1 Hold go/pause');
};
let tap = function() {
  if (active === 0) { mins = mod(mins, 60) + 1; left = mins * 60000; done = 0; }
};
let prev = function() {
  if (active === 0) { mins = mins - 1; if (mins < 1) { mins = 60; } left = mins * 60000; done = 0; }
};
let hold = function() {
  tick();
  if (active === 1) { active = 0; } else {
    if (left === 0) { left = mins * 60000; }
    done = 0; stamp = ms(); active = 1;
  }
};
