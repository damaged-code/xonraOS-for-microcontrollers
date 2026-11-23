#!/usr/bin/env node
// Run with: node tools/test-apps.js <compiled Elk harness>
// Uses the real 12 KB Elk VM, deterministic time, button events and saved data.
const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');
const assert = require('assert/strict');
const root = path.resolve(__dirname, '..');
const harness = path.resolve(process.argv[2] || 'tools/harness');
const temp = fs.mkdtempSync(path.join(os.tmpdir(), 'xonra-app-tests-'));
let count = 0;
const entries = fs.readFileSync(path.join(root, 'server/manifest.txt'), 'utf8').trim().split(/\r?\n/).map(s => s.split('|'));
const ids = ['countdown', 'focus', 'breathe', 'dice', 'reaction', 'mathquiz', 'scoreboard', 'metronome', 'morse', 'lanedodge'];

function run(id, label, rows, saved = '') {
  const file = path.join(temp, `${id}-${count++}.tsv`);
  fs.writeFileSync(file, rows.map(r => r.join('\t')).join('\n') + '\n');
  const result = spawnSync(harness, [path.join(root, 'server/apps', id + '.js'), file], {
    encoding: 'utf8', env: { ...process.env, ELK_SAVED: saved }, maxBuffer: 32 * 1024 * 1024, timeout: 30000,
  });
  assert.equal(result.status, 0, `${id} ${label}: ${result.error || result.stderr}\n${result.stdout?.slice(-2500)}`);
  assert.doesNotMatch(result.stdout, /ERROR/, `${id} ${label}`);
  assert.match(result.stdout, /(?:ctext|text|circle|rect)\(/, `${id} must draw`);
  // Every text call must fit the 128x64 OLED with the firmware's 6x8 font cells.
  for (const line of result.stdout.split('\n')) {
    let m = line.match(/^  ctext\((-?\d+),(\d+),"(.*)"\)/);
    if (m) { assert.ok(+m[1] >= 0 && +m[1] + 8 * +m[2] <= 64 && m[3].length * 6 * +m[2] <= 128, `${id}: text clips: ${line}`); }
    m = line.match(/^  text\((-?\d+),(-?\d+),(\d+),"(.*)"\)/);
    if (m) { assert.ok(+m[1] >= 0 && +m[2] >= 0 && +m[1] + m[4].length * 6 * +m[3] <= 128 && +m[2] + 8 * +m[3] <= 64, `${id}: text clips: ${line}`); }
  }
  const usage = [...result.stdout.matchAll(/heap (\d+)%/g)].map(m => +m[1]);
  console.log(`PASS ${id}: ${label} (${rows.length} steps, peak heap ${Math.max(...usage)}%)`);
  return result.stdout;
}

try {
  for (const id of ids) {
    const entry = entries.find(e => e[0] === id);
    assert.ok(entry, `${id} missing from manifest`);
    const code = fs.readFileSync(path.join(root, 'server/apps', id + '.js'), 'utf8');
    const header = code.split(/\r?\n/)[0].slice(2).split('|');
    assert.equal(header[0], entry[1]); assert.equal(header[1], entry[2]);
    assert.ok(header[0].length <= 11 && header[2].length <= 5 && Buffer.byteLength(code) < 6000);
    run(id, 'buttons and redraw', [[1000, 'draw();'], [1200, 'tap();'], [1300, 'draw();'], [2000, 'hold();'], [2100, 'draw();'], [2200, 'prev();'], [2300, 'draw();']]);
  }
  assert.equal(new Set(entries.map(e => e[0])).size, entries.length);
  const firmware = fs.readFileSync(path.join(root, 'firmware/xonraOS/xonraOS.ino'), 'utf8');
  assert.ok(entries.length <= +firmware.match(/const int ST_MAX = (\d+)/)[1]);
  assert.ok(fs.statSync(path.join(root, 'server/manifest.txt')).size < 2000);

  const timer = run('countdown', 'pause, resume, finish once, restart and bounds', [
    [0, 'draw(); hold();'], [60000, 'draw(); left;', '240000'], [61000, 'hold(); left;', '239000'],
    [90000, 'draw(); left;', '239000'], [100000, 'hold();'], [339000, 'draw(); done;', '1'],
    [340000, 'draw();'], [341000, 'hold(); active;', '1'], [341000, 'left;', '300000'],
    [342000, 'hold(); mins = 60; tap(); mins;', '1'], [342000, 'prev(); mins;', '60'], [342000, 'draw();'],
  ]);
  assert.equal((timer.match(/beep\(1568,700\)/g) || []).length, 1);
  run('focus', 'focus complete, break complete, pause and reset', [
    [0, 'draw(); tap();'], [1500000, 'draw(); phase;', '1'], [1500000, 'rounds;', '1'], [1500000, 'active;', '0'],
    [1500100, 'tap();'], [1800100, 'draw(); phase;', '0'], [1800100, 'rounds;', '1'],
    [1800200, 'tap();'], [1801200, 'tap(); left;', '1499000'], [1900000, 'draw(); left;', '1499000'],
    [1900000, 'hold(); rounds;', '0'], [1900000, 'draw();'],
  ]);
  run('breathe', 'in/out cycle, pause, reset', [
    [0, 'draw(); tap();'], [4000, 'draw(); elapsed;', '4000'], [10000, 'draw(); elapsed;', '10000'],
    [11000, 'tap();'], [20000, 'draw(); elapsed;', '11000'], [20000, 'hold(); elapsed;', '0'], [20000, 'draw();'],
  ]);
  const diceRows = [[0, 'draw();']];
  for (let i = 1; i <= 150; i++) {
    diceRows.push([i * 173, 'tap(); first >= 1 && first <= sides && other >= 1 && other <= sides;', 'true']);
    if (i % 10 === 0) diceRows.push([i * 173, 'hold(); prev(); draw();']);
  }
  run('dice', '150 rolls across all die sizes', diceRows);
  run('reaction', 'early tap, GO response, saved best', [
    [0, 'draw(); tap();'], [1, 'tap(); state;', '4'], [10, 'tap();'], [6000, 'draw(); state;', '2'],
    [6234, 'tap(); result;', '234'], [6234, 'best;', '234'], [6234, 'draw();'], [6234, 'load();', '"234"'],
    [7000, 'tap();'], [13000, 'draw();'], [13500, 'tap(); best;', '234'], [13500, 'draw();'],
  ]);
  run('reaction', 'restore best', [[0, 'draw(); best;', '234']], '234');
  const quizRows = [[0, 'draw();']];
  for (let i = 0; i < 10; i++) {
    quizRows.push([i * 1000 + 1, 'choice = answer; hold(); phase;', '1']);
    quizRows.push([i * 1000 + 2, 'draw(); hold();']);
  }
  quizRows.push([11000, 'draw(); points;', '10'], [11000, 'phase;', '2'], [12000, 'hold(); question;', '1'], [12000, 'draw();']);
  run('mathquiz', 'ten correct answers and new round', quizRows);
  run('mathquiz', 'wrong answer and answer cycling', [[0, 'draw(); choice = mod(answer + 1, 19); hold(); points;', '0'], [1, 'draw(); hold(); choice = 0; prev(); choice;', '18'], [2, 'tap(); choice;', '0'], [2, 'draw();']]);
  run('scoreboard', 'restore, score each team, subtract and cap', [
    [0, 'draw(); home;', '12'], [1, 'away;', '9'], [2, 'tap(); home;', '13'], [3, 'hold(); tap(); away;', '10'],
    [4, 'prev(); away;', '9'], [5, 'load();', '"13,9"'], [6, 'away = 99; tap(); away;', '99'],
    [7, 'away = 0; prev(); away;', '0'], [8, 'draw();'],
  ], '12,9');
  run('scoreboard', 'invalid saved scores', [[0, 'draw(); home;', '0'], [0, 'away;', '0']], '-1,100');
  const metro = run('metronome', 'beat timing, silence on stop, BPM bounds', [
    [0, 'draw(); hold();'], [600, 'draw(); beat;', '1'], [2400, 'draw(); beat;', '0'],
    [2500, 'hold(); active;', '0'], [5000, 'draw(); beat;', '0'], [5000, 'bpm = 200; tap(); bpm;', '40'],
    [5001, 'prev(); bpm;', '200'], [5001, 'draw();'],
  ]);
  assert.match(metro, /beep\(0,0\)/);
  const expectedCodes = '.- -... -.-. -.. . ..-. --. .... .. .--- -.- .-.. -- -. --- .--. --.- .-. ... - ..- ...- .-- -..- -.-- --.. ----- .---- ..--- ...-- ....- ..... -.... --... ---.. ----.'.split(' ');
  const morseRows = [[0, 'draw();']];
  expectedCodes.forEach((code, i) => { morseRows.push([i * 100, 'pattern;', JSON.stringify(code)]); morseRows.push([i * 100 + 1, 'draw(); tap();']); });
  morseRows.push([4000, 'selected;', '0'], [4000, 'prev(); pattern;', '"----."'], [4000, 'draw();']);
  run('morse', 'all 36 characters and wraparound', morseRows);
  const morse = run('morse', 'A dot/dash playback', [[0, 'draw(); hold(); draw();'], [160, 'draw();'], [320, 'draw();'], [800, 'draw();'], [960, 'draw(); active;', '0']]);
  assert.match(morse, /beep\(750,160\)/); assert.match(morse, /beep\(750,480\)/);
  run('lanedodge', 'dodge, collision, best score, restart', [
    [0, 'draw(); hold(); obstacle = 0; lane = 1; x = 1;'], [40, 'draw(); points;', '1'],
    [41, 'x = 26; obstacle = lane;'], [80, 'draw(); state;', '2'], [80, 'best;', '1'], [80, 'load();', '"1"'],
    [100, 'hold(); points;', '0'], [100, 'state;', '1'], [100, 'draw();'],
  ]);

  for (const id of ids) {
    let start = 'draw(); hold();';
    if (id === 'focus' || id === 'breathe' || id === 'reaction') start = 'draw(); tap();';
    const rows = [[0, start]];
    for (let i = 1; i <= 1200; i++) {
      let action = 'draw();';
      if (id === 'dice' && i % 10 === 0) action = 'tap(); draw();';
      if (id === 'morse' && i % 40 === 0) action = 'tap(); hold(); draw();';
      if (id === 'lanedodge') action = 'lane = 1 - obstacle; draw();';
      if (id === 'scoreboard' && i % 30 === 0) action = 'hold(); tap(); draw();';
      if (id === 'mathquiz' && i % 30 === 0) action = 'hold(); draw();';
      rows.push([i * 40, action]);
    }
    rows.push([1, 'draw();']); // clock wrap/backwards jump must not hang or produce an error
    run(id, '1200-frame endurance and clock wrap', rows);
  }
  console.log(`All ${count} scenarios passed; ${ids.length} new apps, ${entries.length} catalog entries.`);
} finally {
  fs.rmSync(temp, { recursive: true, force: true });
}
