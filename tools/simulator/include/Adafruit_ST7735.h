// Simulated 1.8" ST7735 TFT (160 x 128 in landscape): pixels go to the simulator window.
#pragma once
#include "Adafruit_GFX.h"
#define INITR_GREENTAB 0x00
#define INITR_REDTAB 0x01
#define INITR_BLACKTAB 0x02
#define ST77XX_BLACK 0x0000
#define ST77XX_WHITE 0xFFFF
#define ST77XX_RED 0xF800
#define ST77XX_GREEN 0x07E0
#define ST77XX_BLUE 0x001F
#define ST77XX_CYAN 0x07FF
#define ST77XX_MAGENTA 0xF81F
#define ST77XX_YELLOW 0xFFE0
#define ST77XX_ORANGE 0xFC00

class Adafruit_ST7735 {
 public:
  Adafruit_ST7735(int8_t, int8_t, int8_t) {}
  void initR(uint8_t);
  void setRotation(uint8_t);
  void setSPISpeed(uint32_t) {}
  void fillScreen(uint16_t c);
  void startWrite() {}
  void endWrite();
  void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
  void writePixels(uint16_t *colors, uint32_t len, bool block = true, bool bigEndian = false);
 private:
  int ax = 0, ay = 0, aw = 160, ah = 128, cur = 0;
};
