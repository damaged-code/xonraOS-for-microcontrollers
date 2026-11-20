//Counter|3|Count
let n = 0;
let ready = 0;
let draw = function() {
  if (ready === 0) { n = num(load()); ready = 1; }
  ctext(6, 1, 'Counter');
  ctext(22, 3, str(n));
  ctext(54, 1, 'Tap +1 Hold 0 D6 -1');
};
let tap = function() { n = n + 1; save(str(n)); };
let hold = function() { n = 0; save('0'); };
let prev = function() { if (n > 0) { n = n - 1; save(str(n)); } };
