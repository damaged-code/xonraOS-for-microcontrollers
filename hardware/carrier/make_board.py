# Builds the xonraOS carrier board (v0.3) as a KiCad PCB: the low-cost version of the board.
# A ready-made Wemos D1 mini (ESP8266 + USB) is soldered flat onto it; everything else is through-hole or big pads,
# so it can be soldered by hand and ordered as a bare board (no factory assembly).
# Run with KiCad's own Python:  "<KiCad>\bin\python.exe" make_board.py
# Then route.py (Freerouting), refill zones with kicad-cli, and export_jlc.py for the factory files.
#
# Same 60 x 88 mm outline, screen header, buttons and wheel as the v0.2 board, so it fits the printed case
# (hardware/case, built for this board). Board coordinates: mm from the bottom-left corner, y up.
# License: CERN-OHL-S-2.0
import os
import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
KLIB = os.environ.get("KICAD_FP", r"C:\Users\moham\AppData\Local\Programs\KiCad\10.0\share\kicad\footprints")
LOCAL = os.path.join(HERE, "..", "pcb", "xonraOS.pretty")
W, D = 60.0, 88.0
OX, OY = 100.0, 100.0

# The D1 mini: centre position, and the cutout under it (room for the CH340 and regulator on its underside)
D1X, D1Y = 17.1, 14.5  # module edge on the board edge, so a micro-USB plug reaches it through the case wall
CUT = (5.0, D1Y - 8.2, 33.4, D1Y + 8.2)  # x0, y0, x1, y1


def P(x, y):  # board coords (y up) -> KiCad (y down), in internal units
    return pcbnew.VECTOR2I(pcbnew.FromMM(OX + x), pcbnew.FromMM(OY + D - y))


# ref: (library, footprint, x, y, rotation (deg, CCW), value, LCSC part)  (no LCSC parts: nothing is factory-assembled)
PARTS = {
    "U1":  ("xonraOS", "WEMOS_D1_mini_flat", D1X, D1Y, 0, "Wemos D1 mini", ""),
    "J2":  ("Connector_PinHeader_2.54mm", "PinHeader_1x08_P2.54mm_Vertical", 2.5, 69.25, 0, "TFT 1.8in ST7735", ""),
    "SW1": ("Button_Switch_THT", "SW_PUSH-12mm", 41, 35.2, 0, "Main (D3)", ""),
    "SW2": ("Button_Switch_THT", "SW_PUSH-12mm", 19, 35.2, 0, "Second (D6)", ""),
    "LS1": ("xonraOS", "Buzzer_5x5_SMD", 45, 15, 0, "Buzzer 5x5", ""),
    "R1":  ("Resistor_THT", "R_Axial_DIN0204_L3.6mm_D1.6mm_P7.62mm_Horizontal", 45, 21.5, 0, "220R", ""),
    "RV1": ("xonraOS", "Thumbwheel_16mm", 56, 38, 0, "10k thumbwheel", ""),
    "H1":  ("MountingHole", "MountingHole_2.2mm_M2", 3, 31, 0, "M2", ""),
    "H2":  ("MountingHole", "MountingHole_2.2mm_M2", 57, 3, 0, "M2", ""),
    "H3":  ("MountingHole", "MountingHole_2.2mm_M2", 3, 46, 0, "M2", ""),
}

# net: list of (ref, pad). Pins match firmware/xonraOS_TFT (D1 mini pin names).
NETS = {
    "GND":      [("U1", "G"), ("SW1", "2"), ("SW2", "2"), ("RV1", "3"), ("J2", "2"), ("LS1", "2")],
    "+3V3":     [("U1", "3V3"), ("RV1", "1"), ("J2", "1"), ("J2", "8")],
    "TFT_CS":   [("U1", "D1"), ("J2", "3")],
    "RST":      [("U1", "RST"), ("J2", "4")],
    "TFT_DC":   [("U1", "D2"), ("J2", "5")],
    "TFT_MOSI": [("U1", "D7"), ("J2", "6")],
    "TFT_SCK":  [("U1", "D5"), ("J2", "7")],
    "BTN1":     [("U1", "D3"), ("SW1", "1")],
    "BTN2":     [("U1", "D6"), ("SW2", "1")],
    "BUZ_DRV":  [("U1", "D8"), ("R1", "1")],
    "BUZ":      [("R1", "2"), ("LS1", "1")],
    "WIPER":    [("U1", "A0"), ("RV1", "2")],  # the D1 mini already has the divider for its A0 pin
}
POWER = {"GND", "+3V3"}


