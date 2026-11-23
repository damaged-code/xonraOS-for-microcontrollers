#!/bin/sh
# Rebuilds the carrier board: place parts, autoroute, fill pours, DRC, factory files. Run from Git Bash.
set -e
cd "$(dirname "$0")"
K="${KICAD_BIN:-$LOCALAPPDATA/Programs/KiCad/10.0/bin}"
JAVA="${JAVA25:-C:/Program Files/Eclipse Adoptium/jre-25.0.5.10002-hotspot/bin/java.exe}"
JAR="${FREEROUTING:-$USERPROFILE/tools/freerouting/freerouting.jar}"
"$K/python.exe" make_board.py 2>&1 | grep -v "memory leak"
"$K/python.exe" route.py "$JAVA" "$JAR" 2>&1 | grep -E "routed and saved|Auto-routing stage"
"$K/kicad-cli.exe" pcb drc --refill-zones --save-board --format json --severity-all -o drc-report.json xonraOS.kicad_pcb | tail -2
