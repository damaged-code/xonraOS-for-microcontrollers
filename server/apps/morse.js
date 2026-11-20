//Morse|1|Morse
// Browse A-Z and 0-9 Morse codes and hold to hear the selected character. Tap next, D6 previous.
let alphabet = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
let codes = '.- -... -.-. -.. . ..-. --. .... .. .--- -.- .-.. -- -. --- .--. --.- .-. ... - ..- ...- .-- -..- -.-- --.. ----- .---- ..--- ...-- ....- ..... -.... --... ---.. ----.';
let selected = 0;
let pattern = '.-';
let active = 0;
let pos = 0;
let sounding = 0;
let stamp = 0;
let duration = 0;
let select = function() {
  let offset = 0; let i = 0;
  while (i < selected) { let tail = sub(codes, offset, 159); offset = offset + find(tail, ' ') + 1; i = i + 1; }
  let rest = sub(codes, offset, 6); let end = find(rest, ' '); if (end < 0) { end = rest.length; }
  pattern = sub(rest, 0, end); active = 0; beep(0, 0);
};
let draw = function() {
  let now = ms(); if (now < stamp) { stamp = now; }
  if (active === 1 && now - stamp >= duration) {
    stamp = now;
    if (sounding === 1) { sounding = 0; duration = 160; pos = pos + 1; }
    else {
      if (pos >= pattern.length) { active = 0; } else {
        duration = 160; if (sub(pattern, pos, 1) === '-') { duration = 480; }
        sounding = 1; beep(750, duration);
      }
    }
  }
  ctext(0, 1, 'MORSE CODE'); ctext(12, 3, sub(alphabet, selected, 1)); ctext(38, 2, pattern);
  ctext(56, 1, 'Tap next Hold play');
};
let tap = function() { selected = mod(selected + 1, 36); select(); };
let prev = function() { selected = mod(selected + 35, 36); select(); };
let hold = function() { pos = 0; sounding = 0; stamp = ms(); duration = 0; active = 1; };
