#pragma once
#include "ESP8266WiFi.h"
class DNSServer {
 public:
  bool start(int, const char *, const IPAddress &) { return true; }
  void processNextRequest() {}
  void stop() {}
};
