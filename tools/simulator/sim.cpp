// xonraOS desktop simulator (Windows): the runtime behind the Arduino/ESP8266 stand-ins, plus the window.
// The real firmware (firmware/xonraOS_TFT) is compiled against include/ and runs unchanged.
//
// Controls:  Space or left mouse = main button (tap / hold, just like the real one)
//            B or right mouse    = second button
//            Up/Down arrows or mouse wheel = scroll wheel (knob)
//            Esc = quit
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <filesystem>
#include <thread>
#include <algorithm>
#include "Arduino.h"
#include "EEPROM.h"
#include "LittleFS.h"
#include "ESP8266WiFi.h"
#include "ESP8266HTTPClient.h"
#include "ESP8266httpUpdate.h"
#include "Adafruit_ST7735.h"

namespace fs = std::filesystem;

void setup();
void loop();

// ---------------- globals the firmware expects ----------------
HardwareSerial Serial;
EspClass ESP;
ESP8266WiFiClass WiFi;
EEPROMClass EEPROM;
LittleFSClass LittleFS;
ESP8266HTTPUpdate ESPhttpUpdate;

static const int TW = 160, TH = 128, SCALE = 4, HELP_H = 30;
static uint16_t fb[TH][TW];
static uint32_t rgb[TH][TW];
static HWND win;
static volatile bool keyMain = false, keySecond = false, mouseMain = false, mouseSecond = false;
static int knob = 512;
static ULONGLONG t0;
static fs::path simDir, serverDir;

// ---------------- window ----------------
static void present() {
  if (!win) return;
  for (int y = 0; y < TH; y++)
    for (int x = 0; x < TW; x++) {
      uint16_t c = fb[y][x];
      uint8_t r = (c >> 11) & 31, g = (c >> 5) & 63, b = c & 31;
      rgb[y][x] = ((r << 3 | r >> 2) << 16) | ((g << 2 | g >> 4) << 8) | (b << 3 | b >> 2);
    }
  BITMAPINFO bi = {};
  bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
  bi.bmiHeader.biWidth = TW;
  bi.bmiHeader.biHeight = -TH;  // top-down
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  bi.bmiHeader.biCompression = BI_RGB;
  HDC dc = GetDC(win);
  SetStretchBltMode(dc, COLORONCOLOR);
  StretchDIBits(dc, 0, 0, TW * SCALE, TH * SCALE, 0, 0, TW, TH, rgb, &bi, DIB_RGB_COLORS, SRCCOPY);
  ReleaseDC(win, dc);
}

static void drawHelp(HDC dc) {
  RECT r = { 0, TH * SCALE, TW * SCALE, TH * SCALE + HELP_H };
  HBRUSH br = CreateSolidBrush(RGB(30, 30, 30));
  FillRect(dc, &r, br);
  DeleteObject(br);
  SetBkMode(dc, TRANSPARENT);
  SetTextColor(dc, RGB(200, 200, 200));
  char t[160];
  snprintf(t, sizeof t, "  Space/click: main   B/right-click: second   Up/Down/wheel: knob (%d)   Esc: quit", knob);
  DrawTextA(dc, t, -1, &r, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
}

static void setKnob(int v) {
  knob = std::max(0, std::min(1023, v));
  HDC dc = GetDC(win);
  drawHelp(dc);
  ReleaseDC(win, dc);
}

static LRESULT CALLBACK wndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  switch (m) {
    case WM_KEYDOWN:
      if (w == VK_SPACE) keyMain = true;
      else if (w == 'B' || w == VK_BACK) keySecond = true;
      else if (w == VK_UP || w == VK_RIGHT) setKnob(knob + 40);
      else if (w == VK_DOWN || w == VK_LEFT) setKnob(knob - 40);
      else if (w == VK_ESCAPE) PostQuitMessage(0);
      return 0;
    case WM_KEYUP:
      if (w == VK_SPACE) keyMain = false;
      else if (w == 'B' || w == VK_BACK) keySecond = false;
      return 0;
    case WM_LBUTTONDOWN: mouseMain = true; SetCapture(h); return 0;
    case WM_LBUTTONUP: mouseMain = false; ReleaseCapture(); return 0;
    case WM_RBUTTONDOWN: mouseSecond = true; return 0;
    case WM_RBUTTONUP: mouseSecond = false; return 0;
    case WM_MOUSEWHEEL: setKnob(knob + (GET_WHEEL_DELTA_WPARAM(w) > 0 ? 32 : -32)); return 0;
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC dc = BeginPaint(h, &ps);
      drawHelp(dc);
      EndPaint(h, &ps);
      present();
      return 0;
    }
    case WM_DESTROY: PostQuitMessage(0); return 0;
  }
  return DefWindowProcA(h, m, w, l);
}

