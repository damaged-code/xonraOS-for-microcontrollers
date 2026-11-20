//Scoreboard|1|Score
// Keep two team scores. Tap adds, hold selects a team, D6 subtracts. Scores are saved.
let home = 0;
let away = 0;
let team = 0;
let ready = 0;
let persist = function() { save(str(home) + ',' + str(away)); };
let draw = function() {
  if (ready === 0) {
    let record = load(); let split = find(record, ',');
    if (split >= 0) { home = num(sub(record, 0, split)); away = num(sub(record, split + 1, 4)); }
    if (home < 0 || home > 99) { home = 0; } if (away < 0 || away > 99) { away = 0; }
    ready = 1;
  }
  text(12, 1, 1, 'HOME'); text(84, 1, 1, 'AWAY');
  text(14, 22, 3, str(home)); text(80, 22, 3, str(away));
  line(64, 18, 64, 44);
  let x = 4; if (team === 1) { x = 70; } rect(x, 13, 54, 35, 0);
  ctext(56, 1, 'Tap + Hold team D6 -');
};
let tap = function() {
  if (team === 0 && home < 99) { home = home + 1; }
  if (team === 1 && away < 99) { away = away + 1; }
  persist();
};
let hold = function() { team = 1 - team; };
let prev = function() {
  if (team === 0 && home > 0) { home = home - 1; }
  if (team === 1 && away > 0) { away = away - 1; }
  persist();
};
