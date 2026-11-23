// xonraOS: a phone-style OS on an ESP8266 (NodeMCU) + 1.8" ST7735 colour TFT (160x128) + 2 buttons.
// This is the TFT version. The 0.96" OLED version is firmware/xonraOS.
//
// Wiring (TFT module pin -> NodeMCU):
//   VCC -> 3V3, GND -> GND, LED (backlight) -> 3V3
//   SCK/SCL -> D5, SDA/MOSI -> D7, CS -> D1, A0/DC -> D2, RESET -> RST
//   Button main: one leg -> D3, diagonally opposite leg -> GND   (don't hold it while powering on)
//   Button second: one leg -> D6, diagonally opposite leg -> GND
//   Buzzer + -> D8, - -> GND.   Knob: ends to 3V3 and GND, middle -> A0.
//
// Home screen apps: Clock, Jump, Snake, Store (empty for now), Settings.
// Controls (main button D3):
//   Tap  = next / "do it" in games.   Hold 0.6 s (acts on release) = select / open.
//   Hold 1.5 s (a line fills along the top) = back to Home.
// Second button D6 (Settings > D6 button): Off, Home (always goes Home) or Back (previous / left / jump).
//
// Apps are loaded only while open: their memory (Snake's board, the phone-setup web server,
// the Wi-Fi scan list) is allocated when you open them and freed when you leave.
// Wi-Fi setup lives inside Settings. Wi-Fi name and password fall back to secrets.h.

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <time.h>
#include <math.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <LittleFS.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <stddef.h>
extern "C" {
#include "elk.h"  // tiny JavaScript engine used for Store apps
}
#include "secrets.h"

// Firmware build number. Raise it for every release you publish (see tools/release-firmware.js).
const int FW_VERSION = 9;
const char* FW_VERSION_LABEL = "V2.0.7";

// STORE_URL (the app server that serves manifest.txt and apps/<id>.js) comes from secrets.h.

// Saudi Arabia, UTC+3, no summer time. (UAE/Oman would be "<+04>-4".)
const char* TIME_ZONE = "<+03>-3";

const int SCREEN_WIDTH = 160;
const int SCREEN_HEIGHT = 128;
const int ROW_BYTES = (SCREEN_WIDTH + 7) / 8;           // the frame is 1 bit per pixel, row by row
const int FRAME_BYTES = ROW_BYTES * SCREEN_HEIGHT;      // 2560 bytes
const int BUTTON_PIN = D3;  // main button (GPIO0: don't hold it while powering on)
const int HOME_BTN = D6;    // second button (see Settings > D6 button)

// TFT on hardware SPI (SCK D5, MOSI D7). Change TFT_TAB if colours or edges look wrong
// (INITR_BLACKTAB, INITR_GREENTAB or INITR_REDTAB, printed on the screen's protective film tab).
#define TFT_CS D1
#define TFT_DC D2
#define TFT_RST -1  // TFT reset is wired to the board's RST pin
#define TFT_TAB INITR_BLACKTAB
#define TFT_ROTATION 1  // landscape; use 3 to turn it upside down
const int BUZZER_PIN = D8;  // buzzer: + to D8, - to GND (D8 must be low at boot, which a buzzer to GND is)

const unsigned long DEBOUNCE_MS = 30;
const unsigned long LONG_PRESS_MS = 600;
const unsigned long HOME_PRESS_MS = 1500;  // hold this long to go back to Home
const unsigned long FRAME_MS = 40;         // 25 frames per second
const unsigned long TRANS_MS = 280;        // slide transition length

// Drawing still uses two "colours", on and off, like the OLED did; display() paints them on the TFT.
#define SSD1306_WHITE 1
#define SSD1306_BLACK 0
Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
uint16_t fgColor = ST77XX_WHITE;  // set by Settings > Brightness
const uint16_t BG_COLOR = ST77XX_BLACK;

class Screen : public GFXcanvas1 {
 public:
  Screen() : GFXcanvas1(SCREEN_WIDTH, SCREEN_HEIGHT) {}
  void clearDisplay() { fillScreen(0); }
  // Send the whole frame in one SPI burst, a row at a time (per-pixel drawing would be far too slow)
  void display() {
    static uint16_t line[SCREEN_WIDTH];
    const uint8_t* buf = getBuffer();
    tft.startWrite();
    tft.setAddrWindow(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
      const uint8_t* row = buf + y * ROW_BYTES;
      for (int x = 0; x < SCREEN_WIDTH; x++) line[x] = (row[x >> 3] & (0x80 >> (x & 7))) ? fgColor : BG_COLOR;
      tft.writePixels(line, SCREEN_WIDTH);
    }
    tft.endWrite();
  }
};
Screen display;

// ---------- Saved settings ----------
struct Prefs {
  uint32_t magic;
  uint8_t use24, bright, secs, anim, ctrl, sound;  // ctrl: D6 button 0 off, 1 Home, 2 Back
};
const uint32_t PREFS_MAGIC = 0xC10C0006;
const char* CTRL_NAMES[3] = { "Off", "Home", "Back" };
Prefs prefs;

// The backlight is always on, so Brightness sets how bright the text and drawings are.
const uint16_t BRIGHT_LEVELS[3] = { 0x6B4D, 0xB5B6, 0xFFFF };  // grey, light grey, white
const char* BRIGHT_NAMES[3] = { "Low", "Med", "High" };

struct HiScore { uint16_t magic; uint16_t best; };
const int HI_ADDR = 32;   // Jump best score
const int HI2_ADDR = 40;  // Snake best score
const uint16_t HI_MAGIC = 0xB357;
uint16_t gBest = 0, snBest = 0;

void savePrefs() {
  EEPROM.put(0, prefs);
  EEPROM.commit();
}

void saveBest(int addr, uint16_t v) {
  HiScore h = { HI_MAGIC, v };
  EEPROM.put(addr, h);
  EEPROM.commit();
}

// Wi-Fi network chosen in Settings (falls back to secrets.h when nothing is saved)
struct Creds { uint32_t magic; char ssid[33]; char pass[65]; };
const int NET_ADDR = 64;
const uint32_t NET_MAGIC = 0x57F1C0DE;
char curSsid[33];
char curPass[65];

void useDefaultCreds() {
  strlcpy(curSsid, WIFI_NAME, sizeof(curSsid));
  strlcpy(curPass, WIFI_PASSWORD, sizeof(curPass));
}

void loadCreds() {
  Creds c;
  EEPROM.get(NET_ADDR, c);
  if (c.magic == NET_MAGIC && c.ssid[0]) {
    c.ssid[32] = 0;
    c.pass[64] = 0;
    strlcpy(curSsid, c.ssid, sizeof(curSsid));
    strlcpy(curPass, c.pass, sizeof(curPass));
  } else {
    useDefaultCreds();
  }
}

void saveCreds(bool clearIt) {
  Creds c;
  memset(&c, 0, sizeof(c));
  if (!clearIt) {
    c.magic = NET_MAGIC;
    strlcpy(c.ssid, curSsid, sizeof(c.ssid));
    strlcpy(c.pass, curPass, sizeof(c.pass));
  }
  EEPROM.put(NET_ADDR, c);
  EEPROM.commit();
}

void loadPrefs() {
  EEPROM.begin(256);
  EEPROM.get(0, prefs);
  if (prefs.magic != PREFS_MAGIC) {
    prefs = { PREFS_MAGIC, 0, 2, 1, 1, 2, 1 };  // 12h, high, seconds on, animations on, D6 = Back, sound on
    savePrefs();
  }
  HiScore h;
  EEPROM.get(HI_ADDR, h);
  if (h.magic == HI_MAGIC) gBest = h.best;
  EEPROM.get(HI2_ADDR, h);
  if (h.magic == HI_MAGIC) snBest = h.best;
}

void applyBrightness() {
  fgColor = BRIGHT_LEVELS[prefs.bright % 3];
}

// ---------- State ----------
enum App { APP_HOME, APP_CLOCK, APP_GAME, APP_SNAKE, APP_STORE, APP_SETTINGS, APP_SCRIPT };
int app = APP_HOME;  // boots to the app selection screen

// Apps installed from the Store (scripts in flash, /apps/<id>.js). First line: //Name|version|Short
const int MAX_INST = 8;
char instId[MAX_INST][14];
char instName[MAX_INST][12];
char instVer[MAX_INST][8];
char instShort[MAX_INST][6];
int instN = 0;
int scriptIdx = 0;  // which installed app is open

const int HOME_BUILTIN = 5;           // Clock, Jump, Snake, Store, Settings
#define HOME_N (HOME_BUILTIN + instN)  // apps on Home (4 tiles per page)
int homeSel = 0;                       // tile on Home (built-ins: app = homeSel + 1)

// Settings app
const int SET_N = 9;  // Includes Storage/Memory after Firmware
int setCursor = 0;
float setScroll = 0, setBar = 0;  // animated list scroll and highlight positions in pixels
bool inNet = false;   // inside Settings > Wi-Fi setup
bool inFw = false;    // inside Settings > Firmware
bool inMemory = false;  // inside Settings > Storage/Memory

// Wi-Fi setup state (the web server and scan list only exist while the portal is open)
enum NetState { N_MENU, N_SCAN, N_PORTAL, N_TRYING, N_OK };
int netState = N_MENU;
int netCursor = 0;
unsigned long netT = 0;
bool portalUp = false;
String netMsg;
char pendSsid[33], pendPass[65];
String* scanName = nullptr;
int* scanPct = nullptr;
int scanN = 0;
ESP8266WebServer* web = nullptr;
DNSServer* dnsSrv = nullptr;
const char* AP_NAME = "xonraOS-Setup";
const char* AP_PASS = "esp12345";

// Snake (32 x 23 grid of 5-pixel cells under the score line, edges wrap around). Buffers exist only while the app is open.
const int SN_COLS = 32, SN_ROWS = 23, SN_MAX = SN_COLS * SN_ROWS, SN_CELL = 5, SN_TOP = 12;
uint8_t* snX = nullptr;
uint8_t* snY = nullptr;
int snLen = 3, snDir = 0, snNext = 0, snScore = 0, snState = 0;
int foodX = 0, foodY = 0;
unsigned long snLast = 0, snOverAt = 0;