static void pump() {
  MSG msg;
  while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT) { EEPROM.commit(); exit(0); }
    TranslateMessage(&msg);
    DispatchMessageA(&msg);
  }
}

static void openWindow() {
  WNDCLASSA wc = {};
  wc.lpfnWndProc = wndProc;
  wc.hInstance = GetModuleHandleA(nullptr);
  wc.lpszClassName = "xonraOSsim";
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  RegisterClassA(&wc);
  RECT r = { 0, 0, TW * SCALE, TH * SCALE + HELP_H };
  DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
  AdjustWindowRect(&r, style, FALSE);
  win = CreateWindowA("xonraOSsim", "xonraOS simulator - 1.8in TFT", style, CW_USEDEFAULT, CW_USEDEFAULT,
                      r.right - r.left, r.bottom - r.top, nullptr, nullptr, wc.hInstance, nullptr);
  ShowWindow(win, SW_SHOW);
  pump();
}

// ---------------- Arduino core ----------------
unsigned long millis() { return (unsigned long)(GetTickCount64() - t0); }
unsigned long micros() { return millis() * 1000UL; }
void delay(unsigned long ms) {
  unsigned long end = millis() + ms;
  while ((long)(end - millis()) > 0) { pump(); Sleep(1); }
}
void yield() { pump(); }
void pinMode(int, int) {}
void digitalWrite(int, int) {}
int digitalRead(int pin) {
  if (pin == D3) return (keyMain || mouseMain) ? LOW : HIGH;    // main button
  if (pin == D6) return (keySecond || mouseSecond) ? LOW : HIGH;  // second button
  return HIGH;
}
int analogRead(int) { return knob; }
void tone(int, unsigned freq, unsigned long ms) {
  if (freq < 37 || freq > 32767 || getenv("XONRA_SIM_MUTE")) return;
  if (!ms) ms = 150;
  std::thread([freq, ms] { Beep(freq, (DWORD)ms); }).detach();
}
void noTone(int) {}
long random(long howbig) { return howbig > 0 ? rand() % howbig : 0; }
long random(long lo, long hi) { return hi > lo ? lo + rand() % (hi - lo) : lo; }
void randomSeed(unsigned long s) { srand((unsigned)s); }
void configTime(const char *, const char *, const char *, const char *) {}  // uses the PC's clock and time zone
size_t strlcpy(char *dst, const char *src, size_t size) {
  size_t n = strlen(src);
  if (size) { size_t k = n < size - 1 ? n : size - 1; memcpy(dst, src, k); dst[k] = 0; }
  return n;
}
struct tm *sim_localtime_r(const time_t *t, struct tm *out) { return localtime_s(out, t) == 0 ? out : nullptr; }
uint32_t EspClass::getFreeHeap() { return 30500; }
void EspClass::restart() { exit(0); }

// ---------------- EEPROM ----------------
void EEPROMClass::begin(size_t n) {
  size = n;
  memset(data, 0xFF, sizeof data);
  if (FILE *f = fopen((simDir / "eeprom.bin").string().c_str(), "rb")) { fread(data, 1, sizeof data, f); fclose(f); }
}
bool EEPROMClass::commit() {
  if (FILE *f = fopen((simDir / "eeprom.bin").string().c_str(), "wb")) { fwrite(data, 1, sizeof data, f); fclose(f); return true; }
  return false;
}

