#pragma once
#include "ESP8266WiFi.h"
class ESP8266HTTPUpdate {
 public:
  void rebootOnUpdate(bool) {}
  int update(WiFiClient &, const String &) { return -1; }
  String getLastErrorString() { return String("No updates in simulator"); }
};
extern ESP8266HTTPUpdate ESPhttpUpdate;
