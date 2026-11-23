// Simulated phone-setup web server: it accepts the calls but nothing can connect to it.
#pragma once
#include "ESP8266WiFi.h"
enum HTTPMethod { HTTP_ANY, HTTP_GET, HTTP_POST };
class ESP8266WebServer {
 public:
  ESP8266WebServer(int) {}
  void on(const char *, std::function<void()>) {}
  void on(const char *, int, std::function<void()>) {}
  void onNotFound(std::function<void()>) {}
  void begin() {}
  void stop() {}
  void handleClient() {}
  void send(int, const char *, const String &) {}
  void sendHeader(const String &, const String &) {}
  String arg(const char *) { return String(); }
};
