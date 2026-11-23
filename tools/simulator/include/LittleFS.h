// Simulated flash file system: files live in the simfs/ folder next to the simulator.
#pragma once
#include "Arduino.h"

struct FSInfo { size_t totalBytes = 0, usedBytes = 0, blockSize = 0, pageSize = 0, maxOpenFiles = 0, maxPathLength = 0; };

class File : public Print {
 public:
  FILE *fp = nullptr;
  File() {}
  explicit File(FILE *f) : fp(f) {}
  explicit operator bool() const { return fp != nullptr; }
  size_t write(uint8_t c) override { return fp ? fwrite(&c, 1, 1, fp) : 0; }
  size_t write(const uint8_t *b, size_t n) override { return fp ? fwrite(b, 1, n, fp) : 0; }
  using Print::write;
  int read() { return fp ? fgetc(fp) : -1; }
  size_t readBytes(char *b, size_t n) { return fp ? fread(b, 1, n, fp) : 0; }
  String readStringUntil(char t) { std::string s; int c; while (fp && (c = fgetc(fp)) != EOF && c != t) s += (char)c; return String(s); }
  String readString() { std::string s; int c; while (fp && (c = fgetc(fp)) != EOF) s += (char)c; return String(s); }
  size_t size();
  void close() { if (fp) fclose(fp); fp = nullptr; }
};

class Dir {
 public:
  std::string dir;
  std::vector<std::string> names;
  int idx = -1;
  bool next();
  String fileName();
  File openFile(const char *mode);
};

class LittleFSClass {
 public:
  bool begin();
  bool format();
  bool mkdir(const String &p);
  File open(const String &p, const char *mode);
  bool exists(const String &p);
  bool remove(const String &p);
  bool rename(const String &a, const String &b);
  Dir openDir(const String &p);
  bool info(FSInfo &i);
};
extern LittleFSClass LittleFS;
