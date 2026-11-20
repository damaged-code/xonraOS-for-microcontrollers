// xonraOS for Raspberry Pi 3B: runs the same JavaScript apps as the ESP8266 build.
// A 128x64 SSD1306 OLED on I2C, two buttons and a buzzer on GPIO, optional knob on an MCP3008.
// Build and wiring: see pi/README.md.   License: GPL-3.0 (same as the firmware).
//
// Build for the Pi:   make            (needs elk.c and elk.h, see the Makefile)
// Build the simulator (any PC, prints the screen as text):   make sim
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "elk.h"
#include "font5x7.h"

#if defined(__linux__) && !defined(XONRA_SIM)
#define HW 1
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netdb.h>
#include <linux/i2c-dev.h>
#include <linux/spi/spidev.h>
#else
#define HW 0
#endif

#ifdef _WIN32
#define mkdir(p, m) mkdir(p)  /* simulator on Windows */
#endif

#define W 128
#define H 64

// ---------- configuration (~/.xonraos/config.txt, key=value per line) ----------
static struct {
  char server[160];       // app server, plain http:// only
  char i2c[40];           // I2C device
  int addr;               // OLED address
  int pinMain, pinSecond, pinBuzzer;  // BCM numbers; -1 disables
  int d6;                 // second button: 0 off, 1 home, 2 back
  int use24, sound, flip, bright;
  int mcpChannel;         // MCP3008 channel for knob(), -1 = no knob
} cfg = { "", "/dev/i2c-1", 0x3C, 17, 27, 18, 2, 1, 1, 0, 207, -1 };

static char homeDir[200];

static void load_config(void) {
  const char *h = getenv("XONRA_HOME");
  if (h) snprintf(homeDir, sizeof homeDir, "%s", h);
  else snprintf(homeDir, sizeof homeDir, "%s/.xonraos", getenv("HOME") ? getenv("HOME") : ".");
  char p[260];
  mkdir(homeDir, 0755);
  snprintf(p, sizeof p, "%s/apps", homeDir);
  mkdir(p, 0755);
  snprintf(p, sizeof p, "%s/config.txt", homeDir);
  FILE *f = fopen(p, "r");
  if (!f) return;
  char line[200];
  while (fgets(line, sizeof line, f)) {
    line[strcspn(line, "\r\n")] = 0;
    char *eq = strchr(line, '=');
    if (line[0] == '#' || !eq) continue;
    *eq++ = 0;
    int v = (int)strtol(eq, NULL, 0);
    if (!strcmp(line, "server")) snprintf(cfg.server, sizeof cfg.server, "%s", eq);
    else if (!strcmp(line, "i2c")) snprintf(cfg.i2c, sizeof cfg.i2c, "%s", eq);
    else if (!strcmp(line, "addr")) cfg.addr = v;
    else if (!strcmp(line, "main")) cfg.pinMain = v;
    else if (!strcmp(line, "second")) cfg.pinSecond = v;
    else if (!strcmp(line, "buzzer")) cfg.pinBuzzer = v;
    else if (!strcmp(line, "d6")) cfg.d6 = !strcmp(eq, "off") ? 0 : !strcmp(eq, "home") ? 1 : 2;
    else if (!strcmp(line, "use24")) cfg.use24 = v;
    else if (!strcmp(line, "sound")) cfg.sound = v;
    else if (!strcmp(line, "flip")) cfg.flip = v;
    else if (!strcmp(line, "brightness")) cfg.bright = v;
    else if (!strcmp(line, "knob")) cfg.mcpChannel = v;
  }
  fclose(f);
}

static unsigned ms_now(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (unsigned)(t.tv_sec * 1000u + t.tv_nsec / 1000000u);
}

// ---------- 1-bit framebuffer (SSD1306 page layout) and drawing ----------
static uint8_t fb[8][W];