def write_project():
    import json
    def nc(name, width, via, drill, prio):
        return {"name": name, "clearance": 0.2, "track_width": width, "via_diameter": via, "via_drill": drill,
                "microvia_diameter": 0.3, "microvia_drill": 0.1, "diff_pair_width": 0.2, "diff_pair_gap": 0.25,
                "diff_pair_via_gap": 0.25, "wire_width": 6, "bus_width": 12, "line_style": 0, "pcb_color": "rgba(0, 0, 0, 0.000)",
                "schematic_color": "rgba(0, 0, 0, 0.000)", "priority": prio}
    pro = {
        "board": {"design_settings": {"rules": {
            "min_clearance": 0.2, "min_copper_edge_clearance": 0.5, "min_track_width": 0.2,
            "min_via_diameter": 0.6, "min_via_annular_width": 0.13, "min_through_hole_diameter": 0.3,
            "min_hole_clearance": 0.25, "min_hole_to_hole": 0.25, "min_silk_clearance": 0.0}}},
        "net_settings": {
            "classes": [nc("Default", 0.3, 0.7, 0.35, 2147483647), nc("Power", 0.5, 0.8, 0.4, 0)],
            "meta": {"version": 4},
            "netclass_assignments": None,
            "netclass_patterns": [{"netclass": "Power", "pattern": n} for n in sorted(POWER)],
        },
        "meta": {"filename": "xonraOS.kicad_pro", "version": 3},
    }
    with open(os.path.join(HERE, "xonraOS.kicad_pro"), "w") as f:
        json.dump(pro, f, indent=2)


def load(lib, name):
    path = LOCAL if lib == "xonraOS" else os.path.join(KLIB, lib + ".pretty")
    fp = pcbnew.FootprintLoad(path, name)
    if fp is None:
        raise RuntimeError("footprint not found: %s:%s" % (lib, name))
    return fp


