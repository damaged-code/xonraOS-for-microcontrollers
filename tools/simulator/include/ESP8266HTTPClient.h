// Simulated HTTP: a GET for <STORE_URL>/<path> reads <server folder>/<path> from this repository.
#pragma once
#include "ESP8266WiFi.h"
class HTTPClient {
 public:
  String url, body;
  int code = 0;
  void setTimeout(int) {}
  void useHTTP10(bool) {}
  bool begin(WiFiClient &, const String &u) { url = u; return true; }
  int GET();
  int getSize() { return (int)body.length(); }
  String getString() { return body; }
  void end() {}
};