static void px(int x, int y, int c) {
  if (x < 0 || x >= W || y < 0 || y >= H) return;
  if (c) fb[y >> 3][x] |= (uint8_t)(1 << (y & 7));
  else fb[y >> 3][x] &= (uint8_t)~(1 << (y & 7));
}
static void fillrect(int x, int y, int w, int h, int c) {
  for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) px(x + i, y + j, c);
}
static void outline(int x, int y, int w, int h, int c) {
  for (int i = 0; i < w; i++) { px(x + i, y, c); px(x + i, y + h - 1, c); }
  for (int j = 0; j < h; j++) { px(x, y + j, c); px(x + w - 1, y + j, c); }
}
static void gline(int x0, int y0, int x1, int y1, int c) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1, err = dx + dy;
  for (;;) {
    px(x0, y0, c);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}
static void gcircle(int cx, int cy, int r, int fill, int c) {
  for (int y = -r; y <= r; y++)
    for (int x = -r; x <= r; x++) {
      int d = x * x + y * y;
      if (fill ? d <= r * r + r : (d <= r * r + r && d > (r - 1) * (r - 1) + (r - 1))) px(cx + x, cy + y, c);
    }
}
static void gchar(int x, int y, unsigned char ch, int s, int c) {
  for (int col = 0; col < 5; col++) {
    uint8_t bits = font[ch * 5 + col];
    for (int row = 0; row < 7; row++)
      if (bits & (1 << row)) fillrect(x + col * s, y + row * s, s, s, c);
  }
}
static void gtext(int x, int y, int s, const char *t, int c) {  // wraps at the right edge like the OLED library
  int cx = x;
  for (; *t; t++) {
    if (*t == '\n') { cx = 0; y += 8 * s; continue; }
    if (*t == '\r') continue;
    if (cx + 6 * s > W) { cx = 0; y += 8 * s; }
    gchar(cx, y, (unsigned char)*t, s, c);
    cx += 6 * s;
  }
}
static void gctext(int y, int s, const char *t, int c) {
  int w = (int)strlen(t) * 6 * s;
  gtext((W - w) / 2, y, s, t, c);
}

// ---------- hardware: OLED, GPIO, buzzer, knob ----------
#if HW
static int i2cfd = -1;
static void oled_cmd(uint8_t c) { uint8_t b[2] = { 0x00, c }; if (write(i2cfd, b, 2) < 0) {} }
static void oled_open(void) {
  i2cfd = open(cfg.i2c, O_RDWR);
  if (i2cfd < 0 || ioctl(i2cfd, I2C_SLAVE, cfg.addr) < 0) { perror("OLED (is I2C enabled? sudo raspi-config)"); exit(1); }
  const uint8_t init[] = { 0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0x8D, 0x14, 0x20, 0x00,
                           (uint8_t)(cfg.flip ? 0xA0 : 0xA1), (uint8_t)(cfg.flip ? 0xC0 : 0xC8), 0xDA, 0x12,
                           0x81, (uint8_t)cfg.bright, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF };
  for (unsigned i = 0; i < sizeof init; i++) oled_cmd(init[i]);
}
static void oled_flush(void) {
  const uint8_t pos[] = { 0x21, 0, 127, 0x22, 0, 7 };
  for (unsigned i = 0; i < sizeof pos; i++) oled_cmd(pos[i]);
  const uint8_t *p = &fb[0][0];
  for (int off = 0; off < W * 8; off += 64) {  // 64 data bytes per transfer
    uint8_t b[65];
    b[0] = 0x40;
    memcpy(b + 1, p + off, 64);
    if (write(i2cfd, b, 65) < 0) {}
  }
}

static volatile uint32_t *gp;
static void gpio_open(void) {
  int fd = open("/dev/gpiomem", O_RDWR | O_SYNC);
  if (fd < 0) { perror("/dev/gpiomem (add your user to the gpio group)"); exit(1); }
  gp = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (gp == MAP_FAILED) { perror("mmap"); exit(1); }
}
static void gpio_mode(int p, int out) {
  volatile uint32_t *r = gp + p / 10;
  int sh = (p % 10) * 3;
  *r = (*r & ~(7u << sh)) | ((out ? 1u : 0u) << sh);
}
static void gpio_pullup(int p) {  // BCM2835/2837 pull sequence
  gp[37] = 2; usleep(10);
  gp[38 + p / 32] = 1u << (p % 32); usleep(10);
  gp[37] = 0; gp[38 + p / 32] = 0;
}
static int gpio_read(int p) { return (gp[13 + p / 32] >> (p % 32)) & 1; }
static void gpio_write(int p, int v) { if (v) gp[7 + p / 32] = 1u << (p % 32); else gp[10 + p / 32] = 1u << (p % 32); }

