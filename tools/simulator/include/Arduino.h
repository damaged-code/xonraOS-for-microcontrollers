// Minimal Arduino core for the xonraOS desktop simulator (Windows). Only what the firmware and Adafruit GFX use.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <string>
#include <vector>
#include <functional>

#define SIMULATOR 1
#define ARDUINO 10819

typedef uint8_t byte;
typedef bool boolean;

// ---- pins (NodeMCU names -> GPIO numbers, like the ESP8266 core) ----
#define D0 16
#define D1 5
#define D2 4
#define D3 0
#define D4 2
#define D5 14
#define D6 12
#define D7 13
#define D8 15
#define A0 17
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

// ---- flash-memory helpers (plain memory on a PC) ----
#define PROGMEM
#define PSTR(s) (s)
#define F(s) (s)
#define pgm_read_byte(a) (*(const uint8_t *)(a))
#define pgm_read_word(a) (*(const uint16_t *)(a))
#define pgm_read_dword(a) (*(const uint32_t *)(a))
#define pgm_read_pointer(a) (*(void *const *)(a))
class __FlashStringHelper;
#define radians(deg) ((deg) * 0.017453292519943295)
#define degrees(rad) ((rad) * 57.29577951308232)
#define ICACHE_RAM_ATTR
#define IRAM_ATTR

#include <type_traits>
// Return by value (a reference here would dangle: that bug once made the Store page crash)
template <class A, class B> inline typename std::common_type<A, B>::type min(A a, B b) { return a < b ? a : b; }
template <class A, class B> inline typename std::common_type<A, B>::type max(A a, B b) { return a > b ? a : b; }
#define constrain(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))

size_t strlcpy(char *dst, const char *src, size_t size);
#ifndef localtime_r
struct tm *sim_localtime_r(const time_t *t, struct tm *out);
#define localtime_r sim_localtime_r
#endif

unsigned long millis();
unsigned long micros();
void delay(unsigned long ms);
void yield();
void pinMode(int pin, int mode);
int digitalRead(int pin);
void digitalWrite(int pin, int v);
int analogRead(int pin);
void tone(int pin, unsigned freq, unsigned long ms = 0);
void noTone(int pin);
long random(long howbig);
long random(long lo, long hi);
void randomSeed(unsigned long);
void configTime(const char *tz, const char *s1, const char *s2 = nullptr, const char *s3 = nullptr);