// ---------------- LittleFS (simfs folder) ----------------
static fs::path fsPath(const String &p) {
  std::string s = p.s;
  while (!s.empty() && s[0] == '/') s.erase(0, 1);
  return simDir / "flash" / fs::path(s);
}
size_t File::size() {
  if (!fp) return 0;
  long cur = ftell(fp); fseek(fp, 0, SEEK_END); long n = ftell(fp); fseek(fp, cur, SEEK_SET);
  return (size_t)n;
}
bool LittleFSClass::begin() { std::error_code e; fs::create_directories(simDir / "flash", e); return true; }
bool LittleFSClass::format() { std::error_code e; fs::remove_all(simDir / "flash", e); fs::create_directories(simDir / "flash", e); return true; }
bool LittleFSClass::mkdir(const String &p) { std::error_code e; fs::create_directories(fsPath(p), e); return true; }
bool LittleFSClass::exists(const String &p) { return fs::exists(fsPath(p)); }
File LittleFSClass::open(const String &p, const char *mode) {
  std::string m = mode && mode[0] == 'w' ? "wb" : (mode && mode[0] == 'a' ? "ab" : "rb");
  return File(fopen(fsPath(p).string().c_str(), m.c_str()));
}
bool LittleFSClass::remove(const String &p) { std::error_code e; return fs::remove(fsPath(p), e); }
bool LittleFSClass::rename(const String &a, const String &b) {
  std::error_code e;
  fs::remove(fsPath(b), e);
  fs::rename(fsPath(a), fsPath(b), e);
  return !e;
}
Dir LittleFSClass::openDir(const String &p) {
  Dir d;
  d.dir = fsPath(p).string();
  std::error_code e;
  for (auto &it : fs::directory_iterator(fsPath(p), e))
    if (it.is_regular_file()) d.names.push_back(it.path().filename().string());
  std::sort(d.names.begin(), d.names.end());
  return d;
}
bool LittleFSClass::info(FSInfo &i) {
  i.totalBytes = 2 * 1024 * 1024 - 16384;
  i.usedBytes = 8192;
  std::error_code e;
  for (auto &it : fs::recursive_directory_iterator(simDir / "flash", e))
    if (it.is_regular_file()) i.usedBytes += (it.file_size() + 4095) / 4096 * 4096;
  return true;
}
bool Dir::next() { return ++idx < (int)names.size(); }
String Dir::fileName() { return idx >= 0 && idx < (int)names.size() ? String(names[idx]) : String(); }
File Dir::openFile(const char *mode) {
  if (idx < 0 || idx >= (int)names.size()) return File();
  return File(fopen((fs::path(dir) / names[idx]).string().c_str(), mode && mode[0] == 'w' ? "wb" : "rb"));
}

// ---------------- HTTP: read files from the repository's server/ folder ----------------
int HTTPClient::GET() {
  std::string u = url.s;
  size_t p = u.find("://");
  p = u.find('/', p == std::string::npos ? 0 : p + 3);
  std::string path = p == std::string::npos ? "/" : u.substr(p);
  fs::path f = serverDir / fs::path(path.substr(1));
  body = String();
  FILE *fp = fopen(f.string().c_str(), "rb");
  if (!fp) { code = 404; return code; }
  std::string s;
  char buf[4096];
  size_t n;
  while ((n = fread(buf, 1, sizeof buf, fp)) > 0) s.append(buf, n);
  fclose(fp);
  body = String(s);
  code = 200;
  return code;
}

// ---------------- TFT ----------------
void Adafruit_ST7735::initR(uint8_t) { fillScreen(0); }
void Adafruit_ST7735::setRotation(uint8_t) {}
void Adafruit_ST7735::fillScreen(uint16_t c) {
  for (auto &row : fb) for (auto &px : row) px = c;
  present();
}
void Adafruit_ST7735::setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) { ax = x; ay = y; aw = w; ah = h; cur = 0; }
void Adafruit_ST7735::writePixels(uint16_t *c, uint32_t len, bool, bool) {
  for (uint32_t i = 0; i < len; i++, cur++) {
    int x = ax + cur % aw, y = ay + cur / aw;
    if (x >= 0 && x < TW && y >= 0 && y < TH) fb[y][x] = c[i];
  }
}
void Adafruit_ST7735::endWrite() { present(); pump(); }

// ---------------- script mode (for automatic tests) ----------------
// XONRA_SIM_SCRIPT="w3000 t w500 h s:home.bmp q"
//   wN wait N ms, t tap main, h hold main 0.8 s, H hold main 1.7 s (Home), b press second button,
//   kN set knob to N, s:FILE save a screenshot (BMP, 3x), q quit
static std::vector<std::string> script;
static size_t scriptPos = 0;
static unsigned long scriptNext = 0, releaseAt = 0;
static bool scriptPressMain = false, scriptPressSecond = false;