// Buzzer: a thread toggles the pin at the requested frequency (fine for a small passive buzzer).
static volatile int buzFreq = 0;
static volatile unsigned buzUntil = 0;
static void *buzzer_thread(void *arg) {
  (void)arg;
  for (;;) {
    int f = buzFreq;
    if (f > 0 && (int)(buzUntil - ms_now()) > 0 && cfg.pinBuzzer >= 0) {
      gpio_write(cfg.pinBuzzer, 1); usleep(500000 / f);
      gpio_write(cfg.pinBuzzer, 0); usleep(500000 / f);
    } else {
      if (cfg.pinBuzzer >= 0) gpio_write(cfg.pinBuzzer, 0);
      usleep(5000);
    }
  }
  return NULL;
}
static void beep_hw(int f, int d) { buzUntil = ms_now() + (unsigned)d; buzFreq = f; }

static int spifd = -1;
static int knob_hw(void) {
  if (spifd < 0) return 512;
  uint8_t tx[3] = { 1, (uint8_t)((8 | cfg.mcpChannel) << 4), 0 }, rx[3] = { 0, 0, 0 };
  struct spi_ioc_transfer t = { 0 };
  t.tx_buf = (unsigned long)tx; t.rx_buf = (unsigned long)rx; t.len = 3; t.speed_hz = 1000000;
  if (ioctl(spifd, SPI_IOC_MESSAGE(1), &t) < 0) return 512;
  return ((rx[1] & 3) << 8) | rx[2];
}

// Plain HTTP GET (HTTP/1.0 so the body is never chunked). Returns body length or -1.
static int http_get(const char *url, char *out, int max) {
  if (strncmp(url, "http://", 7)) return -1;
  char host[100], port[8] = "80", path[160] = "/";
  const char *h = url + 7, *slash = strchr(h, '/');
  int hl = slash ? (int)(slash - h) : (int)strlen(h);
  if (hl <= 0 || hl >= (int)sizeof host) return -1;
  memcpy(host, h, hl); host[hl] = 0;
  if (slash) snprintf(path, sizeof path, "%s", slash);
  char *colon = strchr(host, ':');
  if (colon) { *colon = 0; snprintf(port, sizeof port, "%s", colon + 1); }
  struct addrinfo hints = { 0 }, *ai;
  hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(host, port, &hints, &ai)) return -1;
  int s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
  struct timeval tv = { 5, 0 };
  setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
  setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
  if (s < 0 || connect(s, ai->ai_addr, ai->ai_addrlen) < 0) { if (s >= 0) close(s); freeaddrinfo(ai); return -1; }
  freeaddrinfo(ai);
  char req[400];
  int rl = snprintf(req, sizeof req, "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: xonraOS-Pi\r\nConnection: close\r\n\r\n", path, host);
  if (write(s, req, rl) < 0) { close(s); return -1; }
  static char buf[20000];
  int n = 0, r;
  while (n < (int)sizeof buf - 1 && (r = (int)read(s, buf + n, sizeof buf - 1 - n)) > 0) n += r;
  close(s);
  buf[n] = 0;
  if (strncmp(buf, "HTTP/1.", 7) || !strstr(buf, " 200")) return -1;
  char *body = strstr(buf, "\r\n\r\n");
  if (!body) return -1;
  body += 4;
  int bl = n - (int)(body - buf);
  if (bl > max - 1) bl = max - 1;
  memcpy(out, body, bl); out[bl] = 0;
  return bl;
}
#else  // simulator: no hardware; the "server" is a folder on this computer
static void beep_hw(int f, int d) { if (f > 0) printf("[beep %d Hz %d ms]\n", f, d); }
static int knob_hw(void) { return 512; }
static int http_get(const char *url, char *out, int max) {
  const char *p = strstr(url, "://");
  p = p ? strchr(p + 3, '/') : NULL;
  if (!p) return -1;
  const char *dir = getenv("XONRA_SIM_SERVER");
  char path[300];
  snprintf(path, sizeof path, "%s%s", dir ? dir : "server", p);
  FILE *f = fopen(path, "rb");
  if (!f) return -1;
  int n = (int)fread(out, 1, max - 1, f);
  out[n] = 0;
  fclose(f);
  return n;
}
#endif

