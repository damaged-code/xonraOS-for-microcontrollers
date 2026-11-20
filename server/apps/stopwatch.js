//Stopwatch|4|Stopw
// Elk bugs avoided: flags are numbers (0/1) tested with === 1, no bare `if (x)`, no ternaries,
// no `return` inside an if-block. See ../AGENTS.md.
let run = 0;
let start = 0;
let acc = 0;
let pad = function(v) { let r = str(v); if (v < 10) { r = '0' + r; } return r; };
let cur = function() { let r = acc; if (run === 1) { r = acc + ms() - start; } return r; };
let draw = function() {
  let t = cur();
  let s = div(t, 1000);
  ctext(6, 1, 'Stopwatch');
  ctext(26, 2, pad(div(s, 60)) + ':' + pad(mod(s, 60)) + '.' + str(div(mod(t, 1000), 100)));
  ctext(54, 1, 'Tap go/stop Hold 0');
};
let tap = function() { if (run === 1) { acc = acc + ms() - start; run = 0; } else { start = ms(); run = 1; } };
let hold = function() { run = 0; acc = 0; };
