# Exports JLCPCB production files from xonraOS.kicad_pcb into production/:
#   gerbers + drill (zipped), bom.csv (Comment, Designator, Footprint, LCSC) and cpl.csv (placement).
#   "<KiCad>\bin\python.exe" export_jlc.py
import csv
import os
import shutil
import subprocess
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
PCB = os.path.join(HERE, "xonraOS.kicad_pcb")
OUT = os.path.join(HERE, "production")
CLI = os.path.join(os.path.dirname(sys.executable), "kicad-cli.exe")
sys.path.insert(0, HERE)
from make_board import PARTS  # noqa: E402

gdir = os.path.join(OUT, "gerbers")
if os.path.isdir(gdir):
    shutil.rmtree(gdir)
os.makedirs(gdir)
layers = "F.Cu,B.Cu,F.Paste,F.SilkS,B.SilkS,F.Mask,B.Mask,Edge.Cuts"
subprocess.run([CLI, "pcb", "export", "gerbers", "--layers", layers, "--subtract-soldermask", "--use-drill-file-origin",
                "-o", gdir + os.sep, PCB], check=True)
subprocess.run([CLI, "pcb", "export", "drill", "--format", "excellon", "--excellon-separate-th", "--generate-map",
                "--map-format", "gerberx2", "--drill-origin", "absolute", "-o", gdir + os.sep, PCB], check=True)
with zipfile.ZipFile(os.path.join(OUT, "xonraOS-gerbers.zip"), "w", zipfile.ZIP_DEFLATED) as z:
    for f in sorted(os.listdir(gdir)):
        z.write(os.path.join(gdir, f), f)

# Placement (CPL) from KiCad, then rewritten in JLCPCB's column names
pos = os.path.join(OUT, "_pos.csv")
subprocess.run([CLI, "pcb", "export", "pos", "--format", "csv", "--units", "mm", "--side", "both",
                "--exclude-dnp", "-o", pos, PCB], check=True)
machine = {ref for ref, p in PARTS.items() if p[6]}  # parts with an LCSC number are machine-assembled
with open(pos, newline="") as f, open(os.path.join(OUT, "cpl.csv"), "w", newline="") as g:
    w = csv.writer(g)
    w.writerow(["Designator", "Mid X", "Mid Y", "Layer", "Rotation"])
    for row in csv.DictReader(f):
        if row["Ref"] in machine:
            w.writerow([row["Ref"], row["PosX"] + "mm", row["PosY"] + "mm", "Top" if row["Side"] == "top" else "Bottom", row["Rot"]])
os.remove(pos)

# BOM grouped by value + footprint + LCSC
groups = {}
for ref, (lib, fp, x, y, rot, value, lcsc) in PARTS.items():
    if lib == "MountingHole":
        continue
    groups.setdefault((value, fp, lcsc), []).append(ref)
with open(os.path.join(OUT, "bom.csv"), "w", newline="") as g:
    w = csv.writer(g)
    w.writerow(["Comment", "Designator", "Footprint", "LCSC", "Assembly"])
    for (value, fp, lcsc), refs in sorted(groups.items(), key=lambda kv: kv[1][0]):
        w.writerow([value, ",".join(sorted(refs)), fp, lcsc, "JLCPCB" if lcsc else "by hand"])
print("production files in", OUT)