// Jump game
enum GameState { G_IDLE, G_RUN, G_OVER };
int gState = G_IDLE;
const int GROUND_Y = 116;
float gH = 0, gV = 0;  // player height above ground and vertical speed
float gX[2];
int gW[2], gHt[2];
bool gAct[2], gScored[2];
int gScore = 0, gGap = 0;
unsigned long gFrame = 0, gOverAt = 0;

// Button
bool pressed = false, lastReading = false, longDone = false;
unsigned long lastChange = 0, pressStart = 0;

// Animation
uint8_t lastFrame[FRAME_BYTES];
uint8_t newFrame[FRAME_BYTES];
bool transActive = false;
unsigned long transStart = 0;
int lastSec = -1;
unsigned long secStartMs = 0;
unsigned long lastDraw = 0;

const char* DAYS3[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
const char* MONTHS[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

bool timeIsSet() { return time(nullptr) > 1700000000; }  // after Nov 2023

// Sound effect on the buzzer (non-blocking), respects Settings > Sound
void sfx(int freq, int ms) {
  if (prefs.sound) tone(BUZZER_PIN, freq, ms);
}

// ---------- Drawing helpers ----------
// Smooth scrolling: move a value toward its target a bit each frame (snaps when animations are off)
float smoothTo(float cur, float target) {
  if (!prefs.anim) return target;
  cur += (target - cur) * 0.4f;
  if (fabsf(target - cur) < 0.4f) cur = target;
  return cur;
}

void centerText(const char* s, int y, int size) {
  display.setTextSize(size);
  int w = strlen(s) * 6 * size;
  display.setCursor((SCREEN_WIDTH - w) / 2, y);
  display.print(s);
}

// Title bar along the top (inverted), with optional text on the right
const int TITLE_H = 12;
void drawTitle(const char* left, const char* right = nullptr) {
  display.fillRect(0, 0, SCREEN_WIDTH, TITLE_H, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(4, 2);
  display.print(left);
  if (right) {
    display.setCursor(SCREEN_WIDTH - 4 - strlen(right) * 6, 2);
    display.print(right);
  }
  display.setTextColor(SSD1306_WHITE);
}

// Hint line along the bottom (clears what scrolled under it)
const int HINT_Y = SCREEN_HEIGHT - 10;
void drawHint(const char* s) {
  display.fillRect(0, HINT_Y - 2, SCREEN_WIDTH, SCREEN_HEIGHT - HINT_Y + 2, SSD1306_BLACK);
  display.drawFastHLine(0, HINT_Y - 2, SCREEN_WIDTH, SSD1306_WHITE);
  centerText(s, HINT_Y + 1, 1);
}

void drawWifi(int x, int y) {
  if (WiFi.status() != WL_CONNECTED) {
    display.drawLine(x, y, x + 6, y + 7, SSD1306_WHITE);
    display.drawLine(x + 6, y, x, y + 7, SSD1306_WHITE);
    return;
  }
  long r = WiFi.RSSI();
  int lvl = r > -55 ? 4 : r > -65 ? 3 : r > -75 ? 2 : 1;
  for (int i = 0; i < 4; i++) {
    int h = 2 + i * 2;
    if (i < lvl) display.fillRect(x + i * 3, y + 8 - h, 2, h, SSD1306_WHITE);
    else display.drawPixel(x + i * 3, y + 7, SSD1306_WHITE);
  }
}

// ---------- Clock app: just a clock ----------
void drawConnecting() {
  display.setTextSize(1);
  int active = (millis() / 100) % 8;
  for (int i = 0; i < 8; i++) {
    float a = i * 0.7854f;
    int x = SCREEN_WIDTH / 2 + (int)(20 * sinf(a));
    int y = 40 - (int)(20 * cosf(a));
    if (i == active) display.fillCircle(x, y, 3, SSD1306_WHITE);
    else if (i == (active + 7) % 8) display.fillCircle(x, y, 2, SSD1306_WHITE);
    else display.fillCircle(x, y, 1, SSD1306_WHITE);
  }
  if (WiFi.status() != WL_CONNECTED) {
    centerText("Connecting to", 78, 1);
    centerText(curSsid, 94, 1);
  } else {
    centerText("Connected!", 78, 2);
    centerText("Getting the time...", 100, 1);
  }
}

void drawClock(const tm& t) {
  drawWifi(4, 4);

  int hour = t.tm_hour;
  const char* ampm = "";
  if (!prefs.use24) {
    ampm = hour < 12 ? "AM" : "PM";
    hour = hour % 12;
    if (hour == 0) hour = 12;
  }
  char hm[8];
  if (prefs.use24) snprintf(hm, sizeof(hm), "%02d:%02d", hour, t.tm_min);
  else snprintf(hm, sizeof(hm), "%d:%02d", hour, t.tm_min);

  // Blinking colon (half a second on, half off) when animations are on
  unsigned long frac = millis() - secStartMs;
  if (prefs.anim && frac >= 500) {
    char* c = strchr(hm, ':');
    if (c) *c = ' ';
  }

  // Big time (text size 4 = 24 x 32 px per character); seconds and AM/PM sit to its right
  const int TY = 34;
  int w = strlen(hm) * 24;
  int side = (prefs.secs || !prefs.use24) ? 28 : 0;  // room for seconds / AM-PM (size 2 = 12 px wide)
  int x = (SCREEN_WIDTH - w - side) / 2;
  display.setTextSize(4);
  display.setCursor(x, TY);
  display.print(hm);
  display.setTextSize(2);
  if (prefs.secs) {
    char ss[3];
    snprintf(ss, sizeof(ss), "%02d", t.tm_sec);
    display.setCursor(x + w + 4, TY);
    display.print(ss);
  }
  if (!prefs.use24) {
    display.setCursor(x + w + 4, TY + 16);
    display.print(ampm);
  }

  // Date line and a smooth seconds bar
  char dl[20];
  snprintf(dl, sizeof(dl), "%s %d %s", DAYS3[t.tm_wday], t.tm_mday, MONTHS[t.tm_mon]);
  centerText(dl, 84, 2);
  if (frac > 999) frac = 999;
  int bar = (int)((t.tm_sec * 1000UL + (prefs.anim ? frac : 0)) * SCREEN_WIDTH / 60000UL);
  display.drawFastHLine(0, SCREEN_HEIGHT - 3, SCREEN_WIDTH, SSD1306_WHITE);
  display.fillRect(0, SCREEN_HEIGHT - 5, bar, 5, SSD1306_WHITE);
}

// ---------- Installed apps (scripts stored in flash) ----------
void parseHeader(const String& line, int slot) {  // "//Name|ver|Short"
  String s = line;
  if (s.startsWith("//")) s = s.substring(2);
  s.trim();
  int a = s.indexOf('|');
  String name = a < 0 ? s : s.substring(0, a);
  String rest = a < 0 ? "" : s.substring(a + 1);
  int b = rest.indexOf('|');
  String ver = b < 0 ? rest : rest.substring(0, b);
  String shortName = b < 0 ? "" : rest.substring(b + 1);
  if (shortName.length() == 0) shortName = name;
  strlcpy(instName[slot], name.c_str(), sizeof(instName[slot]));
  strlcpy(instVer[slot], ver.c_str(), sizeof(instVer[slot]));
  strlcpy(instShort[slot], shortName.c_str(), sizeof(instShort[slot]));
}

void refreshInstalled() {
  instN = 0;
  Dir d = LittleFS.openDir("/apps");
  while (d.next() && instN < MAX_INST) {
    String fn = d.fileName();
    if (!fn.endsWith(".js")) continue;
    File f = d.openFile("r");
    if (!f) continue;
    String first = f.readStringUntil('\n');
    f.close();
    strlcpy(instId[instN], fn.substring(0, fn.length() - 3).c_str(), sizeof(instId[instN]));
    parseHeader(first, instN);
    instN++;
  }
  if (homeSel >= HOME_N) homeSel = HOME_N - 1;
}

int findInstalled(const char* id) {
  for (int i = 0; i < instN; i++)
    if (strcmp(instId[i], id) == 0) return i;
  return -1;
}

bool httpGetText(const String& url, String& out, size_t maxLen) {
  if (WiFi.status() != WL_CONNECTED) return false;
  WiFiClient c;
  HTTPClient h;
  h.setTimeout(5000);
  h.useHTTP10(true);
  if (!h.begin(c, url)) return false;
  int code = h.GET();
  bool ok = false;
  if (code == 200) {
    int len = h.getSize();
    if (len < 0 || (size_t)len <= maxLen) {
      out = h.getString();
      ok = out.length() <= maxLen;
    }
  }
  h.end();
  return ok;
}

// ---------- Store app (downloads apps from your app server) ----------
void goHome();
enum StoreState { ST_FETCH, ST_LIST, ST_BUSY, ST_ERR };
enum StoreLevel { SL_CAT, SL_APP, SL_DETAIL };
const int ST_MAX = 24;
const int ST_CAT_N = 5;
const char* ST_CATS[ST_CAT_N] = { "Tools", "Games", "Timers", "Sound", "Other" };
int stState = ST_FETCH, stLevel = SL_CAT, stSel = 0, stCategory = 0, stCur = 0, stN = 0;
unsigned long stReq = 0, stMsgAt = 0;
char stId[ST_MAX][14], stName[ST_MAX][12], stVer[ST_MAX][8];
uint8_t stCat[ST_MAX];
String stMsg, stDesc;
bool stDescPending = false;

int storeCatIndex(String name) {
  name.trim();
  for (int i = 0; i < ST_CAT_N - 1; i++) if (name.equalsIgnoreCase(ST_CATS[i])) return i;
  return ST_CAT_N - 1;
}

int storeCount(int cat) {
  int n = 0;
  for (int i = 0; i < stN; i++) if (stCat[i] == cat) n++;
  return n;
}

int storeCategoryAt(int pos) {
  for (int c = 0; c < ST_CAT_N; c++) if (storeCount(c) && pos-- == 0) return c;
  return -1;
}

int storeAppAt(int pos) {
  for (int i = 0; i < stN; i++) if (stCat[i] == stCategory && pos-- == 0) return i;
  return -1;
}

int storeChoices() {
  if (stLevel == SL_DETAIL) return 0;
  if (stLevel == SL_APP) return storeCount(stCategory);
  int n = 0;
  for (int c = 0; c < ST_CAT_N; c++) if (storeCount(c)) n++;
  return n;
}

void storeOpen() {
  stState = ST_FETCH;
  stLevel = SL_CAT;
  stSel = stCur = stCategory = stN = 0;
  stReq = millis();
  stMsg = stDesc = "";
  stDescPending = false;
}

void storeFetch() {
  String body;
  if (!httpGetText(String(STORE_URL) + "/manifest.txt", body, 2000)) { stState = ST_ERR; return; }
  stN = 0;
  int pos = 0;
  while (pos < (int)body.length() && stN < ST_MAX) {
    int e = body.indexOf('\n', pos);
    if (e < 0) e = body.length();
    String row = body.substring(pos, e);
    pos = e + 1;
    row.trim();
    int a = row.indexOf('|'), b = row.indexOf('|', a + 1);
    if (a <= 0 || b < 0) continue;
    int c = row.indexOf('|', b + 1);
    strlcpy(stId[stN], row.substring(0, a).c_str(), sizeof(stId[stN]));
    strlcpy(stName[stN], row.substring(a + 1, b).c_str(), sizeof(stName[stN]));
    strlcpy(stVer[stN], row.substring(b + 1, c < 0 ? row.length() : c).c_str(), sizeof(stVer[stN]));
    stCat[stN] = c < 0 ? ST_CAT_N - 1 : storeCatIndex(row.substring(c + 1));
    stN++;
  }
  stSel = 0;
  stLevel = SL_CAT;
  stState = ST_LIST;
}

void storeNext() {
  if (stState == ST_ERR) { storeOpen(); return; }
  int n = storeChoices();
  if (stState == ST_LIST && n) stSel = (stSel + 1) % n;
}

void storeSelect() {
  if (stState == ST_ERR) { storeOpen(); return; }
  if (stState != ST_LIST || !stN) return;
  stMsg = "";
  if (stLevel == SL_CAT) {
    stCategory = storeCategoryAt(stSel);
    if (stCategory < 0) return;
    stSel = 0;
    stLevel = SL_APP;
  } else if (stLevel == SL_APP) {
    stCur = storeAppAt(stSel);
    if (stCur < 0) return;
    stLevel = SL_DETAIL;
    stDesc = "Loading...";
    stDescPending = true;
    stReq = millis();
  } else {
    stState = ST_BUSY;
    stReq = millis();
  }
}

bool storeBack() {
  if (stState != ST_LIST || stLevel == SL_CAT) return false;
  stMsg = "";
  stDescPending = false;
  if (stLevel == SL_DETAIL) {
    stLevel = SL_APP;  // stSel still identifies the app we opened
    stDesc = "";
  } else {
    stLevel = SL_CAT;
    stSel = 0;
    for (int c = 0; c < stCategory; c++) if (storeCount(c)) stSel++;
  }
  return true;
}

void storePrevious() {
  if (stState == ST_ERR) { storeOpen(); return; }
  if (stState != ST_LIST) return;
  if (stLevel == SL_DETAIL) { storeBack(); return; }
  int n = storeChoices();
  if (n) stSel = (stSel + n - 1) % n;
}

void storeFetchDesc() {
  stDescPending = false;
  stDesc = "";
  String body;
  if (httpGetText(String(STORE_URL) + "/apps/" + stId[stCur] + ".js", body, 9000)) {
    int pos = body.indexOf('\n') + 1;
    while (pos > 0 && pos < (int)body.length() && stDesc.length() < 48) {
      int e = body.indexOf('\n', pos);
      if (e < 0) e = body.length();
      String row = body.substring(pos, e);
      pos = e + 1;
      row.trim();
      if (!row.startsWith("//")) break;
      row = row.substring(2);
      row.trim();
      if (!row.length()) break;
      if (stDesc.length()) stDesc += " ";
      stDesc += row;
    }
  }
  if (!stDesc.length()) stDesc = "No description";
  if (stDesc.length() > 48) stDesc = stDesc.substring(0, 45) + "...";
}

void storeAct() {
  int idx = findInstalled(stId[stCur]);
  String path = String("/apps/") + stId[stCur] + ".js";
  if (idx >= 0 && strcmp(instVer[idx], stVer[stCur]) == 0) {
    if (LittleFS.remove(path)) {
      LittleFS.remove(String("/d_") + stId[stCur] + ".txt");
      refreshInstalled();
      stMsg = "Removed";
    } else stMsg = "Remove failed";
  } else if (idx < 0 && instN >= MAX_INST) {
    stMsg = "8 apps: remove one";
  } else {
    String body;
    if (!httpGetText(String(STORE_URL) + "/apps/" + stId[stCur] + ".js", body, 9000) || !body.startsWith("//")) {
      stMsg = "Download failed";
    } else {
      // Stage the download so a failed write does not destroy an installed app.
      File f = LittleFS.open("/store.tmp", "w");
      if (!f) stMsg = "Flash full?";
      else {
        size_t written = f.print(body);
        f.close();
        if (written == body.length() && LittleFS.rename("/store.tmp", path)) {
          refreshInstalled();
          stMsg = "Installed";
        } else {
          LittleFS.remove("/store.tmp");
          stMsg = "Install failed";
        }
      }
    }
  }
  stMsgAt = millis();
  stState = ST_LIST;
}

void updateStore() {
  if (app != APP_STORE || transActive || millis() - stReq < 150) return;
  if (stState == ST_FETCH) storeFetch();
  else if (stState == ST_BUSY) storeAct();
  else if (stLevel == SL_DETAIL && stDescPending) storeFetchDesc();
}

void storeIcon(int x, int y, int cat, uint16_t ink) {
  if (cat == 1) {  // gamepad
    display.drawRoundRect(x, y + 2, 11, 7, 2, ink);
    display.drawLine(x + 2, y + 5, x + 4, y + 5, ink);
    display.drawLine(x + 3, y + 4, x + 3, y + 6, ink);
    display.drawPixel(x + 7, y + 4, ink); display.drawPixel(x + 8, y + 6, ink);
  } else if (cat == 0) {  // wrench
    display.drawCircle(x + 7, y + 3, 3, ink);
    display.drawLine(x + 5, y + 5, x + 1, y + 9, ink);
    display.drawLine(x + 6, y + 6, x + 2, y + 10, ink);
  } else if (cat == 2) {
    display.drawCircle(x + 5, y + 6, 4, ink);
    display.drawLine(x + 5, y + 6, x + 5, y + 3, ink);
    display.drawLine(x + 3, y, x + 7, y, ink);
  } else if (cat == 3) {
    display.drawLine(x + 7, y + 1, x + 7, y + 8, ink);
    display.drawLine(x + 7, y + 1, x + 10, y + 3, ink);
    display.fillCircle(x + 5, y + 8, 2, ink);
  } else {
    for (int i = 0; i < 4; i++) display.drawRect(x + (i % 2) * 6, y + (i / 2) * 6, 4, 4, ink);
  }
}

void storeText(const char* text, int x, int y, int limit) {
  display.setCursor(x, y);
  int n = strlen(text);
  for (int i = 0; i < n && i < limit; i++) display.print(i == limit - 1 && n > limit ? '.' : text[i]);
}

void drawStore() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  String crumb = "Store";
  if (stLevel != SL_CAT) crumb += String(" > ") + ST_CATS[stCategory];
  drawTitle(crumb.c_str());
  if (stState == ST_FETCH) centerText("Loading...", 56, 2);
  else if (stState == ST_BUSY) centerText("Working...", 56, 2);
  else if (stState == ST_ERR) {
    centerText("Can't reach", 36, 2);
    centerText("the server", 54, 2);
    centerText("Check Wi-Fi", 80, 1);
    drawHint("Tap: retry");
  } else if (!stN) centerText("No apps yet", 56, 2);
  else if (stLevel != SL_DETAIL) {
    // 2 x 3 tiles per page
    const int PER = 6, TW = 76, TH = 30;
    int n = storeChoices(), first = (stSel / PER) * PER;
    for (int i = first; i < n && i < first + PER; i++) {
      int x = 2 + ((i - first) % 2) * (TW + 4), y = TITLE_H + 4 + ((i - first) / 2) * (TH + 3);
      bool selected = i == stSel;
      uint16_t ink = selected ? SSD1306_BLACK : SSD1306_WHITE;
      if (selected) display.fillRoundRect(x, y, TW, TH, 4, SSD1306_WHITE);
      else display.drawRoundRect(x, y, TW, TH, 4, SSD1306_WHITE);
      int item = stLevel == SL_CAT ? storeCategoryAt(i) : storeAppAt(i);
      int cat = stLevel == SL_CAT ? item : stCat[item];
      storeIcon(x + 5, y + 9, cat, ink);
      display.setTextColor(ink);
      storeText(stLevel == SL_CAT ? ST_CATS[item] : stName[item], x + 20, y + 11, 9);
    }
    display.setTextColor(SSD1306_WHITE);
    char footer[28];
    snprintf(footer, sizeof(footer), "Hold: open   page %d/%d", stSel / PER + 1, (n + PER - 1) / PER);
    drawHint(footer);
  } else {
    int idx = findInstalled(stId[stCur]);
    const char* action = idx < 0 ? "get" : (strcmp(instVer[idx], stVer[stCur]) == 0 ? "remove" : "update");
    storeIcon(4, TITLE_H + 6, stCat[stCur], SSD1306_WHITE);
    display.setTextSize(2);
    storeText(stName[stCur], 20, TITLE_H + 6, 11);
    display.setTextSize(1);
    char ver[16];
    snprintf(ver, sizeof(ver), "version %s", stVer[stCur]);
    storeText(ver, 20, TITLE_H + 24, 20);
    // Description: up to 4 lines of 17 characters, beside a picture frame for future screenshots
    int pos = 0;
    for (int line = 0; line < 4; line++) {
      int end = min(pos + 17, (int)stDesc.length());
      if (line < 3 && end < (int)stDesc.length() && stDesc[end] != ' ') {
        int space = end;
        while (space > pos && stDesc[space] != ' ') space--;
        if (space > pos) end = space;
      }
      String part = stDesc.substring(pos, end);
      if (line == 3 && end < (int)stDesc.length()) part = part.substring(0, 14) + "...";
      storeText(part.c_str(), 4, 52 + line * 11, 17);
      pos = end;
      while (pos < (int)stDesc.length() && stDesc[pos] == ' ') pos++;
    }
    const int PX = 112, PY = 50;
    display.drawRoundRect(PX, PY, 44, 34, 3, SSD1306_WHITE);
    display.drawCircle(PX + 10, PY + 9, 3, SSD1306_WHITE);
    display.drawLine(PX + 3, PY + 30, PX + 16, PY + 16, SSD1306_WHITE);
    display.drawLine(PX + 16, PY + 16, PX + 25, PY + 25, SSD1306_WHITE);
    display.drawLine(PX + 25, PY + 25, PX + 32, PY + 18, SSD1306_WHITE);
    display.drawLine(PX + 32, PY + 18, PX + 40, PY + 30, SSD1306_WHITE);
    // Outline stars mean unrated; there is no ratings service in the catalog.
    for (int s = 0; s < 5; s++) {
      int x = PX - 2 + s * 9, y = PY + 37;
      display.drawLine(x + 3, y, x + 4, y + 2, SSD1306_WHITE);
      display.drawLine(x + 4, y + 2, x + 6, y + 2, SSD1306_WHITE);
      display.drawLine(x + 6, y + 2, x + 4, y + 4, SSD1306_WHITE);
      display.drawLine(x + 4, y + 4, x + 5, y + 6, SSD1306_WHITE);
      display.drawLine(x + 5, y + 6, x + 3, y + 5, SSD1306_WHITE);
      display.drawLine(x + 3, y + 5, x + 1, y + 6, SSD1306_WHITE);
      display.drawLine(x + 1, y + 6, x + 2, y + 4, SSD1306_WHITE);
      display.drawLine(x + 2, y + 4, x, y + 2, SSD1306_WHITE);
      display.drawLine(x, y + 2, x + 2, y + 2, SSD1306_WHITE);
      display.drawLine(x + 2, y + 2, x + 3, y, SSD1306_WHITE);
    }
    String hint = String("Hold: ") + action;
    drawHint(hint.c_str());
  }
  if (stMsg.length() && millis() - stMsgAt < 2500) drawHint(stMsg.c_str());
  display.setTextWrap(true);
}

// ---------- Script apps (Elk JavaScript) ----------
// An app defines any of: draw() each frame, tap() (main button), hold() (main button held),
// prev() (D6 button). It can draw and use the functions registered in scriptLoad().
const size_t JS_HEAP = 12000;
char* jsHeap = nullptr;
struct js* jsEng = nullptr;
String jsErr;
bool scriptExitReq = false;

static char rbuf[4][160];
static int rbi = 0;
static char* nextBuf() { rbi = (rbi + 1) & 3; rbuf[rbi][0] = 0; return rbuf[rbi]; }
static char httpBuf[1000];

// Store apps are written for a 128 x 64 screen. They draw on their own canvas, which drawScript()
// shows in a framed window in the middle of the TFT, so every existing app works unchanged.
const int APP_W = 128, APP_H = 64;
const int APP_X = (SCREEN_WIDTH - APP_W) / 2, APP_Y = TITLE_H + (SCREEN_HEIGHT - TITLE_H - 12 - APP_H) / 2;
GFXcanvas1 appCanvas(APP_W, APP_H);
static void appCenter(const char* s, int y, int size) {
  appCanvas.setTextSize(size);
  appCanvas.setCursor((APP_W - (int)strlen(s) * 6 * size) / 2, y);
  appCanvas.print(s);
}
static void n_text(int x, int y, int sz, char* s) { appCanvas.setTextSize(sz < 1 ? 1 : sz); appCanvas.setCursor(x, y); appCanvas.print(s); }
static void n_ctext(int y, int sz, char* s) { appCenter(s, y, sz < 1 ? 1 : sz); }
static void n_rect(int x, int y, int w, int h, int fill) { if (fill) appCanvas.fillRect(x, y, w, h, 1); else appCanvas.drawRect(x, y, w, h, 1); }
static void n_line(int a, int b, int c, int d) { appCanvas.drawLine(a, b, c, d, 1); }
static void n_dot(int x, int y) { appCanvas.drawPixel(x, y, 1); }
static void n_circle(int x, int y, int r, int fill) { if (fill) appCanvas.fillCircle(x, y, r, 1); else appCanvas.drawCircle(x, y, r, 1); }
static const char* n_str(int v) { char* b = nextBuf(); snprintf(b, 160, "%d", v); return b; }
static int n_num(char* s) { return atoi(s); }
static int n_find(char* s, char* t) { char* p = strstr(s, t); return p ? (int)(p - s) : -1; }
static const char* n_sub(char* s, int i, int n) {
  char* b = nextBuf();
  int len = strlen(s);
  if (i < 0) i = 0;
  if (i > len) i = len;
  if (n > len - i) n = len - i;
  if (n > 159) n = 159;
  if (n < 0) n = 0;
  memcpy(b, s + i, n);
  b[n] = 0;
  return b;
}
static int n_div(int a, int b) { return b ? a / b : 0; }
static int n_mod(int a, int b) { return b ? a % b : 0; }
static int n_ms() { return (int)(millis() & 0x3FFFFFFF); }
static int n_timeok() { return timeIsSet() ? 1 : 0; }
static int n_hour() { time_t n = time(nullptr); tm t; localtime_r(&n, &t); return t.tm_hour; }
static int n_minute() { time_t n = time(nullptr); tm t; localtime_r(&n, &t); return t.tm_min; }
static int n_second() { time_t n = time(nullptr); tm t; localtime_r(&n, &t); return t.tm_sec; }
static int n_wifi() { return WiFi.status() == WL_CONNECTED ? 1 : 0; }
static void n_exit() { scriptExitReq = true; noTone(BUZZER_PIN); }
static void n_clear(int x, int y, int w, int h) { appCanvas.fillRect(x, y, w, h, 0); }
static int n_lines(char* s) {  // how many text lines size-1 text takes (21 columns, \n breaks)
  int lines = 1, col = 0;
  for (; *s; s++) {
    if (*s == '\n') { lines++; col = 0; }
    else if (*s != '\r') { if (++col > 21) { lines++; col = 1; } }
  }
  return lines;
}
static const char* n_server() { return STORE_URL; }  // base URL of the app server, e.g. for http(server() + '/data.txt')
static int n_knob() { return analogRead(A0); }  // potentiometer on A0: 0..1023
static void n_beep(int freq, int dur) {  // non-blocking tone; freq 0 = silence
  if (freq <= 0) { noTone(BUZZER_PIN); digitalWrite(BUZZER_PIN, LOW); return; }
  if (dur < 1) dur = 1;
  if (dur > 5000) dur = 5000;
  sfx(freq, dur);
}
static const char* n_http(char* url) {
  httpBuf[0] = 0;
  String body;
  if (httpGetText(String(url), body, 4000)) strlcpy(httpBuf, body.c_str(), sizeof(httpBuf));
  return httpBuf;
}
static String dataPath() { return String("/d_") + instId[scriptIdx] + ".txt"; }
static void n_save(char* s) { File f = LittleFS.open(dataPath(), "w"); if (f) { f.print(s); f.close(); } }
static const char* n_load() {
  char* b = nextBuf();
  File f = LittleFS.open(dataPath(), "r");
  if (f) { size_t n = f.readBytes(b, 159); b[n] = 0; f.close(); }
  return b;
}

static void reg(const char* name, uintptr_t fn, const char* sig) { js_set(jsEng, js_glob(jsEng), name, js_import(jsEng, fn, sig)); }

void scriptUnload() {
  jsEng = nullptr;
  free(jsHeap);
  jsHeap = nullptr;
}

void scriptLoad() {
  scriptUnload();
  jsErr = "";
  scriptExitReq = false;
  if (scriptIdx >= instN) { jsErr = "App missing"; return; }
  File f = LittleFS.open(String("/apps/") + instId[scriptIdx] + ".js", "r");
  if (!f) { jsErr = "Can't open app"; return; }
  String code = f.readString();
  f.close();
  jsHeap = (char*)malloc(JS_HEAP);
  if (!jsHeap) { jsErr = "Out of memory"; return; }
  jsEng = js_create(jsHeap, JS_HEAP);
  if (!jsEng) { jsErr = "Engine failed"; scriptUnload(); return; }
  reg("text", (uintptr_t)n_text, "viiis");
  reg("ctext", (uintptr_t)n_ctext, "viis");
  reg("rect", (uintptr_t)n_rect, "viiiii");
  reg("line", (uintptr_t)n_line, "viiii");
  reg("dot", (uintptr_t)n_dot, "vii");
  reg("circle", (uintptr_t)n_circle, "viiii");
  reg("str", (uintptr_t)n_str, "si");
  reg("num", (uintptr_t)n_num, "is");
  reg("find", (uintptr_t)n_find, "iss");
  reg("sub", (uintptr_t)n_sub, "ssii");
  reg("div", (uintptr_t)n_div, "iii");
  reg("mod", (uintptr_t)n_mod, "iii");
  reg("ms", (uintptr_t)n_ms, "i");
  reg("timeok", (uintptr_t)n_timeok, "i");
  reg("hour", (uintptr_t)n_hour, "i");
  reg("minute", (uintptr_t)n_minute, "i");
  reg("second", (uintptr_t)n_second, "i");
  reg("wifi", (uintptr_t)n_wifi, "i");
  reg("http", (uintptr_t)n_http, "ss");
  reg("save", (uintptr_t)n_save, "vs");
  reg("load", (uintptr_t)n_load, "s");
  reg("exit", (uintptr_t)n_exit, "v");
  reg("beep", (uintptr_t)n_beep, "vii");
  reg("knob", (uintptr_t)n_knob, "i");
  reg("clear", (uintptr_t)n_clear, "viiii");
  reg("lines", (uintptr_t)n_lines, "is");
  reg("server", (uintptr_t)n_server, "s");
  jsval_t r = js_eval(jsEng, code.c_str(), ~0);
  const char* s = js_str(jsEng, r);
  if (strncmp(s, "ERROR", 5) == 0) jsErr = s;
}

void scriptEvent(const char* call) {  // missing handlers just give an ignored error
  if (jsEng) js_eval(jsEng, call, ~0);
}

void drawScript() {
  display.setTextSize(1);
  drawTitle(scriptIdx < instN ? instName[scriptIdx] : "App");
  if (!jsEng) {
    centerText("App failed", 44, 2);
    centerText(jsErr.c_str(), 72, 1);
    return;
  }
  appCanvas.fillScreen(0);
  appCanvas.setTextColor(1);
  appCanvas.setTextSize(1);
  jsval_t r = js_eval(jsEng, "draw();", ~0);
  const char* s = js_str(jsEng, r);
  if (strncmp(s, "ERROR", 5) == 0) {  // errors show on the app's bottom line, as before
    appCanvas.fillRect(0, 56, APP_W, 8, 0);
    appCanvas.setTextSize(1);
    appCanvas.setCursor(0, 56);
    appCanvas.print(s);
  }
  if (jsErr.length()) { appCanvas.setTextSize(1); appCanvas.setCursor(0, 56); appCanvas.print(jsErr.c_str()); }
  display.drawRoundRect(APP_X - 3, APP_Y - 3, APP_W + 6, APP_H + 6, 4, SSD1306_WHITE);
  display.drawBitmap(APP_X, APP_Y, appCanvas.getBuffer(), APP_W, APP_H, SSD1306_WHITE);
  drawHint("Hold 1.5 s: Home");
}

// ---------- Jump game (tap to jump over the cacti) ----------
void gameReset() {
  gH = 0; gV = 0; gScore = 0; gFrame = 0; gGap = 0;
  for (int i = 0; i < 2; i++) gAct[i] = false;
}

void gameTap() {
  if (gState == G_IDLE) {
    gameReset();
    gState = G_RUN;
    gV = 5.5f;
    sfx(1200, 30);
  } else if (gState == G_RUN) {
    if (gH <= 0.5f) { gV = 5.5f; sfx(1200, 30); }
  } else if (millis() - gOverAt > 700) {  // game over: tap to retry (short delay avoids accidents)
    gameReset();
    gState = G_RUN;
  }
}

void stepGame() {
  if (gState != G_RUN) return;
  gFrame++;
  if (gH > 0 || gV > 0) {
    gH += gV;
    gV -= 0.55f;
    if (gH <= 0) { gH = 0; gV = 0; }
  }
  float speed = 2.6f + gScore * 0.12f;
  if (speed > 4.8f) speed = 4.8f;

  float lastX = -100;
  for (int i = 0; i < 2; i++) {
    if (!gAct[i]) continue;
    gX[i] -= speed;
    if (!gScored[i] && gX[i] + gW[i] < 12) { gScored[i] = true; gScore++; }
    if (gX[i] + gW[i] < 0) gAct[i] = false;
    else if (gX[i] > lastX) lastX = gX[i];
  }
  if (lastX < SCREEN_WIDTH - gGap) {
    for (int i = 0; i < 2; i++) {
      if (gAct[i]) continue;
      gX[i] = SCREEN_WIDTH;
      gW[i] = random(5, 10);
      gHt[i] = random(8, 17);
      gAct[i] = true;
      gScored[i] = false;
      gGap = random(55, 110) + (int)(speed * 10);
      break;
    }
  }

  // Collision: player is a box about x 13..19, obstacles are boxes standing on the ground
  int bottom = GROUND_Y - (int)gH;
  for (int i = 0; i < 2; i++) {
    if (!gAct[i]) continue;
    if (19 > gX[i] && 13 < gX[i] + gW[i] && bottom > GROUND_Y - gHt[i] + 1) {
      gState = G_OVER;
      gOverAt = millis();
      if (gScore > gBest) { gBest = gScore; saveBest(HI_ADDR, gBest); }
      sfx(180, 300);
      return;
    }
  }
}

void drawGame() {
  display.setTextSize(1);
  char b[16];
  snprintf(b, sizeof(b), "Score %d", gScore);
  display.setCursor(0, 0);
  display.print(b);
  snprintf(b, sizeof(b), "Best %u", gBest);
  display.setCursor(SCREEN_WIDTH - strlen(b) * 6, 0);
  display.print(b);

  display.drawFastHLine(0, GROUND_Y + 1, SCREEN_WIDTH, SSD1306_WHITE);
  for (int i = 0; i < 7; i++) {  // scrolling ground specks
    int x = (i * 29 - (int)(gFrame * 3)) % SCREEN_WIDTH;
    if (x < 0) x += SCREEN_WIDTH;
    display.drawPixel(x, GROUND_Y + 4, SSD1306_WHITE);
  }
  for (int i = 0; i < 2; i++)
    if (gAct[i]) display.fillRoundRect((int)gX[i], GROUND_Y - gHt[i], gW[i], gHt[i], 2, SSD1306_WHITE);

  int top = GROUND_Y - (int)gH - 10;  // little runner with an eye and moving legs
  display.fillRect(12, top, 8, 7, SSD1306_WHITE);
  display.drawPixel(17, top + 2, SSD1306_BLACK);
  if (gH > 0 || gState != G_RUN) {
    display.fillRect(13, top + 7, 2, 3, SSD1306_WHITE);
    display.fillRect(17, top + 7, 2, 3, SSD1306_WHITE);
  } else {
    display.fillRect((gFrame / 3) % 2 ? 13 : 17, top + 7, 2, 3, SSD1306_WHITE);
  }

  if (gState == G_IDLE) {
    centerText("JUMP", 30, 3);
    centerText((millis() / 2500) % 2 == 0 ? "Tap to start" : "Hold long: Home", 66, 1);
  } else if (gState == G_OVER) {
    centerText("GAME OVER", 30, 2);
    snprintf(b, sizeof(b), "Score %d", gScore);
    centerText(b, 54, 2);
    centerText("Tap: retry", 78, 1);
  }
}

// ---------- Snake (tap = turn right, edges wrap around) ----------
void placeFood() {
  for (int t = 0; t < 200; t++) {
    int x = random(0, SN_COLS), y = random(0, SN_ROWS);
    bool ok = true;
    for (int i = 0; i < snLen; i++)
      if (snX[i] == x && snY[i] == y) { ok = false; break; }
    if (ok) { foodX = x; foodY = y; return; }
  }
  foodX = -1;
}

void snakeReset() {
  if (!snX) return;
  snLen = 3; snDir = 0; snNext = 0; snScore = 0;
  for (int i = 0; i < snLen; i++) { snX[i] = 10 - i; snY[i] = SN_ROWS / 2; }
  placeFood();
  snLast = millis();
}

void snakeLoad() {  // called when the app opens
  if (!snX) snX = new uint8_t[SN_MAX];
  if (!snY) snY = new uint8_t[SN_MAX];
  snState = G_IDLE;
  snakeReset();
}

void snakeUnload() {  // called when the app closes
  delete[] snX; snX = nullptr;
  delete[] snY; snY = nullptr;
}

void snakeTurn(int step) {  // step 1 = clockwise, 3 = counter-clockwise
  if (!snX) return;
  if (snState == G_IDLE) {
    snakeReset();
    snState = G_RUN;
  } else if (snState == G_RUN) {
    int cand = (snNext + step) % 4;  // 0 right, 1 down, 2 left, 3 up
    if (cand != (snDir + 2) % 4) snNext = cand;
  } else if (millis() - snOverAt > 700) {
    snakeReset();
    snState = G_RUN;
  }
}

void snakeTap() { snakeTurn(1); }

void updateSnake() {
  if (app != APP_SNAKE || !snX || snState != G_RUN || transActive) return;
  unsigned long interval = 230 - snScore * 6;
  if (interval < 90) interval = 90;
  if (millis() - snLast < interval) return;
  snLast = millis();
  snDir = snNext;
  int nx = snX[0], ny = snY[0];
  if (snDir == 0) nx++;
  else if (snDir == 1) ny++;
  else if (snDir == 2) nx--;
  else ny--;
  nx = (nx + SN_COLS) % SN_COLS;
  ny = (ny + SN_ROWS) % SN_ROWS;
  bool eat = (nx == foodX && ny == foodY);
  int check = snLen - (eat ? 0 : 1);  // the tail moves away unless we grow
  for (int i = 0; i < check; i++) {
    if (snX[i] == nx && snY[i] == ny) {
      snState = G_OVER;
      snOverAt = millis();
      if (snScore > snBest) { snBest = snScore; saveBest(HI2_ADDR, snBest); }
      sfx(180, 300);
      return;
    }
  }
  if (eat && snLen < SN_MAX) {
    memmove(snX + 1, snX, snLen);
    memmove(snY + 1, snY, snLen);
    snLen++;
  } else {
    memmove(snX + 1, snX, snLen - 1);
    memmove(snY + 1, snY, snLen - 1);
  }
  snX[0] = nx;
  snY[0] = ny;
  if (eat) {
    snScore++;
    sfx(1500, 40);
    placeFood();
  }
}

void drawSnake() {
  if (!snX) return;
  display.setTextSize(1);
  char b[16];
  snprintf(b, sizeof(b), "Score %d", snScore);
  display.setCursor(0, 0);
  display.print(b);
  snprintf(b, sizeof(b), "Best %u", snBest);
  display.setCursor(SCREEN_WIDTH - strlen(b) * 6, 0);
  display.print(b);

  for (int i = 0; i < snLen; i++)
    display.fillRect(snX[i] * SN_CELL, SN_TOP + snY[i] * SN_CELL, i == 0 ? SN_CELL : SN_CELL - 1, i == 0 ? SN_CELL : SN_CELL - 1, SSD1306_WHITE);
  if (foodX >= 0) {
    if ((millis() / 250) % 2) display.fillRect(foodX * SN_CELL, SN_TOP + foodY * SN_CELL, SN_CELL - 1, SN_CELL - 1, SSD1306_WHITE);
    else display.drawRect(foodX * SN_CELL, SN_TOP + foodY * SN_CELL, SN_CELL - 1, SN_CELL - 1, SSD1306_WHITE);
  }

  if (snState != G_RUN) {
    display.fillRect(12, 28, 136, 76, SSD1306_BLACK);
    display.drawRoundRect(12, 28, 136, 76, 4, SSD1306_WHITE);
    if (snState == G_IDLE) {
      centerText("SNAKE", 40, 3);
      centerText((millis() / 2500) % 2 == 0 ? "Tap = turn right" : "Hold long: Home", 80, 1);
    } else {
      centerText("GAME OVER", 38, 2);
      snprintf(b, sizeof(b), "Score %d", snScore);
      centerText(b, 62, 2);
      centerText("Tap: retry", 86, 1);
    }
  }
}

// ---------- Wi-Fi setup (inside Settings; phone opens a page on the device's own network) ----------
String htmlEsc(const String& s) {
  String o;
  for (unsigned i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '<') o += "&lt;";
    else if (c == '>') o += "&gt;";
    else if (c == '&') o += "&amp;";
    else if (c == '"') o += "&quot;";
    else if (c == '\'') o += "&#39;";
    else o += c;
  }
  return o;
}

void freeScan() {
  delete[] scanName; scanName = nullptr;
  delete[] scanPct; scanPct = nullptr;
  scanN = 0;
}

void doScan() {
  if (!scanName) scanName = new String[12];
  if (!scanPct) scanPct = new int[12];
  int n = WiFi.scanNetworks();
  scanN = 0;
  for (int i = 0; i < n && scanN < 12; i++) {
    String s = WiFi.SSID(i);
    if (s.length() == 0) continue;
    bool dup = false;
    for (int j = 0; j < scanN; j++)
      if (scanName[j] == s) dup = true;
    if (dup) continue;
    scanName[scanN] = s;
    scanPct[scanN] = constrain(2 * (WiFi.RSSI(i) + 100), 0, 100);
    scanN++;
  }
  WiFi.scanDelete();
}

void portalPage() {
  String h = F("<!DOCTYPE html><html><head><meta name=viewport content='width=device-width,initial-scale=1'><title>xonraOS Wi-Fi</title><style>body{font-family:sans-serif;margin:16px;max-width:420px}input,select,button{width:100%;padding:12px;margin:6px 0;font-size:16px;box-sizing:border-box}button{background:#0a7;color:#fff;border:0;border-radius:6px}</style></head><body><h2>xonraOS Wi-Fi setup</h2>");
  if (netMsg.length()) h += "<p><b>" + htmlEsc(netMsg) + "</b></p>";
  h += F("<form method=POST action=/save><label>Pick your network</label><select name=s>");
  for (int i = 0; i < scanN; i++) {
    String e = htmlEsc(scanName[i]);
    h += "<option value=\"" + e + "\">" + e + " (" + String(scanPct[i]) + "%)</option>";
  }
  h += F("</select><label>or type the name (hidden networks)</label><input name=m><label>Password</label><input name=p type=password><button>Connect</button></form><form action=/rescan><button style='background:#666'>Scan again</button></form></body></html>");
  web->send(200, "text/html", h);
}

void portalSave() {
  String s = web->arg("m");
  if (s.length() == 0) s = web->arg("s");
  String p = web->arg("p");
  if (s.length() == 0 || s.length() > 32 || p.length() > 64) {
    netMsg = "Check the name and password";
    portalPage();
    return;
  }
  strlcpy(pendSsid, s.c_str(), sizeof(pendSsid));
  strlcpy(pendPass, p.c_str(), sizeof(pendPass));
  WiFi.begin(pendSsid, pendPass);
  netState = N_TRYING;
  netT = millis();
  netMsg = "";
  web->send(200, "text/html", F("<html><head><meta name=viewport content='width=device-width,initial-scale=1'></head><body style='font-family:sans-serif;margin:16px'><h2>Connecting...</h2><p>Watch the xonraOS screen. When it says Connected, this setup network switches off.</p></body></html>"));
}

void startPortal() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_NAME, AP_PASS);
  dnsSrv = new DNSServer();
  dnsSrv->start(53, "*", WiFi.softAPIP());
  web = new ESP8266WebServer(80);
  web->on("/", portalPage);
  web->on("/save", HTTP_POST, portalSave);
  web->on("/rescan", []() { doScan(); portalPage(); });
  web->onNotFound([]() {
    web->sendHeader("Location", "http://192.168.4.1/");
    web->send(302, "text/plain", "");
  });
  web->begin();
  portalUp = true;
  netState = N_PORTAL;
}

