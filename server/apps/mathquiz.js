//Math Quiz|1|Math
// Ten addition or subtraction questions. Tap cycles answers, hold submits, D6 cycles backward.
let a = 0;
let b = 0;
let answer = 0;
let choice = 0;
let points = 0;
let question = 1;
let phase = 0;
let op = 0;
let seed = 43;
let ready = 0;
let random = function() { seed = mod(seed * 251 + mod(ms(), 65521) + 13849, 65521); return mod(seed, 10); };
let ask = function() {
  a = random(); b = random(); op = mod(random(), 2);
  if (op === 1 && b > a) { let temp = a; a = b; b = temp; }
  answer = a + b; if (op === 1) { answer = a - b; }
  choice = 0; phase = 0;
};
let draw = function() {
  if (ready === 0) { ask(); ready = 1; }
  ctext(0, 1, 'Q ' + str(question) + '/10  Score ' + str(points));
  if (phase === 2) {
    ctext(18, 3, str(points) + '/10'); ctext(44, 1, 'Round complete'); ctext(56, 1, 'Hold to play again');
  } else {
    let sign = ' + '; if (op === 1) { sign = ' - '; }
    ctext(14, 2, str(a) + sign + str(b) + ' = ?');
    if (phase === 0) { ctext(34, 2, str(choice)); ctext(56, 1, 'Tap answer Hold OK'); }
    else { ctext(35, 1, 'Answer ' + str(answer)); ctext(56, 1, 'Hold next question'); }
  }
};
let tap = function() { if (phase === 0) { choice = mod(choice + 1, 19); } };
let prev = function() { if (phase === 0) { choice = mod(choice + 18, 19); } };
let hold = function() {
  if (phase === 0) {
    if (choice === answer) { points = points + 1; beep(1400, 70); } else { beep(220, 150); }
    phase = 1;
  } else {
    if (phase === 2) { points = 0; question = 1; ask(); } else {
      if (question === 10) { phase = 2; } else { question = question + 1; ask(); }
    }
  }
};
