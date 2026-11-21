#!/usr/bin/env python3
"""Build catalog.json (used by index.html) from manifest.txt and apps/*.js.

Usage:  python3 build-catalog.py [folder]      (default: the folder this script is in)

The website loads this one file instead of fetching every app separately, which is much faster.
The board's Store does not use it: it keeps using manifest.txt and apps/<id>.js.

Manifest lines are:  id|Name|version|Category
The category is optional (missing or unknown means "Other"). Apps are written to
catalog.json grouped in the category order below, so the website can show sections.
"""
import json
import os
import re
import sys

CATS = ["Tools", "Games", "Timers", "Sound", "Other"]

root = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))
apps = []
with open(os.path.join(root, "manifest.txt"), encoding="utf-8") as f:
    for line in f:
        parts = [p.strip() for p in line.strip().split("|")]
        if len(parts) < 3 or not re.fullmatch(r"[a-z0-9]+", parts[0]):
            continue
        path = os.path.join(root, "apps", parts[0] + ".js")
        if not os.path.isfile(path):
            continue
        with open(path, encoding="utf-8") as a:
            code = a.read()
        cat = parts[3] if len(parts) > 3 and parts[3] in CATS else "Other"
        apps.append({"id": parts[0], "name": parts[1], "ver": parts[2], "cat": cat, "code": code})

order = {c: i for i, c in enumerate(CATS)}
apps.sort(key=lambda a: order[a["cat"]])

with open(os.path.join(root, "catalog.json"), "w", encoding="utf-8") as out:
    json.dump({"apps": apps}, out, ensure_ascii=False)
print("catalog.json:", len(apps), "apps")
