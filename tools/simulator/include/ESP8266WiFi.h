// Simulated Wi-Fi: always connected to "Simulator Wi-Fi".
#pragma once
#include "Arduino.h"

enum { WL_IDLE_STATUS = 0, WL_CONNECTED = 3, WL_DISCONNECTED = 6 };
enum WiFiMode_t { WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2, WIFI_AP_STA = 3 };
enum WiFiSleepType_t { WIFI_NONE_SLEEP = 0 };

class IPAddress {
 public:
  uint8_t b[4];
  IPAddress(uint8_t a = 0, uint8_t c = 0, uint8_t d = 0, uint8_t e = 0) { b[0] = a; b[1] = c; b[2] = d; b[3] = e; }
  String toString() const { char t[20]; snprintf(t, sizeof t, "%u.%u.%u.%u", b[0], b[1], b[2], b[3]); return String(t); }
};

class WiFiClient {};

class ESP8266WiFiClass {
 public:
  String ssid = "Simulator Wi-Fi";
  int status() { return WL_CONNECTED; }
  bool isConnected() { return true; }
  void mode(int) {}
  void setSleepMode(int) {}
  void begin(const char *s = nullptr, const char * = nullptr) { if (s && *s) ssid = s; }
  void disconnect() {}
  long RSSI() { return -52; }
  int RSSI(int i) { return -45 - i * 9; }
  String SSID() { return ssid; }
  String SSID(int i) { const char *n[] = { "Home Wi-Fi", "Office", "Cafe Guest" }; return String(n[i % 3]); }
  IPAddress localIP() { return IPAddress(192, 168, 1, 42); }
  int scanNetworks() { return 3; }
  void scanDelete() {}
  bool softAP(const char *, const char *) { return true; }
  IPAddress softAPIP() { return IPAddress(192, 168, 4, 1); }
  bool softAPdisconnect(bool) { return true; }
};
extern ESP8266WiFiClass WiFi;
