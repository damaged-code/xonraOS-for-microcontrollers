//Notes|6|Notes
// Long notes scroll. Tap = down one line, D6 = up one line, hold = reload.
// Turn the knob (more than a little) and it takes over: the text position follows the knob.
// The bottom right shows the knob reading (K0 to K100) so you can see it move.
let body = 'Loading...';
let started = 0;
let off = 0;
let maxoff = 0;
let lastk = 0;
let grab = 0;
let setoff = function(v) {
  let o = v;
  if (o > maxoff) { o = maxoff; }
  if (o < 0) { o = 0; }
  off = o;
};
let draw = function() {
  if (started === 0) {
    started = 1;
    body = http(server() + '/notes.txt');
    if (body.length === 0) { body = 'No notes (offline?)'; }
    maxoff = lines(body) * 8 - 40;
    if (maxoff < 0) { maxoff = 0; }
    off = 0;
    grab = 0;
    lastk = knob();
  }
  let k = knob();
  if (grab === 0) {
    if (k - lastk > 40) { grab = 1; }
    if (lastk - k > 40) { grab = 1; }
  }
  if (grab === 1) { setoff(div(k * maxoff, 1023)); }
  text(0, 12 - off, 1, body);
  clear(0, 0, 128, 11);
  clear(0, 54, 128, 10);
  text(0, 0, 1, 'Notes');
  text(0, 56, 1, 'Tap down D6 up');
  text(102, 56, 1, 'K' + str(div(k * 100, 1023)));
  if (maxoff > 0) {
    line(127, 12, 127, 52);
    rect(126, 12 + div(off * 32, maxoff), 2, 8, 1);
  }
};
let tap = function() { grab = 0; lastk = knob(); setoff(off + 8); };
let prev = function() { grab = 0; lastk = knob(); setoff(off - 8); };
let hold = function() { started = 0; body = 'Loading...'; };