void stopPortal() {
  if (portalUp) {
    web->stop();
    delete web; web = nullptr;
    dnsSrv->stop();
    delete dnsSrv; dnsSrv = nullptr;
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    portalUp = false;
  }
  freeScan();
}

// Back out of a portal step to the Wi-Fi menu (also used when leaving Settings)
void netCancel() {
  bool wasTrying = (netState == N_TRYING);
  stopPortal();
  netState = N_MENU;
  netMsg = "";
  if (wasTrying || (WiFi.status() != WL_CONNECTED && !WiFi.isConnected())) WiFi.begin(curSsid, curPass);
}

void netSelect() {
  if (netCursor == 0) {
    netState = N_SCAN;
    netT = millis();
    netMsg = "";
  } else if (netCursor == 1) {
    useDefaultCreds();
    saveCreds(true);
    WiFi.disconnect();
    WiFi.begin(curSsid, curPass);
  } else if (netCursor == 2) {
    WiFi.disconnect();
    WiFi.begin(curSsid, curPass);
  } else {
    inNet = false;  // Back
  }
}

void updateNet() {
  if (portalUp) {
    dnsSrv->processNextRequest();
    web->handleClient();
  }
  if (app != APP_SETTINGS || !inNet) return;
  if (netState == N_SCAN && millis() - netT > 200) {
    doScan();
    startPortal();
  } else if (netState == N_TRYING) {
    if (millis() - netT > 1500 && WiFi.status() == WL_CONNECTED && WiFi.SSID().equals(pendSsid)) {
      strlcpy(curSsid, pendSsid, sizeof(curSsid));
      strlcpy(curPass, pendPass, sizeof(curPass));
      saveCreds(false);
      stopPortal();
      netState = N_OK;
      netT = millis();
    } else if (millis() - netT > 20000) {
      WiFi.begin(curSsid, curPass);
      netMsg = "Couldn't connect";
      netState = N_PORTAL;
    }
  }
}