// ---- String (Arduino style, backed by std::string) ----
class String {
 public:
  std::string s;
  String() {}
  String(const char *c) : s(c ? c : "") {}
  String(const std::string &x) : s(x) {}
  String(char c) : s(1, c) {}
  String(int v) : s(std::to_string(v)) {}
  String(unsigned v) : s(std::to_string(v)) {}
  String(long v) : s(std::to_string(v)) {}
  String(unsigned long v) : s(std::to_string(v)) {}
  String(long long v) : s(std::to_string(v)) {}
  String(unsigned long long v) : s(std::to_string(v)) {}
  String(double v) : s(std::to_string(v)) {}
  unsigned int length() const { return (unsigned)s.size(); }
  const char *c_str() const { return s.c_str(); }
  char operator[](unsigned i) const { return i < s.size() ? s[i] : 0; }
  char &operator[](unsigned i) { return s[i]; }
  char charAt(unsigned i) const { return (*this)[i]; }
  String substring(unsigned from) const { return from >= s.size() ? String() : String(s.substr(from)); }
  String substring(unsigned from, unsigned to) const {
    if (from > to) { unsigned t = from; from = to; to = t; }
    if (from >= s.size()) return String();
    if (to > s.size()) to = (unsigned)s.size();
    return String(s.substr(from, to - from));
  }
  int indexOf(char c, unsigned from = 0) const { size_t p = s.find(c, from); return p == std::string::npos ? -1 : (int)p; }
  int indexOf(const char *t, unsigned from = 0) const { size_t p = s.find(t, from); return p == std::string::npos ? -1 : (int)p; }
  int indexOf(const String &t, unsigned from = 0) const { return indexOf(t.c_str(), from); }
  int lastIndexOf(char c) const { size_t p = s.rfind(c); return p == std::string::npos ? -1 : (int)p; }
  void trim() {
    size_t a = 0, b = s.size();
    while (a < b && isspace((unsigned char)s[a])) a++;
    while (b > a && isspace((unsigned char)s[b - 1])) b--;
    s = s.substr(a, b - a);
  }
  bool startsWith(const String &p) const { return s.compare(0, p.s.size(), p.s) == 0; }
  bool endsWith(const String &p) const { return s.size() >= p.s.size() && s.compare(s.size() - p.s.size(), p.s.size(), p.s) == 0; }
  bool equals(const String &o) const { return s == o.s; }
  bool equalsIgnoreCase(const String &o) const {
    if (s.size() != o.s.size()) return false;
    for (size_t i = 0; i < s.size(); i++) if (tolower((unsigned char)s[i]) != tolower((unsigned char)o.s[i])) return false;
    return true;
  }
  long toInt() const { return atol(s.c_str()); }
  void toLowerCase() { for (auto &c : s) c = (char)tolower((unsigned char)c); }
  bool concat(const String &o) { s += o.s; return true; }
  String &operator+=(const String &o) { s += o.s; return *this; }
  String &operator+=(const char *o) { s += o ? o : ""; return *this; }
  String &operator+=(char c) { s += c; return *this; }
  String &operator+=(int v) { s += std::to_string(v); return *this; }
  bool operator==(const String &o) const { return s == o.s; }
  bool operator==(const char *o) const { return s == (o ? o : ""); }
  bool operator!=(const String &o) const { return s != o.s; }
  bool operator<(const String &o) const { return s < o.s; }
};
inline String operator+(const String &a, const String &b) { return String(a.s + b.s); }
inline String operator+(const String &a, const char *b) { return String(a.s + (b ? b : "")); }
inline String operator+(const char *a, const String &b) { return String(std::string(a ? a : "") + b.s); }
inline String operator+(const String &a, char c) { return String(a.s + c); }
inline String operator+(const String &a, int v) { return String(a.s + std::to_string(v)); }

// ---- Print ----
class Print {
 public:
  virtual ~Print() {}
  virtual size_t write(uint8_t c) = 0;
  virtual size_t write(const uint8_t *b, size_t n) { size_t k = 0; while (n--) k += write(*b++); return k; }
  size_t write(const char *str) { return str ? write((const uint8_t *)str, strlen(str)) : 0; }
  size_t print(const char *s) { return write(s); }
  size_t print(const String &s) { return write(s.c_str()); }
  size_t print(char c) { return write((uint8_t)c); }
  size_t print(int v, int base = 10) { return printNum((long)v, base); }
  size_t print(unsigned v, int base = 10) { return printNum((unsigned long)v, base); }
  size_t print(long v, int base = 10) { return printNum(v, base); }
  size_t print(unsigned long v, int base = 10) { return printNum(v, base); }
  size_t print(double v, int digits = 2) { char b[40]; snprintf(b, sizeof b, "%.*f", digits, v); return write(b); }
  template <class T> size_t println(const T &v) { size_t n = print(v); return n + write("\r\n"); }
  size_t println() { return write("\r\n"); }
 private:
  size_t printNum(long v, int base) { char b[40]; if (base == 16) snprintf(b, sizeof b, "%lx", v); else snprintf(b, sizeof b, "%ld", v); return write(b); }
  size_t printNum(unsigned long v, int base) { char b[40]; if (base == 16) snprintf(b, sizeof b, "%lx", v); else snprintf(b, sizeof b, "%lu", v); return write(b); }
};

class HardwareSerial : public Print {
 public:
  void begin(unsigned long) {}
  size_t write(uint8_t c) override { fputc(c, stdout); return 1; }
  using Print::write;
};
extern HardwareSerial Serial;

class EspClass {
 public:
  uint32_t getFreeHeap();
  void restart();
};
extern EspClass ESP;