// ---------- script runtime (Elk, same natives and 12 KB heap as the ESP8266 build) ----------
static char rb[4][160];
static int ri = 0;
static char *nb(void) { ri = (ri + 1) & 3; rb[ri][0] = 0; return rb[ri]; }

static struct js *J;
static int scriptExit = 0;
static char scriptId[16];
static int soundOn(void) { return cfg.sound; }

void n_text(int x, int y, int s, char *t) { gtext(x, y, s, t, 1); }
void n_ctext(int y, int s, char *t) { gctext(y, s, t, 1); }
void n_rect(int x, int y, int w, int h, int fill) { if (fill) fillrect(x, y, w, h, 1); else outline(x, y, w, h, 1); }
void n_clear(int x, int y, int w, int h) { fillrect(x, y, w, h, 0); }
void n_line(int a, int b, int c, int d) { gline(a, b, c, d, 1); }
void n_dot(int x, int y) { px(x, y, 1); }
void n_circle(int x, int y, int r, int fill) { gcircle(x, y, r, fill, 1); }
const char *n_str(int v) { char *b = nb(); snprintf(b, 160, "%d", v); return b; }
int n_num(char *s) { return atoi(s); }
int n_find(char *s, char *t) { char *p = strstr(s, t); return p ? (int)(p - s) : -1; }
const char *n_sub(char *s, int i, int n) {
  char *b = nb(); int l = (int)strlen(s);
  if (i < 0) i = 0;
  if (i > l) i = l;
  if (n > l - i) n = l - i;
  if (n > 159) n = 159;
  if (n < 0) n = 0;
  memcpy(b, s + i, n); b[n] = 0;
  return b;
}
int n_div(int a, int b) { return b ? a / b : 0; }
int n_mod(int a, int b) { return b ? a % b : 0; }
int n_ms(void) { return (int)ms_now(); }
int n_one(void) { return 1; }
int n_hour(void) { time_t t = time(NULL); return localtime(&t)->tm_hour; }
int n_minute(void) { time_t t = time(NULL); return localtime(&t)->tm_min; }
int n_second(void) { time_t t = time(NULL); return localtime(&t)->tm_sec; }
void n_exit(void) { scriptExit = 1; }
int n_lines(char *s) { int l = 1, c = 0; for (; *s; s++) { if (*s == '\n') { l++; c = 0; } else if (*s != '\r') { if (++c > 21) { l++; c = 1; } } } return l; }
const char *n_server(void) { char *b = nb(); snprintf(b, 160, "%s", cfg.server); return b; }
int n_knob(void) { return knob_hw(); }
void n_beep(int f, int d) { if (!soundOn() && f != 0) return; if (d > 5000) d = 5000; beep_hw(f, d); }
const char *n_http(char *u) {
  static char body[1001];
  if (http_get(u, body, sizeof body) < 0) body[0] = 0;
  return body;
}
static void dataPath(char *p, size_t n) { snprintf(p, n, "%s/d_%s.txt", homeDir, scriptId); }
void n_save(char *s) {
  char p[300]; dataPath(p, sizeof p);
  FILE *f = fopen(p, "w");
  if (f) { fwrite(s, 1, strnlen(s, 159), f); fclose(f); }
}
const char *n_load(void) {
  char p[300]; dataPath(p, sizeof p);
  char *b = nb();
  FILE *f = fopen(p, "r");
  if (f) { size_t n = fread(b, 1, 159, f); b[n] = 0; fclose(f); }
  return b;
}

static void reg(const char *n, void *f, const char *sig) { js_set(J, js_glob(J), n, js_import(J, (uintptr_t)f, sig)); }

static char scriptErr[40];