void drawNet() {
  display.setTextSize(1);
  char b[27];
  if (netState == N_MENU) {
    drawTitle("Wi-Fi setup", "Hold=OK");
    strlcpy(b, curSsid, 25);
    display.setCursor(4, TITLE_H + 6);
    display.print(b);
    display.setCursor(4, TITLE_H + 18);
    if (WiFi.status() == WL_CONNECTED) display.print(WiFi.localIP().toString());
    else display.print("Not connected");
    drawWifi(SCREEN_WIDTH - 14, TITLE_H + 6);
    const char* items[4] = { "Add new network", "Use default network", "Reconnect", "Back" };
    for (int i = 0; i < 4; i++) {
      int top = 46 + i * 18;
      bool sel = (i == netCursor);
      if (sel) display.fillRoundRect(2, top, SCREEN_WIDTH - 4, 16, 3, SSD1306_WHITE);
      else display.drawRoundRect(2, top, SCREEN_WIDTH - 4, 16, 3, SSD1306_WHITE);
      display.setTextColor(sel ? SSD1306_BLACK : SSD1306_WHITE);
      display.setCursor(8, top + 4);
      display.print(items[i]);
    }
    display.setTextColor(SSD1306_WHITE);
  } else if (netState == N_SCAN) {
    drawTitle("Wi-Fi setup");
    centerText("Scanning", 50, 2);
    centerText("networks...", 70, 1);
  } else if (netState == N_PORTAL) {
    drawTitle("Setup from phone");
    display.setCursor(4, TITLE_H + 6);
    display.print("1 Join this Wi-Fi:");
    display.setTextSize(2);
    display.setCursor(4, TITLE_H + 18);
    display.print(AP_NAME);
    display.setTextSize(1);
    display.setCursor(4, TITLE_H + 38);
    display.print("  password: ");
    display.print(AP_PASS);
    display.setCursor(4, TITLE_H + 54);
    display.print("2 Open this in a browser:");
    display.setTextSize(2);
    display.setCursor(4, TITLE_H + 66);
    display.print("192.168.4.1");
    display.setTextSize(1);
    if (netMsg.length()) { display.setCursor(4, TITLE_H + 88); display.print(netMsg); }
    drawHint("Tap: cancel");
  } else if (netState == N_TRYING) {
    drawTitle("Wi-Fi setup");
    centerText("Connecting to", 40, 1);
    strlcpy(b, pendSsid, 14);
    centerText(b, 56, 2);
    int dots = (millis() / 400) % 4;
    char d[5] = "    ";
    for (int i = 0; i < dots; i++) d[i] = '.';
    centerText(d, 80, 2);
  } else {
    drawTitle("Wi-Fi setup");
    centerText("Connected!", 26, 2);
    strlcpy(b, curSsid, 14);
    centerText(b, 54, 2);
    centerText(WiFi.localIP().toString().c_str(), 80, 1);
    drawHint("Saved. Tap: done");
  }
}