def pad_center(fp):
    xs, ys = [], []
    for p in fp.Pads():
        bb = p.GetBoundingBox()
        xs += [bb.GetLeft(), bb.GetRight()]
        ys += [bb.GetTop(), bb.GetBottom()]
    if not xs:
        return pcbnew.VECTOR2I(0, 0)
    pos = fp.GetPosition()
    return pcbnew.VECTOR2I((min(xs) + max(xs)) // 2 - pos.x, (min(ys) + max(ys)) // 2 - pos.y)


def main():
    board = pcbnew.BOARD()
    board.GetDesignSettings().SetCopperLayerCount(2)

    nets = {}
    for name in NETS:
        ni = pcbnew.NETINFO_ITEM(board, name)
        board.Add(ni)
        nets[name] = ni

    fps = {}
    for ref, (lib, name, x, y, rot, value, lcsc) in PARTS.items():
        fp = load(lib, name)
        fp.SetReference(ref)
        fp.SetValue(value)
        board.Add(fp)
        fp.SetOrientationDegrees(rot)
        fp.SetPosition(P(0, 0))
        off = pad_center(fp)
        target = P(x, y)
        if ref in ("U1", "RV1"):  # origin = module centre / wheel axis
            fp.SetPosition(target)
        elif ref == "J2":  # header: pin 1 (VCC) at the top, 8 pins going down
            fp.SetPosition(P(x, y + 3.5 * 2.54))
        else:
            fp.SetPosition(pcbnew.VECTOR2I(target.x - off.x, target.y - off.y))
        fp.Reference().SetTextSize(pcbnew.VECTOR2I(pcbnew.FromMM(1.0), pcbnew.FromMM(1.0)))
        fp.Reference().SetTextThickness(pcbnew.FromMM(0.15))
        fp.Value().SetVisible(False)
        fps[ref] = fp

    for net, members in NETS.items():
        for ref, padnum in members:
            found = [p for p in fps[ref].Pads() if p.GetNumber() == padnum]
            if not found:
                raise RuntimeError("no pad %s on %s" % (padnum, ref))
            for p in found:
                p.SetNet(nets[net])

    def seg(a, b):
        s = pcbnew.PCB_SHAPE(board); s.SetShape(pcbnew.SHAPE_T_SEGMENT)
        s.SetStart(a); s.SetEnd(b); s.SetLayer(pcbnew.Edge_Cuts); s.SetWidth(pcbnew.FromMM(0.1)); board.Add(s)
    def arc(c, start, angle):
        s = pcbnew.PCB_SHAPE(board); s.SetShape(pcbnew.SHAPE_T_ARC)
        s.SetCenter(c); s.SetStart(start); s.SetArcAngleAndEnd(pcbnew.EDA_ANGLE(angle, pcbnew.DEGREES_T), True)
        s.SetLayer(pcbnew.Edge_Cuts); s.SetWidth(pcbnew.FromMM(0.1)); board.Add(s)
    def rounded(x0, y0, x1, y1, r):
        seg(P(x0 + r, y0), P(x1 - r, y0)); seg(P(x1, y0 + r), P(x1, y1 - r)); seg(P(x1 - r, y1), P(x0 + r, y1)); seg(P(x0, y1 - r), P(x0, y0 + r))
        arc(P(x1 - r, y0 + r), P(x1 - r, y0), -90); arc(P(x1 - r, y1 - r), P(x1, y1 - r), -90)
        arc(P(x0 + r, y1 - r), P(x0 + r, y1), -90); arc(P(x0 + r, y0 + r), P(x0, y0 + r), -90)
    rounded(0, 0, W, D, 1.5)  # outline
    rounded(*CUT, 1.0)        # cutout under the D1 mini

    def rect_outline(x0, y0, x1, y1):
        o = pcbnew.SHAPE_LINE_CHAIN()
        for (x, y) in [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]:
            o.Append(P(x, y))
        o.SetClosed(True)
        return o
    ls = pcbnew.LSET(); ls.AddLayer(pcbnew.F_Cu); ls.AddLayer(pcbnew.B_Cu)
    for layer in (pcbnew.F_Cu, pcbnew.B_Cu):  # ground pour on both layers
        z = pcbnew.ZONE(board)
        z.SetLayer(layer); z.SetNet(nets["GND"])
        z.Outline().AddOutline(rect_outline(0.3, 0.3, W - 0.3, D - 0.3))
        z.SetLocalClearance(pcbnew.FromMM(0.3)); z.SetMinThickness(pcbnew.FromMM(0.25))
        z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
        board.Add(z)

    def keepout(x0, y0, x1, y1, fills_ok=False, hole=None):
        k = pcbnew.ZONE(board)
        k.SetIsRuleArea(True)
        k.SetDoNotAllowTracks(True); k.SetDoNotAllowVias(True); k.SetDoNotAllowZoneFills(not fills_ok)
        k.SetDoNotAllowPads(False); k.SetDoNotAllowFootprints(False)
        k.SetLayerSet(ls)
        k.Outline().AddOutline(rect_outline(x0, y0, x1, y1))
        if hole:
            k.Outline().AddHole(rect_outline(*hole))
        board.Add(k)
    keepout(25.8, 0.5, 35.5, 26.8)                                   # no copper under the D1 mini's antenna
    keepout(CUT[0] - 0.5, CUT[1] - 0.5, CUT[2] + 0.5, CUT[3] + 0.5, True)  # tracks 0.5 mm from the cutout
    keepout(-1, -1, W + 1, D + 1, True, (0.5, 0.5, W - 0.5, D - 0.5))  # tracks 0.5 mm from the board edge

    def text(s, x, y, size, layer, mirror=False):
        t = pcbnew.PCB_TEXT(board); t.SetText(s); t.SetPosition(P(x, y)); t.SetLayer(layer)
        t.SetTextSize(pcbnew.VECTOR2I(pcbnew.FromMM(size), pcbnew.FromMM(size))); t.SetTextThickness(pcbnew.FromMM(size * 0.15))
        if mirror: t.SetMirrored(True)
        board.Add(t)
    text("xonraOS carrier v0.3", 30, 47.5, 1.5, pcbnew.F_SilkS)
    text("TFT: 1 VCC 2 GND 3 CS 4 RST 5 DC 6 SDA 7 SCK 8 LED", 30, 85.5, 0.8, pcbnew.B_SilkS, True)
    text("D1 mini: solder flat, plastic spacer removed, USB to the left edge", 30, 49.5, 0.8, pcbnew.B_SilkS, True)
    text("github.com/damaged-code/xonraOS-for-microcontrollers  CERN-OHL-S-2.0", 30, 47.5, 0.8, pcbnew.B_SilkS, True)

    out = os.path.join(HERE, "xonraOS.kicad_pcb")
    write_project()
    board.Save(out)
    print("saved", out, "parts", len(fps), "nets", len(nets))


if __name__ == "__main__":
    main()
