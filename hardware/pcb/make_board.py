# Builds the xonraOS board (v0.2, 1.8" TFT, portrait 60 x 88 mm) as a KiCad PCB.
# Run with KiCad's own Python:  "<KiCad>\bin\python.exe" make_board.py
# It writes xonraOS.kicad_pcb (parts placed, nets assigned, outline, ground pours, antenna keep-out).
# Then route.cmd routes it with Freerouting and exports the factory files.
#
# The circuit (every net) is the NETS table below. Part positions match hardware/case/xonra-board-case.js
# so the board fits the printed case. Board coordinates: mm from the bottom-left corner, y up.
# License: CERN-OHL-S-2.0
import os
import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
KLIB = os.environ.get("KICAD_FP", r"C:\Users\moham\AppData\Local\Programs\KiCad\10.0\share\kicad\footprints")
LOCAL = os.path.join(HERE, "xonraOS.pretty")
W, D = 60.0, 88.0
OX, OY = 100.0, 100.0  # where the board sits on the KiCad page

def P(x, y):  # board coords (y up) -> KiCad (y down), in internal units
    return pcbnew.VECTOR2I(pcbnew.FromMM(OX + x), pcbnew.FromMM(OY + D - y))

# ref: (library, footprint, x, y, rotation (deg, CCW), value, LCSC part, side)
PARTS = {
    "U1":  ("RF_Module", "ESP-12E", 46.9, 14.8, -90, "ESP-12F", "C82891"),
    "U2":  ("Package_SO", "SOIC-16_3.9x9.9mm_P1.27mm", 7, 37, 0, "CH340C", "C84681"),
    "U3":  ("Package_TO_SOT_SMD", "SOT-223-3_TabPin2", 29.5, 15.5, 0, "AMS1117-3.3", "C6186"),
    "J1":  ("Connector_USB", "USB_C_Receptacle_HRO_TYPE-C-31-M-12", 14, 3.2, 0, "USB-C", "C165948"),
    "J2":  ("Connector_PinHeader_2.54mm", "PinHeader_1x08_P2.54mm_Vertical", 2.5, 69.25, 0, "TFT 1.8in ST7735", ""),
    "SW1": ("Button_Switch_THT", "SW_PUSH-12mm", 41, 31.5, 0, "Main (GPIO0)", ""),
    "SW2": ("Button_Switch_THT", "SW_PUSH-12mm", 19, 31.5, 0, "Second (GPIO12)", ""),
    "SW3": ("Button_Switch_SMD", "SW_Push_1P1T_NO_CK_KMR2", 20, 14, 0, "Reset", "C72443"),
    "LS1": ("xonraOS", "Buzzer_5x5_SMD", 10, 18, 0, "Buzzer 5x5", ""),
    "RV1": ("xonraOS", "Thumbwheel_16mm", 56, 38, 0, "10k thumbwheel", ""),
    "Q1":  ("Package_TO_SOT_SMD", "SOT-23", 28, 46, 0, "S8050", "C2146"),
    "Q2":  ("Package_TO_SOT_SMD", "SOT-23", 33, 46, 0, "S8050", "C2146"),
    "Q3":  ("Package_TO_SOT_SMD", "SOT-23", 5, 12, 0, "S8050", "C2146"),
    "D1":  ("Diode_SMD", "D_SOD-123", 10, 12.5, 0, "1N4148W", "C81598"),
    "D2":  ("LED_SMD", "LED_0603_1608Metric", 24, 3, 0, "LED", "C12624"),
    "R1":  ("Resistor_SMD", "R_0603_1608Metric", 50.5, 25.5, 0, "10k", "C25804"),
    "R2":  ("Resistor_SMD", "R_0603_1608Metric", 53.5, 25.5, 0, "10k", "C25804"),
    "R3":  ("Resistor_SMD", "R_0603_1608Metric", 56.5, 25.5, 0, "10k", "C25804"),
    "R4":  ("Resistor_SMD", "R_0603_1608Metric", 50.5, 28, 0, "10k", "C25804"),
    "R5":  ("Resistor_SMD", "R_0603_1608Metric", 53.5, 28, 0, "10k", "C25804"),
    "R6":  ("Resistor_SMD", "R_0603_1608Metric", 6.5, 8.5, 90, "5.1k", "C23186"),
    "R7":  ("Resistor_SMD", "R_0603_1608Metric", 21, 8.5, 90, "5.1k", "C23186"),
    "R8":  ("Resistor_SMD", "R_0603_1608Metric", 28, 42.5, 0, "10k", "C25804"),
    "R9":  ("Resistor_SMD", "R_0603_1608Metric", 33, 42.5, 0, "10k", "C25804"),
    "R10": ("Resistor_SMD", "R_0603_1608Metric", 5, 16, 0, "1k", "C21190"),
    "R11": ("Resistor_SMD", "R_0603_1608Metric", 51, 30.5, 0, "220k", "C22961"),
    "R12": ("Resistor_SMD", "R_0603_1608Metric", 55, 30.5, 0, "100k", "C25803"),
    "R14": ("Resistor_SMD", "R_0603_1608Metric", 27, 3, 0, "1k", "C21190"),
    "C1":  ("Capacitor_SMD", "C_0805_2012Metric", 29.5, 9.2, 0, "10uF", "C15850"),
    "C2":  ("Capacitor_SMD", "C_0805_2012Metric", 23.5, 21, 0, "10uF", "C15850"),
    "C3":  ("Capacitor_SMD", "C_0805_2012Metric", 32.5, 22.8, 0, "10uF", "C15850"),
    "C4":  ("Capacitor_SMD", "C_0603_1608Metric", 51, 33, 0, "100nF", "C14663"),
    "C5":  ("Capacitor_SMD", "C_0603_1608Metric", 7, 29.3, 90, "100nF", "C14663"),
    "C6":  ("Capacitor_SMD", "C_0603_1608Metric", 6, 62, 90, "100nF", "C14663"),
    "C7":  ("Capacitor_SMD", "C_0603_1608Metric", 56.5, 28, 0, "100nF", "C14663"),
    "H1":  ("MountingHole", "MountingHole_2.2mm_M2", 3, 3, 0, "M2", ""),
    "H2":  ("MountingHole", "MountingHole_2.2mm_M2", 57, 3, 0, "M2", ""),
    "H3":  ("MountingHole", "MountingHole_2.2mm_M2", 3, 46, 0, "M2", ""),
}