// ---------- Firmware update (downloads a signed build from the app server) ----------
// TFT builds live in their own folder so a TFT board never installs the OLED build:
// the server serves firmware/tft/version.txt (a number) and firmware/tft/xonraos.bin (the build, signed with
// the private key that matches public.key in this folder). The board only accepts signed builds.
enum FwState { FW_CHECK, FW_UPTODATE, FW_AVAIL, FW_BUSY, FW_FAIL };
int fwState = FW_CHECK, fwLatest = 0;
unsigned long fwReq = 0;
String fwMsg;

void fwOpen() {
  inFw = true;
  fwState = FW_CHECK;
  fwReq = millis();
  fwMsg = "";
}

void updateFw() {
  if (app != APP_SETTINGS || !inFw || transActive || millis() - fwReq < 150) return;  // let one frame draw first
  if (fwState == FW_CHECK) {
    String body;
    if (!httpGetText(String(STORE_URL) + "/firmware/tft/version.txt", body, 32)) { fwState = FW_FAIL; fwMsg = "Can't reach server"; return; }
    fwLatest = body.toInt();
    if (fwLatest <= 0) { fwState = FW_FAIL; fwMsg = "No update info"; return; }
    fwState = fwLatest > FW_VERSION ? FW_AVAIL : FW_UPTODATE;
  } else if (fwState == FW_BUSY) {
    WiFiClient c;
    ESPhttpUpdate.rebootOnUpdate(true);
    ESPhttpUpdate.update(c, String(STORE_URL) + "/firmware/tft/xonraos.bin");  // reboots by itself when it succeeds
    fwState = FW_FAIL;  // only reached when the update did not happen
    fwMsg = ESPhttpUpdate.getLastErrorString().substring(0, 21);
    if (fwMsg.length() == 0) fwMsg = "Update failed";
  }
}