static void saveBmp(const std::string &name) {
  const int S = 3, W = TW * S, H = TH * S, row = W * 3;
  std::vector<uint8_t> px(row * H);
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++) {
      uint32_t c = rgb[y / S][x / S];
      uint8_t *p = &px[(H - 1 - y) * row + x * 3];
      p[0] = c & 255; p[1] = (c >> 8) & 255; p[2] = (c >> 16) & 255;
    }
  BITMAPFILEHEADER fh = {}; BITMAPINFOHEADER ih = {};
  fh.bfType = 0x4D42; fh.bfOffBits = sizeof fh + sizeof ih; fh.bfSize = fh.bfOffBits + (DWORD)px.size();
  ih.biSize = sizeof ih; ih.biWidth = W; ih.biHeight = H; ih.biPlanes = 1; ih.biBitCount = 24;
  if (FILE *f = fopen(name.c_str(), "wb")) { fwrite(&fh, sizeof fh, 1, f); fwrite(&ih, sizeof ih, 1, f); fwrite(px.data(), 1, px.size(), f); fclose(f); }
  printf("screenshot %s\n", name.c_str());
}

static void stepScript() {
  if (releaseAt && millis() >= releaseAt) { keyMain = keySecond = false; releaseAt = 0; scriptNext = millis() + 120; }
  if (releaseAt || scriptPos >= script.size() || millis() < scriptNext) return;
  const std::string &c = script[scriptPos++];
  if (c[0] == 'w') scriptNext = millis() + atol(c.c_str() + 1);
  else if (c == "t") { keyMain = true; releaseAt = millis() + 120; }
  else if (c == "h") { keyMain = true; releaseAt = millis() + 800; }
  else if (c == "H") { keyMain = true; releaseAt = millis() + 1700; }
  else if (c == "b") { keySecond = true; releaseAt = millis() + 120; }
  else if (c[0] == 'k') setKnob(atoi(c.c_str() + 1));
  else if (c.rfind("s:", 0) == 0) saveBmp(c.substr(2));
  else if (c == "q") { EEPROM.commit(); exit(0); }
}

// ---------------- main ----------------
static LONG WINAPI onCrash(EXCEPTION_POINTERS *e) {
  void *frames[32];
  USHORT n = CaptureStackBackTrace(0, 32, frames, nullptr);
  fprintf(stderr, "\nCRASH code 0x%lx at %p (exe base %p)\n", e->ExceptionRecord->ExceptionCode,
          e->ExceptionRecord->ExceptionAddress, (void *)GetModuleHandleA(nullptr));
  fprintf(stderr, "  rip %p\n", (void *)e->ContextRecord->Rip);
  for (USHORT i = 0; i < n; i++) fprintf(stderr, "  frame %p\n", frames[i]);
  fflush(stderr);
  return EXCEPTION_EXECUTE_HANDLER;
}

int main() {
  setvbuf(stdout, nullptr, _IONBF, 0);
  SetUnhandledExceptionFilter(onCrash);
  t0 = GetTickCount64();
  char exe[MAX_PATH];
  GetModuleFileNameA(nullptr, exe, MAX_PATH);
  simDir = fs::path(exe).parent_path() / "simfs";
  std::error_code e;
  fs::create_directories(simDir, e);
  const char *srv = getenv("XONRA_SIM_SERVER");
  serverDir = srv ? fs::path(srv) : fs::path(SIM_SERVER_DIR);
  srand((unsigned)time(nullptr));
  printf("xonraOS simulator\n  flash + settings: %s\n  app store files:  %s\n", simDir.string().c_str(), serverDir.string().c_str());
  if (const char *sc = getenv("XONRA_SIM_SCRIPT")) {
    std::string s(sc), tok;
    for (char ch : s + " ") { if (ch == ' ') { if (!tok.empty()) script.push_back(tok); tok.clear(); } else tok += ch; }
  }
  openWindow();
  setup();
  for (;;) {
    pump();
    stepScript();
    loop();
    Sleep(1);
  }
}
