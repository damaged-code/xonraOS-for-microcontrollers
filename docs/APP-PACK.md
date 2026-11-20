# Ten more apps for xonraOS

These apps use the existing JavaScript API: no extra sensors or potentiometer are needed.
Enable **Settings > Sound** to hear tones through the optional buzzer. Set **Settings > D6 button > Back** for the secondary actions below.

## Install

Firmware **V2.0.3 (OTA build 5)** increases the Store list from 8 to 24 entries so all 16 catalog apps can appear. Earlier firmware reads only the first eight entries, which hides most of this pack. The board still holds **eight installed apps** at a time; remove an installed app in Store before adding another when full.

After this pack and firmware are published, update through **Settings > Firmware**, then open **Store** and hold on an app marked **GET**. These apps themselves need no firmware functions beyond V2.0.2.

| App | What it does | Tap D7 | Hold D7 (0.6 s, then release) | D6 Back |
|---|---|---|---|---|
| Countdown | 1–60 minute countdown with completion tone | Add a minute while stopped | Start / pause / resume | Subtract a minute while stopped |
| Focus | 25-minute work session, then 5-minute break | Start / pause / resume | Reset session and completed count | Reset current phase |
| Breathe | Expanding circle: 4 seconds in, 6 seconds out | Start / pause | Reset | Pause |
| Dice | D4, D6, D8, D12, D20; one or two dice | Roll | Change die size | Toggle one / two dice |
| Reaction | Wait for GO and measure response time | Start / react / retry | Return to ready screen | Return to ready screen |
| Math Quiz | Ten addition/subtraction questions | Next answer (0–18) | Submit, then next question | Previous answer |
| Scoreboard | Saved scores for two teams (0–99) | Add to selected team | Select other team | Subtract from selected team |
| Metronome | Four-beat visual and audible pulse, 40–200 BPM | Add 5 BPM | Start / stop | Subtract 5 BPM |
| Morse | Browse and hear A–Z and 0–9 | Next character | Play its dots and dashes | Previous character |
| Lane Dodge | Switch between two lanes to avoid blocks | Switch lane | Start / restart | Switch lane |

Holding D7 for **1.5 seconds** goes Home, as in every app. Scoreboard scores can be brought back to zero with D6 on each team.

## Behavior and limits

- **Keep timers open.** The firmware unloads an app when you leave it. Countdown, Focus, and Metronome do not run or alert in the background. Countdown and Focus preserve paused time only while open. Focus waits for a tap before starting each new work/break phase.
- Reaction saves the best response time; Lane Dodge saves the best completed score; Scoreboard saves both scores. Other state resets when the app is reopened. Removing an app also removes its saved data.
- Dice uses a small pseudorandom generator mixed with button timing. It is a casual game accessory, not a secure random generator.
- Metronome timing is limited by the roughly 25 FPS app loop. Reaction is a simple game, not a calibrated measurement tool.
- None of the ten apps makes network requests. They work offline after installation.

## Verification

The automated suite runs the real Elk 2.2.0 interpreter with the firmware's 12 KB script heap. It checks all button handlers, timer transitions, persistence, score bounds, all 36 Morse characters, sound calls, text bounds, and 1,200 frames per app. The native functions are simulated; this does **not** test physical buttons, OLED appearance, speaker output, or Wi-Fi installation on a board.

Compile `tools/harness.c` with the installed Elk sources, then run:

```sh
gcc -O1 -I /path/to/elk tools/harness.c /path/to/elk/elk.c -lm -o tools/harness
node tools/test-apps.js ./tools/harness
```

The harness also accepts `harness app.js scenario.tsv`. Scenario rows contain milliseconds, JavaScript to evaluate, and an optional expected result, separated by tabs. `ELK_SAVED` supplies the initial saved string. Saves persist for the lifetime of each test process.