void drawFw() {
  display.setTextSize(1);
  drawTitle("Firmware");
  char b[28];
  if (fwState == FW_CHECK) { centerText("Checking...", 56, 2); return; }
  if (fwState == FW_BUSY) {
    centerText("Updating...", 40, 2);
    centerText("Don't unplug!", 72, 1);
    return;
  }
  if (fwState == FW_FAIL) {
    centerText("Failed", 32, 2);
    centerText(fwMsg.c_str(), 62, 1);
    drawHint("Hold: retry  Tap: back");
    return;
  }
  snprintf(b, sizeof(b), "Installed: %s", FW_VERSION_LABEL);
  centerText(b, 28, 1);
  snprintf(b, sizeof(b), "Latest build: %d", fwLatest);
  centerText(b, 44, 1);
  if (fwState == FW_AVAIL) {
    centerText("Update", 66, 2);
    centerText("available", 86, 2);
    drawHint("Hold: install  Tap: back");
  } else {
    centerText("Up to date", 72, 2);
    drawHint("Tap: back");
  }
}

// ---------- Settings app ----------
FSInfo storageInfo;
bool storageOk = false;

void memoryOpen() {
  inMemory = true;
  storageOk = LittleFS.info(storageInfo);
}

void drawMemory() {
  char b[32];
  drawTitle("Storage / Memory");
  const int y0 = TITLE_H + 10, step = 20;
  if (storageOk) {
    snprintf(b, sizeof(b), "Used: %lu/%lu KiB", (unsigned long)(storageInfo.usedBytes / 1024),
             (unsigned long)(storageInfo.totalBytes / 1024));
    centerText(b, y0, 1);
    size_t freeBytes = storageInfo.totalBytes > storageInfo.usedBytes ? storageInfo.totalBytes - storageInfo.usedBytes : 0;
    snprintf(b, sizeof(b), "Free: %lu KiB", (unsigned long)(freeBytes / 1024));
    centerText(b, y0 + step, 1);
    // usage bar
    int bw = SCREEN_WIDTH - 24;
    int used = storageInfo.totalBytes ? (int)((uint64_t)storageInfo.usedBytes * (bw - 2) / storageInfo.totalBytes) : 0;
    display.drawRect(12, y0 + step + 12, bw, 6, SSD1306_WHITE);
    display.fillRect(13, y0 + step + 13, used, 4, SSD1306_WHITE);
  } else {
    centerText("Storage unavailable", y0, 1);
  }
  snprintf(b, sizeof(b), "RAM free: %lu KiB", (unsigned long)(ESP.getFreeHeap() / 1024));
  centerText(b, y0 + step * 2 + 12, 1);
  snprintf(b, sizeof(b), "Apps: %d/%d", instN, MAX_INST);
  centerText(b, y0 + step * 3 + 12, 1);
  drawHint("Tap: back  Hold: refresh");
}

