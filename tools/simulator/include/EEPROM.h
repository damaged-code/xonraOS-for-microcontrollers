// Simulated EEPROM, saved in simfs/eeprom.bin next to the simulator.
#pragma once
#include "Arduino.h"
class EEPROMClass {
 public:
  uint8_t data[4096];
  size_t size = 0;
  void begin(size_t n);
  bool commit();
  template <class T> T &get(int addr, T &v) { memcpy(&v, data + addr, sizeof(T)); return v; }
  template <class T> const T &put(int addr, const T &v) { memcpy(data + addr, &v, sizeof(T)); return v; }
};
extern EEPROMClass EEPROM;
