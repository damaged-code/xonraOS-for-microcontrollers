// Host test harness for Store apps: runs an app script with the real Elk engine and fake natives.
// Build (Linux/macOS/WSL):  gcc -O1 -o harness harness.c elk.c -lm
//   (elk.c / elk.h come from the Arduino library "elk" 2.2.0, e.g. ...\Arduino\libraries\elk\)
// Run:  ./harness path/to/app.js
// It loads the script, then calls draw(), tap(), draw(), hold(), draw(), prev(), draw() and prints
// every drawing call and every ERROR. A script that prints no drawing calls or shows ERROR is broken.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include "elk.h"

static char rb[4][160];
static int ri = 0;
static char *nb() { ri = (ri + 1) & 3; rb[ri][0] = 0; return rb[ri]; }
void n_text(int x, int y, int s, char *t) { printf("  text(%d,%d,%d,\"%s\")\n", x, y, s, t); }
void n_ctext(int y, int s, char *t) { printf("  ctext(%d,%d,\"%s\")\n", y, s, t); }
void n_rect(int a, int b, int c, int d, int e) { printf("  rect(%d,%d,%d,%d,%d)\n", a, b, c, d, e); }
void n_line(int a, int b, int c, int d) { printf("  line(%d,%d,%d,%d)\n", a, b, c, d); }
void n_dot(int a, int b) { printf("  dot(%d,%d)\n", a, b); }
void n_circle(int a, int b, int c, int d) { printf("  circle(%d,%d,%d,%d)\n", a, b, c, d); }
const char *n_str(int v) { char *b = nb(); snprintf(b, 160, "%d", v); return b; }
int n_num(char *s) { return atoi(s); }
int n_find(char *s, char *t) { char *p = strstr(s, t); return p ? (int)(p - s) : -1; }
const char *n_sub(char *s, int i, int n) {
  char *b = nb(); int l = strlen(s);
  if (i < 0) i = 0; if (i > l) i = l; if (n > l - i) n = l - i; if (n < 0) n = 0;
  memcpy(b, s + i, n); b[n] = 0; return b;
}
int n_div(int a, int b) { return b ? a / b : 0; }
int n_mod(int a, int b) { return b ? a % b : 0; }
static int fakems = 1000;
int n_ms() { return fakems; }
int n_one() { return 1; }
void n_exit() { printf("  exit()\n"); }
void n_clear(int x, int y, int w, int h) { printf("  clear(%d,%d,%d,%d)\n", x, y, w, h); }
int n_lines(char *s) { int l = 1, c = 0; for (; *s; s++) { if (*s == '\n') { l++; c = 0; } else if (*s != '\r') { if (++c > 21) { l++; c = 1; } } } return l; }
const char *n_server() { return "http://example.local:8266"; }
int n_knob() { static int k = 100; k += 300; return k % 1024; }  // fake potentiometer
void n_beep(int f, int d) { printf("  beep(%d,%d)\n", f, d); }
void n_save(char *s) { printf("  save(%s)\n", s); }
const char *n_load() { char *b = nb(); strcpy(b, "5"); return b; }
const char *n_http(char *u) {  // fake body: 300 chars with line breaks, like a long note
  static char big[400];
  printf("  http(%s)\n", u);
  strcpy(big, "Line one of a long note\nSecond line that is long enough to wrap around the screen\nThird\nFourth\nFifth\nSixth line here\nSeventh\nEighth line goes on and on until it wraps\nNinth\nTenth\nEleventh\nTwelfth");
  return big;
}

struct js *J;
void reg(const char *n, void *f, const char *sig) { js_set(J, js_glob(J), n, js_import(J, (uintptr_t)f, sig)); }

int main(int argc, char **argv) {
  static char heap[12000];  // same size as the board
  J = js_create(heap, sizeof heap);
  reg("text", n_text, "viiis"); reg("ctext", n_ctext, "viis"); reg("rect", n_rect, "viiiii");
  reg("line", n_line, "viiii"); reg("dot", n_dot, "vii"); reg("circle", n_circle, "viiii");
  reg("str", n_str, "si"); reg("num", n_num, "is"); reg("find", n_find, "iss"); reg("sub", n_sub, "ssii");
  reg("div", n_div, "iii"); reg("mod", n_mod, "iii"); reg("ms", n_ms, "i");
  reg("timeok", n_one, "i"); reg("hour", n_one, "i"); reg("minute", n_one, "i"); reg("second", n_one, "i");
  reg("wifi", n_one, "i"); reg("http", n_http, "ss"); reg("save", n_save, "vs"); reg("load", n_load, "s");
  reg("exit", n_exit, "v"); reg("beep", n_beep, "vii"); reg("knob", n_knob, "i"); reg("clear", n_clear, "viiii"); reg("lines", n_lines, "is"); reg("server", n_server, "s");
  FILE *f = fopen(argv[1], "r");
  if (!f) { printf("cannot open %s\n", argv[1]); return 1; }
  static char code[9000];
  size_t n = fread(code, 1, sizeof code - 1, f); code[n] = 0; fclose(f);
  jsval_t r = js_eval(J, code, ~0);
  printf("load: %s\n", js_str(J, r));
  const char *calls[] = { "draw();", "tap();", "draw();", "hold();", "draw();", "prev();", "draw();" };
  for (int i = 0; i < 7; i++) {
    fakems += 1234;
    r = js_eval(J, calls[i], ~0);
    printf("%s -> %s (heap used %d of %d)\n", calls[i], js_str(J, r), js_usage(J), (int)sizeof heap);
  }
  return 0;
}
