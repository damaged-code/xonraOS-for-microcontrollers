//Dice|1|Dice
// Roll a D4, D6, D8, D12 or D20 using tap timing. Hold changes sides; D6 toggles two dice.
let sides = 6;
let pair = 0;
let first = 1;
let other = 1;
let seed = 17;
let rolls = 0;
let random = function() { seed = mod(seed * 251 + mod(ms(), 65521) + 13849, 65521); return mod(seed, sides) + 1; };
let tap = function() { first = random(); other = random(); rolls = rolls + 1; beep(900, 40); };
let hold = function() {
  if (sides === 4) { sides = 6; } else {
    if (sides === 6) { sides = 8; } else {
      if (sides === 8) { sides = 12; } else { if (sides === 12) { sides = 20; } else { sides = 4; } }
    }
  }
  first = 1; other = 1;
};
let prev = function() { pair = 1 - pair; };
let draw = function() {
  let title = '1 x D'; if (pair === 1) { title = '2 x D'; }
  ctext(1, 1, title + str(sides));
  if (pair === 0) { ctext(19, 3, str(first)); ctext(44, 1, 'Roll ' + str(rolls)); }
  else { ctext(18, 2, str(first) + ' + ' + str(other)); ctext(40, 1, 'Total ' + str(first + other)); }
  ctext(56, 1, 'Tap roll Hold sides');
};
