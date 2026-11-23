#!/usr/bin/env bash
# Builds the xonraOS desktop simulator (Windows, MSYS2 g++). Usage:
#   bash tools/simulator/build.sh            -> tools/simulator/build/xonraos-sim.exe
# Needs the Arduino libraries "Adafruit GFX Library" and "elk" installed (Arduino IDE / arduino-cli).
# Set ARDUINO_LIBS if they are not in Documents/Arduino/libraries.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
SKETCH="${SKETCH:-$REPO/firmware/xonraOS_TFT/xonraOS_TFT.ino}"
for d in "$ARDUINO_LIBS" "$USERPROFILE/OneDrive/Documents/Arduino/libraries" "$USERPROFILE/Documents/Arduino/libraries" "$HOME/Documents/Arduino/libraries"; do
  if [ -n "$d" ] && [ -d "$d/Adafruit_GFX_Library" ]; then LIBS="$d"; break; fi
done
[ -n "$LIBS" ] || { echo "Arduino libraries not found: set ARDUINO_LIBS"; exit 1; }
GFX="$LIBS/Adafruit_GFX_Library"
ELK="$LIBS/elk"; [ -f "$ELK/elk.c" ] || ELK="$LIBS/elk/src"
B="$HERE/build"
mkdir -p "$B"

# Simulator-only settings (your real secrets.h is never used here)
cat > "$B/secrets.h" <<'EOF'
#define WIFI_NAME     "Simulator Wi-Fi"
#define WIFI_PASSWORD ""
#define STORE_URL     "http://xonra.sim"
EOF
# The sketch, as a C++ file with the Arduino header first (the Arduino IDE adds it the same way)
{ echo '#include "Arduino.h"'; cat "$SKETCH"; } > "$B/sketch.cpp"

SERVER_DIR="$(cygpath -m "$REPO/server" 2>/dev/null || echo "$REPO/server")"
CXXFLAGS="-std=gnu++17 -O2 -w -I$HERE/include -I$B -I$GFX -I$ELK"
gcc -O1 -w -c "$ELK/elk.c" -I"$ELK" -o "$B/elk.o"
g++ $CXXFLAGS -c "$GFX/Adafruit_GFX.cpp" -o "$B/gfx.o"
g++ $CXXFLAGS -c "$B/sketch.cpp" -o "$B/sketch.o"
g++ $CXXFLAGS -DSIM_SERVER_DIR="\"$SERVER_DIR\"" -c "$HERE/sim.cpp" -o "$B/sim.o"
g++ -o "$B/xonraos-sim.exe" "$B/sim.o" "$B/sketch.o" "$B/gfx.o" "$B/elk.o" -static -lgdi32 -luser32
echo "built $B/xonraos-sim.exe"