const char* settingLabel[SET_N] = { "Time format", "Brightness", "Seconds", "Animations", "D6 button", "Sound", "Wi-Fi setup", "Firmware", "Storage/Memory" };
const char* settingValue(int i) {
  switch (i) {
    case 0: return prefs.use24 ? "24h" : "12h";
    case 1: return BRIGHT_NAMES[prefs.bright % 3];
    case 2: return prefs.secs ? "On" : "Off";
    case 3: return prefs.anim ? "On" : "Off";
    case 4: return CTRL_NAMES[prefs.ctrl % 3];
    case 5: return prefs.sound ? "On" : "Off";
    case 7: return FW_VERSION_LABEL;
    default: return ">";
  }
}

void settingsActivate() {
  switch (setCursor) {
    case 0: prefs.use24 = !prefs.use24; break;
    case 1: prefs.bright = (prefs.bright + 1) % 3; applyBrightness(); break;
    case 2: prefs.secs = !prefs.secs; break;
    case 3: prefs.anim = !prefs.anim; break;
    case 4: prefs.ctrl = (prefs.ctrl + 1) % 3; break;
    case 5: prefs.sound = !prefs.sound; if (prefs.sound) sfx(1000, 60); break;
    case 7: fwOpen(); return;
    case 8: memoryOpen(); return;
    default:
      inNet = true;
      netState = N_MENU;
      netCursor = 0;
      return;
  }
  savePrefs();
}

void drawSettings() {
  if (inMemory) { drawMemory(); return; }
  if (inNet) { drawNet(); return; }
  if (inFw) { drawFw(); return; }
  display.setTextSize(1);
  const int ROW_H = 14, LIST_Y = TITLE_H + 2, VISIBLE = 7;  // 7 rows visible, the list scrolls smoothly
  int top = setCursor > VISIBLE - 1 ? setCursor - (VISIBLE - 1) : 0;
  setScroll = smoothTo(setScroll, top * ROW_H);
  setBar = smoothTo(setBar, setCursor * ROW_H);
  display.fillRoundRect(2, LIST_Y + (int)(setBar - setScroll), SCREEN_WIDTH - 4, ROW_H - 1, 3, SSD1306_WHITE);
  for (int i = 0; i < SET_N; i++) {
    int ry = LIST_Y + i * ROW_H - (int)setScroll;
    if (ry < LIST_Y - ROW_H || ry > SCREEN_HEIGHT) continue;
    bool sel = fabsf(i * ROW_H - setBar) < ROW_H / 2;
    display.setTextColor(sel ? SSD1306_BLACK : SSD1306_WHITE);
    display.setCursor(8, ry + 3);
    display.print(settingLabel[i]);
    const char* v = settingValue(i);
    display.setCursor(SCREEN_WIDTH - 8 - strlen(v) * 6, ry + 3);
    display.print(v);
  }
  display.setTextColor(SSD1306_WHITE);
  // scroll position marker on the right edge
  int barH = (SCREEN_HEIGHT - LIST_Y) * VISIBLE / SET_N;
  int barY = LIST_Y + (int)(setScroll * (SCREEN_HEIGHT - LIST_Y - barH) / ((SET_N - VISIBLE) * ROW_H));
  display.drawFastVLine(SCREEN_WIDTH - 1, barY, barH, SSD1306_WHITE);
  drawTitle("Settings", "Hold=change");  // drawn last so it clips rows scrolling under it
}

// ---------- Home screen (2 x 2 tiles, pages of 4) ----------
void drawClockIcon(int cx, int cy, uint16_t c) {
  display.drawCircle(cx, cy, 8, c);
  display.drawLine(cx, cy, cx, cy - 6, c);
  display.drawLine(cx, cy, cx + 4, cy + 2, c);
}

void drawGameIcon(int cx, int cy, uint16_t c) {
  display.fillRect(cx - 9, cy - 5, 5, 5, c);
  display.fillRect(cx - 8, cy, 1, 3, c);
  display.fillRect(cx - 6, cy, 1, 3, c);
  display.fillRect(cx + 4, cy - 4, 3, 7, c);
  display.drawFastHLine(cx - 10, cy + 4, 20, c);
}

void drawSnakeIcon(int cx, int cy, uint16_t c) {
  const int8_t bx[6] = { -9, -6, -3, -3, 0, 3 };
  const int8_t by[6] = { 3, 3, 3, 0, 0, 0 };
  for (int i = 0; i < 6; i++) display.fillRect(cx + bx[i], cy + by[i] - 2, 3, 3, c);
  display.drawRect(cx + 7, cy - 1, 3, 3, c);  // food
}

void drawStoreIcon(int cx, int cy, uint16_t c) {  // shopping bag
  display.drawRect(cx - 7, cy - 3, 14, 11, c);
  display.drawFastHLine(cx - 4, cy - 6, 8, c);
  display.drawFastVLine(cx - 4, cy - 6, 3, c);
  display.drawFastVLine(cx + 3, cy - 6, 3, c);
}

void drawGearIcon(int cx, int cy, uint16_t c) {
  display.drawCircle(cx, cy, 5, c);
  display.fillCircle(cx, cy, 1, c);
  for (int a = 0; a < 8; a++) {
    float t = a * 0.7854f;
    display.drawLine(cx + (int)lroundf(6 * cosf(t)), cy + (int)lroundf(6 * sinf(t)),
                     cx + (int)lroundf(9 * cosf(t)), cy + (int)lroundf(9 * sinf(t)), c);
  }
}

float homeScroll = 0;  // pixels, animated

void drawHome() {
  // Tiles in rows of two; three rows are visible and the rows scroll smoothly to keep the selection in view
  const int TILE_W = 74, TILE_H = 30, ROW_STEP = 34, GRID_Y = TITLE_H + 3, ROWS_SHOWN = 3;
  static int topRow = 0;
  int selRow = homeSel / 2;
  if (selRow < topRow) topRow = selRow;
  if (selRow > topRow + ROWS_SHOWN - 1) topRow = selRow - (ROWS_SHOWN - 1);
  homeScroll = smoothTo(homeScroll, topRow * ROW_STEP);
  const char* names[HOME_BUILTIN] = { "Clock", "Jump", "Snake", "Store", "Settings" };
  for (int i = 0; i < HOME_N; i++) {
    int x = 4 + (i % 2) * 78, y = GRID_Y + (i / 2) * ROW_STEP - (int)homeScroll;
    if (y > HINT_Y || y + TILE_H < TITLE_H) continue;
    bool sel = (i == homeSel);
    uint16_t fg = sel ? SSD1306_BLACK : SSD1306_WHITE;
    if (sel) display.fillRoundRect(x, y, TILE_W, TILE_H, 5, SSD1306_WHITE);
    else display.drawRoundRect(x, y, TILE_W, TILE_H, 5, SSD1306_WHITE);
    int ix = x + TILE_W / 2, iy = y + 10;  // icon centred at the top, name underneath
    if (i == 0) drawClockIcon(ix, iy, fg);
    else if (i == 1) drawGameIcon(ix, iy, fg);
    else if (i == 2) drawSnakeIcon(ix, iy, fg);
    else if (i == 3) drawStoreIcon(ix, iy, fg);
    else if (i == 4) drawGearIcon(ix, iy, fg);
    else {  // installed app: four little squares
      for (int q = 0; q < 4; q++) display.fillRect(ix - 7 + (q % 2) * 8, iy - 7 + (q / 2) * 8, 6, 6, fg);
    }
    display.setTextColor(fg);
    display.setTextSize(1);
    char nb[12];
    if (i < HOME_BUILTIN) strlcpy(nb, names[i], sizeof(nb));
    else strlcpy(nb, instName[i - HOME_BUILTIN], sizeof(nb));  // 74 px fits the full 11-char name
    display.setCursor(x + (TILE_W - (int)strlen(nb) * 6) / 2, y + 21);
    display.print(nb);
  }
  display.setTextColor(SSD1306_WHITE);
  // Title and hint are drawn last so they clip tiles scrolling under them
  char cnt[10];
  snprintf(cnt, sizeof(cnt), "%d/%d", homeSel + 1, HOME_N);
  drawTitle("Apps", cnt);
  drawHint("Tap=next Hold=open");
}

