//Bounce|3|Ball
let paddle = 49;
let bx = 64;
let by = 48;
let vx = 1;
let vy = -2;
let points = 0;
let caption = 'Score 0';
let state = 0;
let last = 0;
let pitch = 0;
let length = 0;
let tap = function() { paddle = paddle + 9; if (paddle > 97) { paddle = 97; } };
let prev = function() { paddle = paddle - 9; if (paddle < 1) { paddle = 1; } };
let hold = function() {
  paddle = 49; bx = 64; by = 48; vx = 1; vy = -2;
  points = 0; pitch = 0; caption = 'Score 0'; last = ms(); state = 1;
  beep(880, 70);
};
let step = function() {
  last = ms();
  bx = bx + vx; by = by + vy;
  if (bx < 3) { bx = 3; vx = 0 - vx; pitch = 650; length = 20; }
  if (bx > 124) { bx = 124; vx = 0 - vx; pitch = 650; length = 20; }
  if (by < 13) { by = 13; vy = 2; pitch = 650; length = 20; }
  let hit = 0;
  if (vy > 0 && by >= 54 && by <= 56) { hit = 1; }
  if (bx < paddle - 2 || bx > paddle + 31) { hit = 0; }
  if (hit === 1) {
    by = 54; vy = -2;
    vx = 1;
    points = points + 1;
    pitch = 1200; length = 35;
  }
  if (hit === 1 && bx < paddle + 15) { vx = -1; }
  if (hit === 1 && bx < paddle + 7) { vx = -2; }
  if (hit === 1 && bx > paddle + 23) { vx = 2; }
  if (by > 65) { state = 2; pitch = 180; length = 250; }
};
let arena = function() {
  text(2, 0, 1, caption);
  line(0, 10, 127, 10);
  rect(paddle, 57, 30, 3, 1);
  circle(bx, by, 2, 1);
};
let welcome = function() {
  ctext(3, 2, 'BOUNCE');
  ctext(24, 1, 'Keep the ball up!');
  ctext(36, 1, 'D6 left  D7 right');
  ctext(54, 1, 'Hold to start');
};
let finished = function() {
  ctext(5, 2, 'GAME OVER');
  ctext(29, 1, caption);
  ctext(54, 1, 'Hold to restart');
};
let draw = function() {
  if (state === 1 && ms() - last >= 50) { step(); }
  if (pitch > 0) { beep(pitch, length); pitch = 0; caption = 'Score ' + str(points); }
  if (state === 1 && ms() < last) { last = ms(); }
  if (state === 0) { welcome(); }
  if (state === 1) { arena(); }
  if (state === 2) { finished(); }
};
