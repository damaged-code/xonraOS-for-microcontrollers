//Reaction|1|React
// Wait for GO, then tap as fast as you can. Early taps lose the round; your best time is saved.
let state = 0;
let stamp = 0;
let wait = 0;
let result = 0;
let best = 0;
let ready = 0;
let seed = 29;
let begin = function() { seed = mod(seed * 251 + mod(ms(), 65521) + 13849, 65521); wait = 1500 + mod(seed, 3000); stamp = ms(); state = 1; };
let draw = function() {
  if (ready === 0) { best = num(load()); if (best < 0 || best > 60000) { best = 0; } ready = 1; }
  if (state === 1) {
    if (ms() < stamp) { stamp = ms(); }
    if (ms() - stamp >= wait) { state = 2; stamp = ms(); }
  }
  ctext(0, 1, 'REACTION');
  if (state === 0) { ctext(20, 2, 'Ready?'); }
  if (state === 1) { ctext(20, 2, 'Wait...'); }
  if (state === 2) { rect(5, 13, 118, 34, 0); ctext(20, 3, 'GO!'); }
  if (state === 3) { ctext(19, 2, str(result) + ' ms'); }
  if (state === 4) { ctext(22, 2, 'Too soon!'); }
  if (state !== 2) { ctext(43, 1, 'Best ' + str(best) + ' ms'); }
  ctext(56, 1, 'Tap start / react');
};
let tap = function() {
  if (state === 1) { state = 4; beep(200, 150); } else {
    if (state === 2) {
      result = ms() - stamp; if (result < 0) { result = 0; }
      state = 3;
      if (best === 0 || result < best) { best = result; save(str(best)); }
      beep(1200, 60);
    } else { begin(); }
  }
};
let hold = function() { state = 0; };
let prev = function() { state = 0; };