void drawFrame() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  if (app == APP_HOME) { drawHome(); return; }
  if (app == APP_GAME) { drawGame(); return; }
  if (app == APP_SNAKE) { drawSnake(); return; }
  if (app == APP_STORE) { drawStore(); return; }
  if (app == APP_SETTINGS) { drawSettings(); return; }
  if (app == APP_SCRIPT) { drawScript(); return; }
  if (!timeIsSet()) {
    drawConnecting();
    return;
  }
  time_t now = time(nullptr);
  tm t;
  localtime_r(&now, &t);
  if (t.tm_sec != lastSec) {
    lastSec = t.tm_sec;
    secStartMs = millis();
  }
  drawClock(t);
}

// ---------- Screen changes (slide transition) ----------
void startSlide() {
  if (prefs.anim && (app != APP_CLOCK || timeIsSet())) {
    drawFrame();
    memcpy(newFrame, display.getBuffer(), FRAME_BYTES);
    transStart = millis();
    transActive = true;
  }
}

// Apps are loaded when opened and unloaded when closed
void appLeave(int a) {
  if (a == APP_STORE) { stDesc = ""; stDescPending = false; }
  if (a == APP_SNAKE) snakeUnload();
  if (a == APP_SCRIPT) scriptUnload();
  if (a == APP_SETTINGS) {
    if (inNet) netCancel();
    inNet = false;
    inFw = false;
    inMemory = false;
  }
}

void appEnter(int a) {
  if (a == APP_GAME) { gState = G_IDLE; gameReset(); }
  if (a == APP_SNAKE) snakeLoad();
  if (a == APP_SCRIPT) scriptLoad();
  if (a == APP_STORE) storeOpen();
  if (a == APP_SETTINGS) { setCursor = 0; setScroll = 0; setBar = 0; inNet = false; inFw = false; inMemory = false; }
}

void gotoApp(int a) {
  if (a == app) return;
  appLeave(app);
  app = a;
  appEnter(a);
  startSlide();
}

void stepTransition() {
  float p = (millis() - transStart) / (float)TRANS_MS;
  if (p > 1) p = 1;
  float e = 1 - (1 - p) * (1 - p) * (1 - p);
  // The frame is stored row by row (8 pixels per byte), so slide in whole bytes (8-pixel steps)
  int sb = (int)(e * ROW_BYTES);
  if (sb > ROW_BYTES) sb = ROW_BYTES;
  uint8_t* buf = display.getBuffer();
  for (int r = 0; r < SCREEN_HEIGHT; r++) {
    int base = r * ROW_BYTES;
    memcpy(buf + base, lastFrame + base + sb, ROW_BYTES - sb);
    memcpy(buf + base + ROW_BYTES - sb, newFrame + base, sb);
  }
  display.display();
  if (p >= 1) {
    transActive = false;
    memcpy(lastFrame, newFrame, FRAME_BYTES);
  }
}

// ---------- Button handling ----------
void goHome() {
  if (app == APP_HOME) return;
  homeSel = (app == APP_SCRIPT) ? HOME_BUILTIN + scriptIdx : app - 1;
  gotoApp(APP_HOME);
}

void openHomeSel() {
  if (homeSel < HOME_BUILTIN) {
    gotoApp(homeSel + 1);
  } else {
    scriptIdx = homeSel - HOME_BUILTIN;
    gotoApp(APP_SCRIPT);
  }
}

void onShortPress() {
  switch (app) {
    case APP_HOME: homeSel = (homeSel + 1) % HOME_N; break;
    case APP_SCRIPT: scriptEvent("tap();"); break;
    case APP_STORE:
      storeNext();
      break;
    case APP_SETTINGS:
      if (inMemory) {
        inMemory = false;
      } else if (inFw) {
        if (fwState != FW_BUSY) inFw = false;  // tap = back
      } else if (inNet) {
        if (netState == N_MENU) netCursor = (netCursor + 1) % 4;
        else netCancel();
      } else {
        setCursor = (setCursor + 1) % SET_N;
      }
      break;
    default: break;  // games act the moment you press, see readButton
  }
}

void onLongPress() {
  switch (app) {
    case APP_HOME: openHomeSel(); break;
    case APP_SCRIPT: scriptEvent("hold();"); break;
    case APP_STORE:
      storeSelect();
      break;
    case APP_SETTINGS:
      if (inMemory) {
        memoryOpen();
      } else if (inFw) {
        if (fwState == FW_AVAIL) { fwState = FW_BUSY; fwReq = millis(); }  // hold = install
        else if (fwState != FW_BUSY) fwOpen();  // hold = check again
      } else if (inNet) {
        if (netState == N_MENU) netSelect();
        else netCancel();
      } else {
        settingsActivate();
      }
      break;
    default: break;
  }
}

void onHomePress() {
  if (app == APP_STORE && storeBack()) return;
  if (app == APP_HOME) { onLongPress(); return; }
  goHome();
}

// Second button (D6). Off / Home (always goes Home) / Back (previous, left, jump)
void extraPress() {
  if (prefs.ctrl == 0 || transActive) return;
  if (prefs.ctrl == 1) { goHome(); return; }
  switch (app) {
    case APP_HOME: homeSel = (homeSel + HOME_N - 1) % HOME_N; break;
    case APP_GAME: gameTap(); break;
    case APP_SNAKE: snakeTurn(3); break;
    case APP_SCRIPT: scriptEvent("prev();"); break;
    case APP_STORE:
      storePrevious();
      break;
    case APP_SETTINGS:
      if (inMemory) {
        inMemory = false;
      } else if (inFw) {
        if (fwState != FW_BUSY) inFw = false;
      } else if (inNet) {
        if (netState == N_MENU) netCursor = (netCursor + 3) % 4;
        else netCancel();
      } else {
        setCursor = (setCursor + SET_N - 1) % SET_N;
      }
      break;
    default: goHome(); break;
  }
}

void readExtraButton() {
  static bool prev = false;
  static unsigned long t0 = 0;
  bool h = (digitalRead(HOME_BTN) == LOW);
  if (h && !prev && millis() - t0 > 60) { t0 = millis(); extraPress(); }
  prev = h;
}

void readButton() {
  readExtraButton();
  bool reading = (digitalRead(BUTTON_PIN) == LOW);
  if (reading != lastReading) {
    lastChange = millis();
    lastReading = reading;
  }
  if (millis() - lastChange <= DEBOUNCE_MS) return;

  if (reading && !pressed) {
    pressed = true;
    pressStart = millis();
    longDone = false;
    if (!transActive) {
      if (app == APP_GAME) gameTap();
      else if (app == APP_SNAKE) snakeTap();
    }
  } else if (!reading && pressed) {
    pressed = false;
    if (!longDone) {
      if (millis() - pressStart >= LONG_PRESS_MS) onLongPress();  // acts on release, so a Home hold never fires it
      else onShortPress();
    }
  }
  if (pressed && !longDone && millis() - pressStart >= HOME_PRESS_MS) {
    longDone = true;
    onHomePress();
  }
}

// ---------- Startup ----------
void startupChime() {
  if (!prefs.sound) return;
  sfx(1047, 100);
  delay(130);
  sfx(1319, 100);
  delay(130);
  sfx(1568, 180);
  delay(200);
  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, LOW);
}

void splash() {
  display.clearDisplay();
  centerText("welcome", 56, 2);
  display.display();
  startupChime();
  delay(prefs.sound ? 440 : 900);

  display.clearDisplay();
  centerText("xonraOS", 40, 3);  // 7 letters x 18 px = 126 px wide
  display.drawFastHLine(30, 70, SCREEN_WIDTH - 60, SSD1306_WHITE);
  centerText(FW_VERSION_LABEL, 80, 1);
  display.display();
  delay(1100);
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  loadPrefs();
  loadCreds();
  if (!LittleFS.begin()) {  // first run: format the flash file system
    LittleFS.format();
    LittleFS.begin();
  }
  LittleFS.mkdir("/apps");
  refreshInstalled();

  tft.initR(TFT_TAB);
  tft.setRotation(TFT_ROTATION);
  tft.setSPISpeed(40000000);  // lower to 20000000 if the picture is garbled
  tft.fillScreen(BG_COLOR);
  // SPI claims GPIO12 (D6) as MISO; the TFT never sends data back, so take it back for the second button
  pinMode(HOME_BTN, INPUT_PULLUP);
  display.setTextColor(SSD1306_WHITE);
  applyBrightness();

  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.begin(curSsid, curPass);
  configTime(TIME_ZONE, "pool.ntp.org", "time.google.com");

  if (prefs.anim) splash();
  else startupChime();
  memset(lastFrame, 0, FRAME_BYTES);
}

void loop() {
  readButton();
  updateSnake();
  updateNet();
  updateStore();
  updateFw();
  if (scriptExitReq) { scriptExitReq = false; goHome(); }

  if (millis() - lastDraw < FRAME_MS) return;
  lastDraw = millis();

  if (transActive) {
    stepTransition();
    return;
  }
  if (app == APP_GAME) stepGame();
  drawFrame();
  // Line along the bottom fills while you hold, showing the Home gesture is coming
  // (the bottom, because the title bar at the top is already solid)
  if (pressed && !longDone && app != APP_HOME) {
    unsigned long held = millis() - pressStart;
    if (held >= LONG_PRESS_MS) {
      unsigned long w = (held - LONG_PRESS_MS) * SCREEN_WIDTH / (HOME_PRESS_MS - LONG_PRESS_MS);
      display.fillRect(0, SCREEN_HEIGHT - 3, SCREEN_WIDTH, 3, SSD1306_BLACK);
      display.fillRect(0, SCREEN_HEIGHT - 2, w > SCREEN_WIDTH ? SCREEN_WIDTH : (int)w, 2, SSD1306_WHITE);
    }
  }
  display.display();
  memcpy(lastFrame, display.getBuffer(), FRAME_BYTES);
}