static int scriptLoad(const char *id) {
  char p[300];
  snprintf(p, sizeof p, "%s/apps/%s.js", homeDir, id);
  static char code[9001];
  FILE *f = fopen(p, "r");
  if (!f) return 0;
  size_t n = fread(code, 1, sizeof code - 1, f);
  code[n] = 0;
  fclose(f);
  snprintf(scriptId, sizeof scriptId, "%s", id);
  static char heap[12000];  // same size as the board, so an app that fits here fits there
  J = js_create(heap, sizeof heap);
  reg("text", n_text, "viiis"); reg("ctext", n_ctext, "viis"); reg("rect", n_rect, "viiiii");
  reg("line", n_line, "viiii"); reg("dot", n_dot, "vii"); reg("circle", n_circle, "viiii");
  reg("str", n_str, "si"); reg("num", n_num, "is"); reg("find", n_find, "iss"); reg("sub", n_sub, "ssii");
  reg("div", n_div, "iii"); reg("mod", n_mod, "iii"); reg("ms", n_ms, "i");
  reg("timeok", n_one, "i"); reg("hour", n_hour, "i"); reg("minute", n_minute, "i"); reg("second", n_second, "i");
  reg("wifi", n_one, "i"); reg("http", n_http, "ss"); reg("save", n_save, "vs"); reg("load", n_load, "s");
  reg("exit", n_exit, "v"); reg("beep", n_beep, "vii"); reg("knob", n_knob, "i"); reg("clear", n_clear, "viiii");
  reg("lines", n_lines, "is"); reg("server", n_server, "s");
  scriptExit = 0;
  scriptErr[0] = 0;
  jsval_t r = js_eval(J, code, ~0);
  const char *s = js_str(J, r);
  if (!strncmp(s, "ERROR", 5)) snprintf(scriptErr, sizeof scriptErr, "%s", s);
  return 1;
}
static void scriptCall(const char *call) {
  if (!J) return;
  const char *s = js_str(J, js_eval(J, call, ~0));
  if (!strncmp(s, "ERROR", 5) && !strstr(s, "not found")) snprintf(scriptErr, sizeof scriptErr, "%s", s);
}

// ---------- apps: Home, Clock, Store, installed scripts ----------
enum { A_HOME, A_CLOCK, A_STORE, A_SCRIPT };
enum { EV_TAP, EV_HOLD, EV_HOME, EV_BACK };
static int app = A_HOME, homeSel = 0, topRow = 0;

typedef struct { char id[16], name[16], ver[10], tag[8]; } Inst;
static Inst inst[16];
static int instN = 0;

static void parseHeader(const char *path, Inst *a) {  // first line: //Name|version|Short
  FILE *f = fopen(path, "r");
  char l[100] = "";
  if (f) { if (!fgets(l, sizeof l, f)) l[0] = 0; fclose(f); }
  l[strcspn(l, "\r\n")] = 0;
  char *name = l, *ver = "", *tag = "";
  if (!strncmp(l, "//", 2)) name = l + 2;
  char *b1 = strchr(name, '|');
  if (b1) { *b1++ = 0; ver = b1; char *b2 = strchr(b1, '|'); if (b2) { *b2++ = 0; tag = b2; } }
  snprintf(a->name, sizeof a->name, "%.15s", name);
  snprintf(a->ver, sizeof a->ver, "%.9s", ver);
  snprintf(a->tag, sizeof a->tag, "%.5s", *tag ? tag : name);
}
static int cmpInst(const void *a, const void *b) { return strcmp(((const Inst *)a)->id, ((const Inst *)b)->id); }
static void scanApps(void) {
  char dir[260];
  snprintf(dir, sizeof dir, "%s/apps", homeDir);
  DIR *d = opendir(dir);
  instN = 0;
  if (!d) return;
  struct dirent *e;
  while ((e = readdir(d)) && instN < 16) {
    size_t n = strlen(e->d_name);
    if (n < 4 || strcmp(e->d_name + n - 3, ".js") || n - 3 >= sizeof inst[0].id) continue;
    Inst *a = &inst[instN];
    memcpy(a->id, e->d_name, n - 3); a->id[n - 3] = 0;
    char p[300];
    snprintf(p, sizeof p, "%s/%s", dir, e->d_name);
    parseHeader(p, a);
    instN++;
  }
  closedir(d);
  qsort(inst, instN, sizeof inst[0], cmpInst);
}
static int homeN(void) { return 2 + instN; }

