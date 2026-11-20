//Calculator|6|Calc
let keys = '0123456789.+-*/=BCN';
let layout = '789/456*123-N0.+CB=';
let key = 0;
let value = 0;
let scale = 0;
let sign = 1;
let left = 0;
let op = 0;
let fresh = 1;
let error = 0;
let resetcalc = function() { value = 0; scale = 0; sign = 1; left = 0; op = 0; fresh = 1; error = 0; };
let shown = '0';
let sx = 34;
let sy = 46;
let sw = 28;
let operatorlabel = '';
let formatvalue = function() {

  let whole = num(str(value));
  let fraction = num(str((value - whole) * 1000000 + 0.5));
  if (fraction >= 1000000) { whole = whole + 1; fraction = 0; }
  shown = str(whole);
  if (fraction > 0) {
    let tail = sub(str(1000000 + fraction), 1, 6);
    while (sub(tail, tail.length - 1, 1) === '0') { tail = sub(tail, 0, tail.length - 1); }
    shown = shown + '.' + tail;
  }
  if (sign < 0 && value > 0) { shown = '-' + shown; }
  if (error === 1) { shown = 'Error - select C'; }
  operatorlabel = '';
  if (op > 0) { operatorlabel = sub(keys, op, 1); }
};
let draw = function() {
  ctext(2, 1, shown);
  text(1, 2, 1, operatorlabel);
  line(0, 13, 127, 13);
  text(15, 20, 1, '7    8    9    /');
  text(15, 29, 1, '4    5    6    *');
  text(15, 38, 1, '1    2    3    -');
  text(15, 47, 1, 'N    0    .    +');
  text(15, 56, 1, 'C    B      =');
  rect(sx, sy, sw, 9, 0);
};
let selectcell = function(cell) {
  key = find(keys, sub(layout, cell, 1));
  sx = 4 + mod(cell, 4) * 30;
  sy = 19 + div(cell, 4) * 9;
  sw = 28;
  if (cell === 18) { sw = 58; }
};
let tap = function() {
  let cell = find(layout, sub(keys, key, 1));
  selectcell(mod(cell + 1, 19));
};
let prev = function() {
  let cell = find(layout, sub(keys, key, 1));
  selectcell(mod(cell + 18, 19));
};
let resetentry = function() { value = 0; scale = 0; sign = 1; fresh = 0; };
let digit = function() {
  if (fresh === 1) { resetentry(); }
  if (key < 10 && scale === 0 && value < 100000000) { value = value * 10 + key; }
  if (key < 10 && scale > 0 && scale < 1000000) { value = value + key / scale; scale = scale * 10; }
  if (key === 10 && scale === 0) { scale = 10; }
};
let backspace = function() {
  if (scale === 0) { value = div(value, 10); }
  if (scale === 10) { scale = 0; }
  if (scale > 10) { scale = scale / 10; value = div(value * scale + 0.000001, 10) * 10 / scale; }
};
let compute = function() {
  let b = sign * value;
  if (op === 0) { left = b; }
  if (op === 11) { left = left + b; }
  if (op === 12) { left = left - b; }
  if (op === 13) { left = left * b; }
  if (op === 14 && b === 0) { error = 1; }
  if (op === 14 && b !== 0) { left = left / b; }
};
let operate = function() {
  if (fresh === 0) { compute(); }
  if (left > 999999999 || left < -999999999) { error = 1; }
  value = left;
  if (left < 0) { value = 0 - left; }
  sign = 1;
  if (left < 0) { sign = -1; }
  scale = 0;
  op = key;
  if (key === 15) { op = 0; }
  fresh = 1;
};
let hold = function() {
  if (key === 17) { resetcalc(); }
  if (error === 0 && key < 11) { digit(); }
  if (error === 0 && key === 18) { sign = 0 - sign; }
  if (error === 0 && key === 16) { backspace(); }
  if (error === 0 && key >= 11 && key <= 15) { operate(); }
  formatvalue();
};