# net: list of (ref, pad)
NETS = {
    "GND": [("J1", "A1"), ("J1", "A12"), ("J1", "B1"), ("J1", "B12"), ("J1", "SH"), ("U3", "1"), ("U1", "15"), ("U2", "1"),
            ("R5", "2"), ("R6", "2"), ("R7", "2"), ("R12", "2"), ("Q3", "2"), ("SW1", "2"), ("SW2", "2"), ("SW3", "2"),
            ("D2", "1"), ("RV1", "3"), ("J2", "2"), ("C1", "2"), ("C2", "2"), ("C3", "2"), ("C4", "2"), ("C5", "2"), ("C6", "2"), ("C7", "2")],
    "VBUS": [("J1", "A4"), ("J1", "A9"), ("J1", "B4"), ("J1", "B9"), ("U3", "3"), ("C1", "1")],
    "+3V3": [("U3", "2"), ("U1", "8"), ("U2", "4"), ("U2", "16"), ("R1", "1"), ("R2", "1"), ("R3", "1"), ("R4", "1"), ("R14", "1"),
             ("LS1", "1"), ("D1", "1"), ("RV1", "1"), ("J2", "1"), ("J2", "8"), ("C2", "1"), ("C3", "1"), ("C4", "1"), ("C5", "1"), ("C6", "1")],
    "USB_DP": [("J1", "A6"), ("J1", "B6"), ("U2", "5")],
    "USB_DM": [("J1", "A7"), ("J1", "B7"), ("U2", "6")],
    "CC1": [("J1", "A5"), ("R6", "1")],
    "CC2": [("J1", "B5"), ("R7", "1")],
    "ESP_RX": [("U2", "2"), ("U1", "21")],   # CH340 TXD -> ESP RXD
    "ESP_TX": [("U2", "3"), ("U1", "22")],   # ESP TXD -> CH340 RXD
    "DTR": [("U2", "13"), ("R8", "1"), ("Q2", "2")],
    "RTS": [("U2", "14"), ("R9", "1"), ("Q1", "2")],
    "Q1_B": [("R8", "2"), ("Q1", "1")],
    "Q2_B": [("R9", "2"), ("Q2", "1")],
    "EN": [("U1", "3"), ("R1", "2"), ("C7", "1"), ("Q1", "3")],
    "RST": [("U1", "1"), ("R2", "2"), ("SW3", "1"), ("J2", "4")],
    "GPIO0": [("U1", "18"), ("R3", "2"), ("Q2", "3"), ("SW1", "1")],
    "GPIO2": [("U1", "17"), ("R4", "2")],
    "GPIO15": [("U1", "16"), ("R5", "1"), ("R10", "1")],
    "Q3_B": [("R10", "2"), ("Q3", "1")],
    "BUZ_N": [("Q3", "3"), ("LS1", "2"), ("D1", "2")],
    "TFT_CS": [("U1", "20"), ("J2", "3")],    # GPIO5
    "TFT_DC": [("U1", "19"), ("J2", "5")],    # GPIO4
    "TFT_MOSI": [("U1", "7"), ("J2", "6")],   # GPIO13
    "TFT_SCK": [("U1", "5"), ("J2", "7")],    # GPIO14
    "BTN2": [("U1", "6"), ("SW2", "1")],      # GPIO12
    "TOUT": [("U1", "2"), ("R11", "2"), ("R12", "1")],
    "WIPER": [("RV1", "2"), ("R11", "1")],
    "LED_A": [("R14", "2"), ("D2", "2")],
}
POWER = {"GND", "VBUS", "+3V3"}