// Store
typedef struct { char id[16], name[16], ver[10]; } Item;
static Item items[24];
static int itemN = 0, storeSel = 0, storeTop = 0;
enum { ST_LOAD, ST_LIST, ST_FAIL, ST_BUSY };
static int storeState = ST_LOAD, busyAt = -1;
static char storeMsg[24];

static const char *itemStatus(int i) {
  for (int k = 0; k < instN; k++)
    if (!strcmp(inst[k].id, items[i].id)) return strcmp(inst[k].ver, items[i].ver) ? "UPD" : "OK";
  return "GET";
}
static void storeFetchList(void) {
  static char body[1001];
  itemN = 0;
  if (!cfg.server[0]) { storeState = ST_FAIL; snprintf(storeMsg, sizeof storeMsg, "Set server in config"); return; }
  char url[260];
  snprintf(url, sizeof url, "%s/manifest.txt", cfg.server);
  if (http_get(url, body, sizeof body) < 0) { storeState = ST_FAIL; snprintf(storeMsg, sizeof storeMsg, "Can't reach server"); return; }
  for (char *l = strtok(body, "\n"); l && itemN < 24; l = strtok(NULL, "\n")) {
    l[strcspn(l, "\r")] = 0;
    char *b1 = strchr(l, '|'), *b2 = b1 ? strchr(b1 + 1, '|') : NULL;
    if (!b2) continue;
    *b1 = 0; *b2 = 0;
    snprintf(items[itemN].id, sizeof items[0].id, "%s", l);
    snprintf(items[itemN].name, sizeof items[0].name, "%s", b1 + 1);
    snprintf(items[itemN].ver, sizeof items[0].ver, "%s", b2 + 1);
    itemN++;
  }
  scanApps();
  storeState = itemN ? ST_LIST : ST_FAIL;
  if (!itemN) snprintf(storeMsg, sizeof storeMsg, "Store is empty");
}
static void storeInstall(int i) {
  static char code[9001];
  char url[260], p[300];
  snprintf(url, sizeof url, "%s/apps/%s.js", cfg.server, items[i].id);
  int n = http_get(url, code, sizeof code);
  if (n <= 0) { storeState = ST_FAIL; snprintf(storeMsg, sizeof storeMsg, "Download failed"); return; }
  snprintf(p, sizeof p, "%s/apps/%s.js", homeDir, items[i].id);
  FILE *f = fopen(p, "w");
  if (!f) { storeState = ST_FAIL; snprintf(storeMsg, sizeof storeMsg, "Can't save"); return; }
  fwrite(code, 1, n, f); fclose(f);
  scanApps();
  storeState = ST_LIST;
}

static void openApp(int i) {
  if (i == 0) { app = A_CLOCK; return; }
  if (i == 1) { app = A_STORE; storeState = ST_LOAD; storeSel = storeTop = 0; return; }
  if (i - 2 < instN && scriptLoad(inst[i - 2].id)) app = A_SCRIPT;
}
static void goHome(void) { app = A_HOME; J = NULL; scanApps(); if (homeSel >= homeN()) homeSel = 0; }

static void onEvent(int ev) {
  if (ev == EV_HOME) { goHome(); return; }
  if (ev == EV_BACK) {
    if (cfg.d6 == 0) return;
    if (cfg.d6 == 1) { goHome(); return; }
  }
  switch (app) {
    case A_HOME:
      if (ev == EV_TAP) homeSel = (homeSel + 1) % homeN();
      else if (ev == EV_BACK) homeSel = (homeSel + homeN() - 1) % homeN();
      else if (ev == EV_HOLD) openApp(homeSel);
      break;
    case A_CLOCK:
      if (ev == EV_BACK) goHome();
      else if (ev == EV_HOLD) cfg.use24 = !cfg.use24;
      break;
    case A_STORE:
      if (storeState == ST_BUSY || storeState == ST_LOAD) break;
      if (storeState == ST_FAIL) { if (ev == EV_HOLD) storeState = ST_LOAD; else goHome(); break; }
      if (ev == EV_TAP) storeSel = (storeSel + 1) % itemN;
      else if (ev == EV_BACK) storeSel = (storeSel + itemN - 1) % itemN;
      else if (ev == EV_HOLD && strcmp(itemStatus(storeSel), "OK")) { storeState = ST_BUSY; busyAt = storeSel; }
      break;
    case A_SCRIPT:
      if (ev == EV_TAP) scriptCall("tap();");
      else if (ev == EV_HOLD) scriptCall("hold();");
      else if (ev == EV_BACK) scriptCall("prev();");
      break;
  }
}

