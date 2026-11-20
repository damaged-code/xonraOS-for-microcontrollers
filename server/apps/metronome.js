//Metronome|1|Metro
// A visual and audible 4-beat metronome, 40-200 BPM. Tap +5, D6 -5, hold starts or stops.
let bpm = 100;
let active = 0;
let beat = 0;
let stamp = 0;
let draw = function() {
  let now = ms();
  if (now < stamp) { stamp = now; }
  if (active === 1 && now - stamp >= div(60000, bpm)) {
    let ticks = div(now - stamp, div(60000, bpm));
    stamp = stamp + ticks * div(60000, bpm); beat = mod(beat + ticks, 4);
    let pitch = 900; if (beat === 0) { pitch = 1500; } beep(pitch, 45);
  }
  ctext(0, 1, 'METRONOME'); ctext(13, 2, str(bpm) + ' BPM');
  let i = 0;
  while (i < 4) { let fill = 0; if (active === 1 && i === beat) { fill = 1; } circle(28 + i * 24, 41, 6, fill); i = i + 1; }
  ctext(56, 1, 'Tap +5 Hold go/stop');
};
let tap = function() { bpm = bpm + 5; if (bpm > 200) { bpm = 40; } stamp = ms(); };
let prev = function() { bpm = bpm - 5; if (bpm < 40) { bpm = 200; } stamp = ms(); };
let hold = function() { active = 1 - active; stamp = ms(); beat = 0; if (active === 1) { beep(1500, 45); } else { beep(0, 0); } };