def write_project():
    """Design rules and net classes live in the .kicad_pro file in KiCad 10 (JLCPCB-friendly values)."""
    import json
    def nc(name, width, via, drill, prio):
        return {"name": name, "clearance": 0.2, "track_width": width, "via_diameter": via, "via_drill": drill,
                "microvia_diameter": 0.3, "microvia_drill": 0.1, "diff_pair_width": 0.2, "diff_pair_gap": 0.25,
                "diff_pair_via_gap": 0.25, "wire_width": 6, "bus_width": 12, "line_style": 0, "pcb_color": "rgba(0, 0, 0, 0.000)",
                "schematic_color": "rgba(0, 0, 0, 0.000)", "priority": prio}
    pro = {
        "board": {"design_settings": {"rules": {
            "min_clearance": 0.2, "min_copper_edge_clearance": 0.5, "min_track_width": 0.12,
            "min_via_diameter": 0.5, "min_via_annular_width": 0.1, "min_through_hole_diameter": 0.3,
            "min_hole_clearance": 0.25, "min_hole_to_hole": 0.25, "min_silk_clearance": 0.0}}},
        "net_settings": {
            "classes": [nc("Default", 0.25, 0.6, 0.3, 2147483647), nc("Power", 0.5, 0.8, 0.4, 0)],
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


def pad_center(fp):  # centre of the pads' bounding box, relative to the footprint position
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
    ds = board.GetDesignSettings()
    ds.SetCopperLayerCount(2)
    ns = ds.m_NetSettings
    dflt = ns.GetDefaultNetclass()
    dflt.SetTrackWidth(pcbnew.FromMM(0.25)); dflt.SetClearance(pcbnew.FromMM(0.2))
    dflt.SetViaDiameter(pcbnew.FromMM(0.6)); dflt.SetViaDrill(pcbnew.FromMM(0.3))
    power = pcbnew.NETCLASS("Power")
    power.SetTrackWidth(pcbnew.FromMM(0.5)); power.SetClearance(pcbnew.FromMM(0.2))
    power.SetViaDiameter(pcbnew.FromMM(0.8)); power.SetViaDrill(pcbnew.FromMM(0.4))
    ns.SetNetclass("Power", power)
    for n in POWER:
        ns.SetNetclassPatternAssignment(n, "Power")

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
        if lcsc:
            fp.SetField("LCSC", lcsc)
            for f in fp.GetFields():
                if f.GetName() == "LCSC":
                    f.SetVisible(False)
        board.Add(fp)
        fp.SetOrientationDegrees(rot)
        fp.SetPosition(P(0, 0))
        off = pad_center(fp)
        target = P(x, y)
        if ref == "J1":  # USB-C: put the receptacle's front at the bottom board edge
            fp.SetPosition(pcbnew.VECTOR2I(target.x, P(0, 0).y - pcbnew.FromMM(3.6)))
        elif ref == "U1":  # ESP: centre the whole module outline (antenna included), not just its pads
            cy = fp.GetCourtyard(pcbnew.F_CrtYd).BBox()
            pos = fp.GetPosition()
            fp.SetPosition(pcbnew.VECTOR2I(target.x - (cy.GetCenter().x - pos.x), target.y - (cy.GetCenter().y - pos.y)))
        elif ref == "RV1":  # origin is the wheel axis (must match WHEEL.x in the case script)
            fp.SetPosition(target)
        elif ref == "J2":  # header: pin 1 (VCC) at the top, 8 pins going down
            fp.SetPosition(P(x, y + 3.5 * 2.54))
        else:
            fp.SetPosition(pcbnew.VECTOR2I(target.x - off.x, target.y - off.y))
        fp.Reference().SetTextSize(pcbnew.VECTOR2I(pcbnew.FromMM(0.8), pcbnew.FromMM(0.8)))
        fp.Reference().SetTextThickness(pcbnew.FromMM(0.12))
        fp.Value().SetVisible(False)
        fps[ref] = fp

    for net, members in NETS.items():
        for ref, padnum in members:
            found = [p for p in fps[ref].Pads() if p.GetNumber() == padnum]
            if not found:
                raise RuntimeError("no pad %s on %s" % (padnum, ref))
            for p in found:
                p.SetNet(nets[net])

    # Outline with rounded corners
    def seg(a, b):
        s = pcbnew.PCB_SHAPE(board); s.SetShape(pcbnew.SHAPE_T_SEGMENT)
        s.SetStart(a); s.SetEnd(b); s.SetLayer(pcbnew.Edge_Cuts); s.SetWidth(pcbnew.FromMM(0.1)); board.Add(s)
    def arc(c, start, angle):
        s = pcbnew.PCB_SHAPE(board); s.SetShape(pcbnew.SHAPE_T_ARC)
        s.SetCenter(c); s.SetStart(start); s.SetArcAngleAndEnd(pcbnew.EDA_ANGLE(angle, pcbnew.DEGREES_T), True)
        s.SetLayer(pcbnew.Edge_Cuts); s.SetWidth(pcbnew.FromMM(0.1)); board.Add(s)
    r = 1.5
    seg(P(r, 0), P(W - r, 0)); seg(P(W, r), P(W, D - r)); seg(P(W - r, D), P(r, D)); seg(P(0, D - r), P(0, r))
    arc(P(W - r, r), P(W - r, 0), -90); arc(P(W - r, D - r), P(W, D - r), -90)
    arc(P(r, D - r), P(r, D), -90); arc(P(r, r), P(0, r), -90)

    # Ground pour on both layers, with a keep-out under the ESP antenna (right edge, x 52..60, y 4..24)
    def rect_outline(x0, y0, x1, y1):
        o = pcbnew.SHAPE_LINE_CHAIN()
        for (x, y) in [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]:
            o.Append(P(x, y))
        o.SetClosed(True)
        return o
    for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
        z = pcbnew.ZONE(board)
        z.SetLayer(layer); z.SetNet(nets["GND"])
        z.Outline().AddOutline(rect_outline(0.3, 0.3, W - 0.3, D - 0.3))
        z.SetLocalClearance(pcbnew.FromMM(0.3)); z.SetMinThickness(pcbnew.FromMM(0.25))
        z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
        board.Add(z)
    ko = pcbnew.ZONE(board)
    ko.SetIsRuleArea(True)
    ko.SetDoNotAllowTracks(True); ko.SetDoNotAllowVias(True); ko.SetDoNotAllowZoneFills(True)
    ko.SetDoNotAllowPads(False); ko.SetDoNotAllowFootprints(False)
    ls = pcbnew.LSET(); ls.AddLayer(pcbnew.F_Cu); ls.AddLayer(pcbnew.B_Cu)
    ko.SetLayerSet(ls)
    ko.Outline().AddOutline(rect_outline(52.5, 4, W, 24))
    board.Add(ko)

    # Silkscreen
    def text(s, x, y, size, layer, mirror=False):
        t = pcbnew.PCB_TEXT(board); t.SetText(s); t.SetPosition(P(x, y)); t.SetLayer(layer)
        t.SetTextSize(pcbnew.VECTOR2I(pcbnew.FromMM(size), pcbnew.FromMM(size))); t.SetTextThickness(pcbnew.FromMM(size * 0.15))
        if mirror: t.SetMirrored(True)
        board.Add(t)
    text("xonraOS v0.2", 30, 49, 1.5, pcbnew.F_SilkS)
    text("TFT: 1 VCC 2 GND 3 CS 4 RST 5 DC 6 SDA 7 SCK 8 LED", 30, 85.5, 0.8, pcbnew.B_SilkS, True)
    text("github.com/damaged-code/xonraOS-for-microcontrollers  CERN-OHL-S-2.0", 30, 50, 0.8, pcbnew.B_SilkS, True)

    # Keep tracks and vias 0.5 mm away from the board edge (pads like the USB-C shell may still sit there)
    ring = pcbnew.ZONE(board)
    ring.SetIsRuleArea(True)
    ring.SetDoNotAllowTracks(True); ring.SetDoNotAllowVias(True)
    ring.SetDoNotAllowZoneFills(False); ring.SetDoNotAllowPads(False); ring.SetDoNotAllowFootprints(False)
    ring.SetLayerSet(ls)
    ring.Outline().AddOutline(rect_outline(-1, -1, W + 1, D + 1))
    ring.Outline().AddHole(rect_outline(0.5, 0.5, W - 0.5, D - 0.5))
    board.Add(ring)

    out = os.path.join(HERE, "xonraOS.kicad_pcb")
    write_project()
    board.Save(out)
    print("saved", out, "parts", len(fps), "nets", len(nets))


if __name__ == "__main__":
    main()