// Work that blocks (network) runs after the screen has been drawn once, so the user sees a message.
static void runPending(void) {
  if (app != A_STORE) return;
  if (storeState == ST_LOAD) storeFetchList();
  else if (storeState == ST_BUSY && busyAt >= 0) { storeInstall(busyAt); busyAt = -1; }
}

static void drawHome(void) {
  int selRow = homeSel / 2;
  if (selRow < topRow) topRow = selRow;
  if (selRow > topRow + 1) topRow = selRow - 1;
  for (int i = 0; i < homeN(); i++) {
    int x = 4 + (i % 2) * 62, y = 9 + (i / 2 - topRow) * 24;
    if (y > 56 || y + 22 < 9) continue;
    int sel = i == homeSel;
    if (sel) fillrect(x, y, 58, 22, 1); else outline(x, y, 58, 22, 1);
    const char *label = i == 0 ? "Clock" : i == 1 ? "Store" : inst[i - 2].tag;
    int w = (int)strlen(label) * 6;
    gtext(x + (58 - w) / 2, y + 7, 1, label, !sel);
  }
  fillrect(0, 0, W, 9, 0);
  fillrect(0, 55, W, 9, 0);
  gtext(0, 0, 1, "Apps", 1);
  char cnt[24];
  snprintf(cnt, sizeof cnt, "%d/%d", homeSel + 1, homeN());
  gtext(W - (int)strlen(cnt) * 6, 0, 1, cnt, 1);
  gctext(56, 1, "Tap=next Hold=open", 1);
}
static void drawClock(void) {
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  int hr = tm->tm_hour;
  const char *ampm = "";
  if (!cfg.use24) { ampm = hr < 12 ? "AM" : "PM"; hr %= 12; if (hr == 0) hr = 12; }
  char hm[8], ss[4], dl[24];
  if (cfg.use24) snprintf(hm, sizeof hm, "%02d:%02d", hr, tm->tm_min); else snprintf(hm, sizeof hm, "%d:%02d", hr, tm->tm_min);
  if (ms_now() % 1000 >= 500) { char *c = strchr(hm, ':'); if (c) *c = ' '; }
  snprintf(ss, sizeof ss, "%02d", tm->tm_sec);
  int w = (int)strlen(hm) * 18;
  gtext(94 - w, 20, 3, hm, 1);
  gtext(100, 20, 2, ss, 1);
  if (!cfg.use24) gtext(104, 40, 1, ampm, 1);
  strftime(dl, sizeof dl, "%a %d %b", tm);
  gctext(2, 1, dl, 1);
  fillrect(0, 58, tm->tm_sec * W / 60 + 1, 3, 1);
}
static void drawStore(void) {
  gtext(0, 0, 1, "Store", 1);
  if (storeState == ST_LOAD) { gctext(28, 1, "Loading...", 1); return; }
  if (storeState == ST_BUSY) { gctext(28, 1, "Installing...", 1); return; }
  if (storeState == ST_FAIL) {
    gctext(14, 2, "Oops", 1);
    gctext(36, 1, storeMsg, 1);
    gctext(56, 1, "Hold: retry Tap: back", 1);
    return;
  }
  if (storeSel < storeTop) storeTop = storeSel;
  if (storeSel > storeTop + 3) storeTop = storeSel - 3;
  for (int r = 0; r < 4 && storeTop + r < itemN; r++) {
    int i = storeTop + r, y = 11 + r * 11;
    if (i == storeSel) fillrect(0, y - 1, W, 10, 1);
    gtext(2, y, 1, items[i].name, i != storeSel);
    const char *st = itemStatus(i);
    gtext(W - (int)strlen(st) * 6 - 2, y, 1, st, i != storeSel);
  }
  gctext(56, 1, "Hold: get/update", 1);
}
static void drawFrame(void) {
  memset(fb, 0, sizeof fb);
  if (app == A_HOME) drawHome();
  else if (app == A_CLOCK) drawClock();
  else if (app == A_STORE) drawStore();
  else if (app == A_SCRIPT) {
    scriptCall("draw();");
    if (scriptErr[0]) { fillrect(0, 56, W, 8, 0); gtext(0, 56, 1, scriptErr, 1); }
    if (scriptExit) goHome();
  }
}

