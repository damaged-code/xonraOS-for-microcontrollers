# Writing apps for xonraOS

Apps for the OS are small JavaScript files. The board downloads them from your [app server](../server/README.md) through the **Store** app and runs them with [Elk](https://github.com/cesanta/elk), a tiny JavaScript engine. You do not reflash the board to add or change an app.

This guide is for people and for AI coding agents. Read the **Elk rules** section carefully: the engine is only a subset of JavaScript and has several bugs that fail silently. Test every app with the [test harness](#test-an-app-on-your-computer) before you publish it.

## App file

`server/apps/<id>.js`, with a header comment on the first line:

```
//Name|version|Short
```

- `Name`: shown in the Store (max 11 characters).
- `version`: any short text (max 7 characters). **Change it whenever the app changes**, and use the same value in `server/manifest.txt`, or the Store will not offer the update.
- `Short`: label on the Home tile, max **5 characters** (optional, defaults to Name).
- `id` is the file name without `.js`: lowercase letters and digits, max 13 characters. It must match the manifest line `id|Name|version|Category`.
- Keep the script under about 6 KB. The engine has a 12 KB heap that holds the code and all variables, and the download limit is 9000 bytes.

Then define any of these global functions (all optional):

| Function | Runs when |
|---|---|
| `draw()` | every frame (about 25 per second). The screen is cleared before it. |
| `tap()` | the main button (D7) is tapped |
| `hold()` | the main button is held 0.6 s (acts on release) |
| `prev()` | the second button (D6) is pressed, when Settings > D6 button is "Back" |

Holding the main button 1.5 s always goes Home (the OS does this). Call `exit()` to leave from code.

Minimal app:

```js
//Hello|1|Hello
let n = 0;
let draw = function() {
  ctext(8, 1, 'Hello');
  ctext(24, 3, str(n));
  ctext(54, 1, 'Tap = +1');
};
let tap = function() { n = n + 1; };
```

## Functions your app can call

The screen is 128x64, one colour. Text size 1 is 6x8 pixels (21 characters per line), size 2 is 12x16, size 3 is 18x24. Text wraps at the right edge.

| Call | Does |
|---|---|
| `text(x, y, size, s)` | draw text at pixel x, y |
| `ctext(y, size, s)` | draw text centred |
| `rect(x, y, w, h, fill)` | rectangle; fill 1 = solid, 0 = outline |
| `clear(x, y, w, h)` | fill a rectangle with black. Use it to clip scrolled content: draw it, then `clear` the header and footer bands and redraw them |
| `line(x0, y0, x1, y1)` | line |
| `dot(x, y)` | one pixel |
| `circle(x, y, r, fill)` | circle |
| `str(n)` | integer to string |
| `num(s)` | string to integer (0 if not a number) |
| `find(s, t)` | index of `t` in `s`, or -1 |
| `sub(s, start, len)` | substring (results up to 159 characters) |
| `lines(s)` | how many text lines size-1 text `s` takes (21 columns, `\n` breaks) |
| `div(a, b)`, `mod(a, b)` | integer divide and remainder (there is no Math) |
| `ms()` | milliseconds since boot (wraps after about 12 days) |
| `timeok()`, `hour()`, `minute()`, `second()` | local time, valid when `timeok()` is 1 |
| `wifi()` | 1 if connected |
| `http(url)` | blocking HTTP GET, returns the body (max 1000 characters), or "" on failure. Plain `http://` only, up to about 5 s |
| `server()` | the app server's base URL from `secrets.h`, for example `http(server() + '/data.txt')` |
| `save(s)` / `load()` | one saved string per app (max 159 characters). `load()` gives "" if nothing is saved |
| `knob()` | potentiometer on A0, 0 to 1023 (percent: `div(knob() * 100, 1023)`). Reads noise if no knob is wired |
| `beep(freq, ms)` | buzzer on D8: tone at `freq` Hz for `ms` ms (max 5000), returns immediately. `beep(0, 0)` stops it. Muted by Settings > Sound |
| `exit()` | go back to Home |

Pass real numbers to number arguments and real strings to string arguments, or you get `ERROR: bad arg N`.

## Elk rules

Elk is a small JavaScript subset.

Language limits:
- End every statement with `;`. Use `let` only (no `var`, no `const`).
- Compare with `===` and `!==` only (no `==`, `!=`).
- No `for`, `do` or `switch`: use `while`. No arrays or `[]` indexing, no closures, no `this`, no `new`, no arrow functions, no template strings, no `Math`, `Date`, `String` or `JSON`.
- Functions: `let f = function(a, b) { ... };`. Objects work: `{x: 1}`.
- `+` joins strings, and both sides must be strings (use `str(n)`). `s.length` works. `\n` works in strings.
- Globals keep their value between frames, so keep app state in globals. `let` inside functions works.

Bugs that fail silently (found by testing):
- **A bare `if (x)` never runs its block**, even when `x` is true or 1. `if (!x)`, `if (x === 1)` and `if (a < b)` work. Keep flags as the numbers 0 and 1 and test them with `=== 1`. Toggle with `x = 1 - x`. Do not use `=== true`: it breaks the whole function.
- **A `return` inside an `if` block makes the caller return too.** Never write `if (x) { return a; } return b;`. Assign to a variable inside the `if` and return it once at the end.
- **Ternaries (`a ? b : c`) are unreliable.** As a call argument the function silently stops drawing; in an assignment the result can be wrong. Do not use `?:`. Write `let r = a; if (cond) { r = b; }`.
- Do not name your variables like a built-in function (`text`, `str`, `line`, `load`, `ms`...), and avoid names that start with one (`loaded`).

Behaviour to design around:
- The OS is single threaded. An endless `while` loop freezes the board until you reset it. `http()` blocks the screen for up to about 5 s, so call it on start or on a tap, never every frame.
- Redraw everything every frame: the screen only shows what `draw()` paints now.
- Errors appear as small text on the bottom line of the screen.
- Put a hint on the last line (y = 54 to 56), under 21 characters, such as `Tap +1 Hold 0`.
- Up to 8 apps can be installed at once.

## Test an app on your computer

`tools/harness.c` runs an app with the real Elk engine and fake versions of the functions above, with the same 12 KB heap as the board. You need a C compiler and Elk's two source files (`elk.c`, `elk.h`) from the Arduino library "elk" 2.2.0 or from the [Elk repository](https://github.com/cesanta/elk).

```
gcc -O1 -o harness tools/harness.c elk.c -lm      # elk.c and elk.h next to it or on the include path
./harness server/apps/myapp.js
```

It loads the script, then calls `draw(); tap(); draw(); hold(); draw(); prev(); draw();` and prints every drawing call and every `ERROR`. (`ERROR: 'tap' not found` for a handler you did not write is fine.) A script that prints no drawing calls is broken even when no error shows. Change the fake functions at the top of `harness.c` (for example make `http` return real-looking data) to test other situations.

For deterministic time and state assertions, pass a scenario TSV as the second argument (milliseconds, expression, optional expected result). A scenario failure exits nonzero. `ELK_SAVED` sets the starting saved value; `save()` updates what `load()` returns in that process. Heap usage is reported as a percentage of the 12 KB heap. Run `node tools/test-apps.js <harness-path>` for the ten-app pack tests; see [APP-PACK.md](APP-PACK.md).

## Publish

1. Put `<id>.js` in `server/apps/`.
2. Add or update the line in `server/manifest.txt` and **bump the version**.
3. On the board: Store, hold the main button on the app (`GET` installs, `UPD` updates).

The manifest line is `id|Name|version|Category`. Use `Tools`, `Games`, `Timers` or `Sound` for the category; anything else (or leaving it out) shows under `Other`. The Store lists the categories in that order and the website shows them as sections.

## Add a new function to the OS

If an app needs something the API lacks (a new sensor, for example), add a C function in `firmware/xonraOS/xonraOS.ino` and register it in `scriptLoad()` with `reg("name", (uintptr_t)fn, "signature")`. The signature's first letter is the return type, then the argument types: `i` integer, `s` string, `v` void. Use only `i`, `s` and `v`: doubles break Elk's call convention on this chip. Then add it to the harness and to this document, and reflash the firmware.
