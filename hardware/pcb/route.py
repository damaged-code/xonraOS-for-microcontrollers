# Routes xonraOS.kicad_pcb with Freerouting.
#   "<KiCad>\bin\python.exe" route.py <path to java.exe> <path to freerouting.jar> [passes]
# Run make_board.py first: this expects a freshly built, unrouted board.
# Freerouting 2.5 needs Java 25. Each step runs in its own KiCad Python process
# (KiCad 10's Python API misbehaves when one process loads several boards).
# Afterwards fill the ground pours:  kicad-cli pcb drc --refill-zones --save-board xonraOS.kicad_pcb
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PCB = os.path.join(HERE, "xonraOS.kicad_pcb")
PY = sys.executable
TMP = os.path.join(HERE, "_route")
DSN, SES, NOZONE = (os.path.join(TMP, n) for n in ("board.dsn", "board.ses", "nozones.kicad_pcb"))


def step(code):
    r = subprocess.run([PY, "-c", "import pcbnew\n" + code], capture_output=True, text=True)
    out = "\n".join(l for l in (r.stdout + r.stderr).splitlines() if "memory leak" not in l)
    if out.strip():
        print(out.strip())
    if r.returncode:
        sys.exit("step failed")


if __name__ == "__main__":
    java, jar = sys.argv[1], sys.argv[2]
    passes = sys.argv[3] if len(sys.argv) > 3 else "100"
    os.makedirs(TMP, exist_ok=True)
    if os.path.exists(SES):
        os.remove(SES)
    # 1. copy without copper pours (they would look like planes to the router); rule areas (keep-outs) stay
    step(f"""b = pcbnew.LoadBoard(r'{PCB}')
for z in list(b.Zones()):
    if not z.GetIsRuleArea():
        b.Remove(z)
b.Save(r'{NOZONE}')""")
    step(f"""b = pcbnew.LoadBoard(r'{NOZONE}')
assert pcbnew.ExportSpecctraDSN(b, r'{DSN}')
print('DSN written')""")
    # 2. autoroute
    r = subprocess.run([java, "-jar", jar, "-de", DSN, "-do", SES, "-mp", passes, "--gui.enabled=false"],
                       capture_output=True, text=True, timeout=3600)
    print("\n".join(l for l in r.stdout.splitlines() if "unrouted" in l or "completed" in l)[-3000:])
    if not os.path.exists(SES):
        sys.exit("no SES produced")
    # 3. bring the tracks back into the real board
    step(f"""b = pcbnew.LoadBoard(r'{PCB}')
assert pcbnew.ImportSpecctraSES(b, r'{SES}')
b.Save(r'{PCB}')
print('routed and saved')""")