// ---------- main loop ----------
#if HW
typedef struct { int pin, stable, cnt; unsigned downAt; int fired; } Btn;
static Btn bMain = { 0 }, bSecond = { 0 };
static int pollBtn(Btn *b, int isMain) {  // returns an event or -1
  if (b->pin < 0) return -1;
  int down = !gpio_read(b->pin);  // pulled up, pressed = low
  if (down == b->stable) { b->cnt = 0; } else if (++b->cnt >= 3) { b->stable = down; b->cnt = 0;
    unsigned t = ms_now();
    if (down) { b->downAt = t; b->fired = 0; if (!isMain) return EV_BACK; }
    else if (isMain && !b->fired) return (t - b->downAt >= 600) ? EV_HOLD : EV_TAP;
  }
  if (isMain && b->stable && !b->fired && ms_now() - b->downAt >= 1500) { b->fired = 1; return EV_HOME; }
  return -1;
}
int main(void) {
  load_config();
  gpio_open();
  oled_open();
  bMain.pin = cfg.pinMain; bSecond.pin = cfg.pinSecond;
  if (cfg.pinMain >= 0) { gpio_mode(cfg.pinMain, 0); gpio_pullup(cfg.pinMain); }
  if (cfg.pinSecond >= 0) { gpio_mode(cfg.pinSecond, 0); gpio_pullup(cfg.pinSecond); }
  if (cfg.pinBuzzer >= 0) { gpio_mode(cfg.pinBuzzer, 1); pthread_t th; pthread_create(&th, NULL, buzzer_thread, NULL); }
  if (cfg.mcpChannel >= 0) spifd = open("/dev/spidev0.0", O_RDWR);
  scanApps();
  unsigned next = 0;
  for (;;) {
    int e = pollBtn(&bMain, 1);
    if (e < 0) e = pollBtn(&bSecond, 0);
    if (e >= 0) onEvent(e);
    unsigned t = ms_now();
    if ((int)(t - next) >= 0) {
      drawFrame();
      oled_flush();
      runPending();
      next = ms_now() + 40;  // about 25 frames per second
    }
    usleep(5000);
  }
}
#else
// Simulator: reads one command per character on stdin.
//   t = tap   h = hold   H = hold 1.5 s (home)   b = second button   . = just a frame   p = print screen   q = quit
static void printScreen(void) {
  for (int y = 0; y < H; y += 2) {
    for (int x = 0; x < W; x++) {
      int a = (fb[y >> 3][x] >> (y & 7)) & 1, b = (fb[(y + 1) >> 3][x] >> ((y + 1) & 7)) & 1;
      fputs(a && b ? "\xE2\x96\x88" : a ? "\xE2\x96\x80" : b ? "\xE2\x96\x84" : " ", stdout);
    }
    putchar('\n');
  }
  printf("--\n");
}
int main(void) {
  load_config();
  scanApps();
  int c;
  drawFrame();
  while ((c = getchar()) != EOF && c != 'q') {
    if (c == 't') onEvent(EV_TAP);
    else if (c == 'h') onEvent(EV_HOLD);
    else if (c == 'H') onEvent(EV_HOME);
    else if (c == 'b') onEvent(EV_BACK);
    else if (c != '.' && c != 'p') continue;
    drawFrame();
    if (app == A_STORE) { runPending(); drawFrame(); }
    if (c == 'p') printScreen();
  }
  return 0;
}
#endif
