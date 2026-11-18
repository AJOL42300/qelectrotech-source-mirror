#!/usr/bin/env python3
# Copyright 2006-2026 The QElectroTech Team
# This file is part of QElectroTech.
#
# QElectroTech is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# QElectroTech is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
"""Write the "Description" appearance of every card that has a realistic view.

For each <card>_realistic.elmt the script writes <card>_description.elmt in the
same folder.  The description view is the realistic view (same scale, same
terminals at the same places, same reference) plus:

  * a legend with a leader line for each terminal (or for each group of
    terminals when they are too close): the pin, the signal of the I/O table
    and its function (Input n, Output n, +24 V DC, 0 V, Motor 1 phase U...);
  * the status LEDs, one per channel with a typed legend ("Input n"...), and the
    diagnostic LED of the TwinSAFE cards;
  * the power contacts (+24 V DC / 0 V) of the terminals that have them.

The legends are on the left and on the right of the drawing, outside of it.
The data (I/O table, terminals) is copied from the realistic view, never
duplicated by hand: regenerate the realistic view first, then run this
script.  The LED sets and the power contacts are generic (they are not taken
from a datasheet): see docs/beckhoff-ethercat.md.

Usage:
  gen_description.py <elements dir>   (e.g. extra-elements/10_electric/20_manufacturers_articles/beckhoff)
"""
import glob
import math
import os
import re
import sys
import uuid
import xml.etree.ElementTree as ET

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from realistic_fixes import box, num, text_width, write  # noqa: E402

NS = uuid.UUID("6f1c2a5e-8b0f-4d57-9a7e-5b1d0c8e2f10")
FONT = "Roboto Condensed,%d,-1,5,50,0,0,0,0,0,Regular"
THIN = "line-style:normal;line-weight:thin;filling:none;color:black"
LED_STYLE = "line-style:normal;line-weight:thin;filling:green;color:black"
BLACK = "line-style:normal;line-weight:thin;filling:black;color:black"
YELLOW = "line-style:normal;line-weight:normal;filling:yellow;color:black"
GAP = 8.0           # between the drawing and the legends
LEAD_GAP = 1.5      # between a circle and the start of its leader
LED_D = 4.0
LED_PITCH = 11.0
NOTE = ("Description view made by misc/beckhoff/gen_description.py from the "
        "realistic view: legends, LEDs and power contacts are generic, check "
        "them against the datasheet. License: see "
        "http://qelectrotech.org/wiki/doc/elements_license")

# Channel LEDs of the EL terminals (generic, not from a datasheet) and
# whether the card also has a diagnostic LED.
EL_LEDS = {
    "el1004": (4, False), "el1008": (8, False), "el2004": (4, False),
    "el2008": (8, False), "el2522": (2, False), "el2574": (4, False),
    "el5152": (2, False), "el9100": (0, False), "el1904": (4, True),
    "el2904": (4, True), "el2911": (5, True),
    "el1xxx_generic": (8, False), "el2xxx_generic": (8, False),
    "el3xxx_generic": (8, False), "el4xxx_generic": (8, False),
    "el2652": (4, False), "el1918": (8, True),
}
EL60 = ("el1004", "el1008", "el2004", "el2008", "el2522", "el2574", "el5152",
        "el9100", "el1904", "el2904", "el1xxx_generic", "el2xxx_generic",
        "el3xxx_generic", "el4xxx_generic", "el2652", "el1918")


def uid(*parts):
    return "{%s}" % uuid.uuid5(NS, "/".join(parts))


def line_height(size):
    return size * 4.0 / 3.0 + 1.0


def baseline_of(size, top):
    """y of the baseline of a text whose line box starts at top: QET draws
    a <text> with its baseline at y (the box is shifted up by the ascent)
    and a line is centred on the cap height."""
    return top + line_height(size) / 2.0 + 0.355 * size * 4.0 / 3.0


def twidth(text, size):
    return text_width(text) * size / 7.0


class Term:
    def __init__(self, e):
        self.e = e
        self.name = e.get("name")
        self.x = float(e.get("x"))
        self.y = float(e.get("y"))
        self.pot = e.get("potential")
        self.r = 3.0


class Legend:
    def __init__(self, side, y, x0, lines, size=7):
        self.side = side        # "L" or "R"
        self.y = y              # where the leader starts
        self.x0 = x0
        self.lines = lines
        self.size = size
        self.yc = y             # where the text is centred (after resolving)

    @property
    def height(self):
        return len(self.lines) * line_height(self.size)


class Card:
    def __init__(self, path):
        self.path = path
        self.stem = os.path.basename(path)[:-len("_realistic.elmt")]
        self.tree = ET.parse(path)
        self.root = self.tree.getroot()
        self.hx = int(self.root.get("hotspot_x"))
        self.hy = int(self.root.get("hotspot_y"))
        self.w = int(self.root.get("width"))
        self.h = int(self.root.get("height"))
        self.desc = self.root.find("description")
        self.terms = [Term(e) for e in self.desc.findall("terminal")]
        self.io = {}
        for io in self.root.iter("plcIO"):
            for t in io.findall("terminal"):
                self.io.setdefault(t.text, io)
        self.texts = []
        for e in self.desc.findall("text"):
            size = int(e.get("font").split(",")[1])
            self.texts.append({
                "x": float(e.get("x")), "y": float(e.get("y")),
                "w": twidth(e.get("text"), size), "size": size,
                "text": e.get("text"), "bold": ",75," in e.get("font")})
        self.new = []           # new graphic elements
        self.legends = []
        self.find_radii()
        self.bounds()

    def find_radii(self):
        circles = []
        for c in self.desc.findall("circle"):
            d = float(c.get("diameter"))
            circles.append((float(c.get("x")) + d / 2,
                            float(c.get("y")) + d / 2, d / 2))
        for t in self.terms:
            best = 0.0
            for cx, cy, r in circles:
                if abs(cx - t.x) < 0.8 and abs(cy - t.y) < 0.8:
                    best = max(best, r)
            t.r = best or 3.0

    def bounds(self):
        xs, ys = [], []
        for p in self.desc:
            b = box(p) if p.tag in ("line", "arc", "circle", "rect",
                                    "polygon", "ellipse") else None
            if b:
                xs += [b[0], b[2]]
                ys += [b[1], b[3]]
        for t in self.texts:
            xs += [t["x"], t["x"] + t["w"]]
            ys += [t["y"], t["y"] + line_height(t["size"])]
        for t in self.terms:
            xs += [t.x - t.r, t.x + t.r]
            ys += [t.y - t.r, t.y + t.r]
        self.minx, self.maxx = min(xs), max(xs)
        self.miny, self.maxy = min(ys), max(ys)

    # new shapes ---------------------------------------------------------
    def line(self, x1, y1, x2, y2):
        self.new.append(ET.Element("line", {
            "end1": "none", "end2": "none", "y1": num(y1),
            "antialias": "false", "y2": num(y2), "style": THIN,
            "x1": num(x1), "length1": "4.5", "x2": num(x2),
            "length2": "4.5"}))

    def circle(self, cx, cy, d, style=LED_STYLE):
        self.new.append(ET.Element("circle", {
            "x": num(cx - d / 2), "y": num(cy - d / 2), "antialias": "false",
            "style": style, "diameter": num(d)}))

    def rect(self, x, y, w, h, style):
        self.new.append(ET.Element("rect", {
            "x": num(x), "y": num(y), "width": num(w), "height": num(h),
            "antialias": "false", "style": style, "rx": "0", "ry": "0"}))

    def text(self, x, y, text, size=7):
        self.new.append(ET.Element("text", {
            "x": num(x), "y": num(y), "font": FONT % size, "rotation": "0",
            "color": "#000000", "text": text}))

    # token and function of a terminal -----------------------------------
    def label_near(self, t, used):
        best, bd = None, 9.0
        for i, lab in enumerate(self.texts):
            if i in used or lab["size"] != 7 or lab["bold"]:
                continue
            if not t.y - 34 <= lab["y"] <= t.y - 8:
                continue
            d = abs(lab["x"] + lab["w"] / 2 - t.x)
            if d < bd:
                best, bd = i, d
        if best is not None:
            used.add(best)
            return self.texts[best]["text"]
        return None

    def io_of(self, name):
        if name in self.io:
            return self.io[name]
        if self.stem == "el2911" and name.isdigit():
            return self.io.get(str((int(name) - 1) % 8 + 1))
        return None

    def label_beside(self, t):
        """Label drawn on the same row, on the outer side of the circle."""
        for lab in self.texts:
            if lab["size"] > 7 or lab["bold"] or lab["text"].startswith("Ether"):
                continue
            yc = lab["y"] + line_height(lab["size"]) / 2 - 1
            if abs(yc - t.y) <= 4 and lab["x"] + lab["w"] <= t.x:
                return lab["text"]
        return None

    def describe(self, t, label):
        io = self.io_of(t.name)
        addr = io.get("address") if io else None
        func = ""
        if addr:
            token, func = addr, function_of(addr, io, t.pot)
        elif label:
            token, func = label, function_of(label, None, t.pot)
        elif t.pot:
            token = {"0V": "0 V", "24V": "+24 V DC"}.get(t.pot, t.pot)
        else:
            token, func = t.name, function_of(t.name, None, None)
        head = token
        if t.name.isdigit() and t.name != token:
            head = "%s: %s" % (t.name, token)
        if t.name.startswith("EtherCAT"):
            return head + " (RJ45)"
        return head + (" - " + func if func else "")

def function_of(token, io, pot):
    if io is not None and io.get("functionText"):
        return io.get("functionText")
    if pot == "0V":
        return "0 V"
    if pot == "24V":
        return "+24 V DC"
    m = re.match(r"^(SI|SO|I|O|IN)(\d+)$", token)
    if m:
        return {"SI": "Safe input %s", "SO": "Safe output %s",
                "I": "Input %s", "O": "Output %s",
                "IN": "Digital input %s"}[m.group(1)] % m.group(2)
    if token == "OUT":
        return "Digital output"
    if re.match(r"^TH\d$", token):
        return "Analog input"
    m = re.match(r"^M(\d)-(U|V|W)$", token)
    if m:
        return "Motor %s, phase %s" % (m.group(1), m.group(2))
    m = re.match(r"^M(\d)-PE$", token)
    if m:
        return "Motor %s, protective earth" % m.group(1)
    if re.match(r"^L[123]N?$", token):
        return "Mains supply"
    if token == "PE":
        return "Protective earth"
    if token in ("RB+", "RB-"):
        return "Brake resistor"
    if token in ("DC+", "DC-"):
        return "DC link"
    if re.match(r"^24V_[A-C]$", token):
        return "24 V supply"
    if token.startswith("FGND"):
        return "Functional ground"
    if token.startswith("GND"):
        return "Ground"
    if pot == "0V" or token == "0 V":
        return "0 V"
    if pot == "24V" or token == "+24 V DC":
        return "+24 V DC"
    if io is not None:
        return {"entree_digitale": "Digital input",
                "sortie_digitale": "Digital output",
                "entree_analogique": "Analog input",
                "sortie_analogique": "Analog output",
                "entree_universelle": "Universal input"
                }.get(io.get("type"), "")
    return ""


# ---------------------------------------------------------------------------
# legends
def obstacle(card, t, side):
    """Where the leader of terminal t may start, on its side."""
    if side == "L":
        x = t.x - t.r - LEAD_GAP
        for lab in card.texts:
            if lab["size"] > 7 or lab["bold"] or lab["text"].startswith("Ether"):
                continue
            yc = lab["y"] + line_height(lab["size"]) / 2 - 1
            if abs(yc - t.y) <= 4 and lab["x"] + lab["w"] <= t.x:
                x = min(x, lab["x"] - LEAD_GAP)
        return x
    x = t.x + t.r + LEAD_GAP
    for lab in card.texts:
        if lab["size"] > 7 or lab["bold"] or lab["text"].startswith("Ether"):
            continue
        yc = lab["y"] + line_height(lab["size"]) / 2 - 1
        if abs(yc - t.y) <= 4 and lab["x"] >= t.x:
            x = max(x, lab["x"] + lab["w"] + LEAD_GAP)
    return x


def terminal_legends(card, centre, size, pick=None):
    """One legend per terminal, or one per group of a row when crowded."""
    used = set()
    labelled = card.stem in EL60 or card.stem in ("el2911",)
    rows = {}
    for t in card.terms:
        if pick is not None and not pick(t):
            continue
        side = "L" if t.x < centre else "R"
        rows.setdefault((round(t.y), side), []).append(t)
    for (y, side), group in sorted(rows.items()):
        group.sort(key=lambda t: t.x)
        if len(group) == 1:
            t = group[0]
            label = card.label_near(t, used) if labelled else (
                card.label_beside(t) if card.stem == "el1918" else None)
            text = card.describe(t, label)
        else:
            text = group_text(group)
        edge = group[0] if side == "L" else group[-1]
        card.legends.append(Legend(side, t_y(group), obstacle(card, edge, side),
                                   [text], size))


def t_y(group):
    return group[0].y


def group_text(group):
    names = [t.name for t in group]
    parts = [n.split("-", 1) for n in names]
    if all(len(p) == 2 for p in parts) and len({p[0] for p in parts}) == 1:
        prefix = parts[0][0]
        m = re.match(r"^M(\d)$", prefix)
        if m:
            prefix = "Motor " + m.group(1)
        return "%s: %s" % (prefix, ", ".join(p[1] for p in parts))
    return ", ".join(names)


def add_leds(card, positions, legends, size=6):
    """positions: list of (cx, cy); legends: list of (index, side, text)."""
    for cx, cy in positions:
        card.circle(cx, cy, LED_D)
    for i, side, text in legends:
        cx, cy = positions[i]
        x0 = cx - LED_D / 2 - LEAD_GAP if side == "L" \
            else cx + LED_D / 2 + LEAD_GAP
        card.legends.append(Legend(side, cy, x0, [text], size))


def led_word(stem):
    """What the channel LEDs of a card are called."""
    if stem in ("el1904", "el1918"):
        return "Safe input"
    if stem == "el2904":
        return "Safe output"
    if stem.startswith("el1"):
        return "Input"
    if stem.startswith("el2"):
        return "Output"
    if stem.startswith("el3"):
        return "Analog input"
    if stem.startswith("el4"):
        return "Analog output"
    if stem == "el5152":
        return "Encoder"
    return "Channel"


def el60_leds(card, x_left, x_right, y0):
    count, diag = EL_LEDS[card.stem]
    n = count + (1 if diag else 0)
    positions, legends = [], []
    word = led_word(card.stem)
    for k in range(n):
        cx = x_left if k % 2 == 0 else x_right
        positions.append((cx, y0 + LED_PITCH * (k // 2)))
        text = "%s %d" % (word, k + 1) if k < count else "Diagnostic"
        legends.append((k, "L" if k % 2 == 0 else "R", text))
    add_leds(card, positions, legends)


def power_contacts(card, right, bottom):
    ya = bottom - 14
    card.rect(right, ya, 3.5, 3.5, BLACK)
    card.rect(right, ya + 7, 3.5, 3.5, BLACK)
    card.legends.append(Legend("R", ya + 5.25, right + 3.5 + LEAD_GAP,
                               ["Power contacts", "+24 V DC / 0 V"], 6))


def build_el60(card):
    terminal_legends(card, 2.5, 7)
    el60_leds(card, -5, 10, -118 if card.stem == "el1918" else -112)
    power_contacts(card, card.maxx_body(), 140)


def build_el2911(card):
    """Two modules. LEDs of the manual: Input 1 to 4 and Diag 1 to 4 on the
    first one, Output 1 and Diag Out on the second one (the column of the
    last two is assumed). A legend names the two LEDs of a row."""
    terminal_legends(card, 32.5, 7)
    rows = ((-5, 10, "Safe input 1, 2"), (-5, 10, "Safe input 3, 4"),
            (-5, 10, "Diag 1, 2"), (-5, 10, "Diag 3, 4"))
    for k, (xa, xb, text) in enumerate(rows):
        y = -118 + LED_PITCH * k
        add_leds(card, [(xa, y), (xb, y)], [(0, "L", text)])
    y = -118
    add_leds(card, [(55, y), (70, y)], [(1, "R", "Safe output 1, Diag Out")])
    power_contacts(card, card.maxx_body(), 140)


ROW_Y = (-45, 5, 60, 115)    # the rows of the EL9410


def build_el9410(card):
    """EL9410 (power feed): the function of each terminal point, the three
    power contacts on the right (24 V, 0 V, PE) and the LEDs of the E-bus
    (left) and of the power contacts (right)."""
    legends = {"1": "1: +24 V DC", "5": "5: 0 V",
               "2": "2: Infeed 24 V", "6": "6: Infeed 24 V",
               "3": "3: Infeed 0 V", "7": "7: Infeed 0 V",
               "4": "4: PE", "8": "8: PE"}
    for t in card.terms:
        side = "L" if t.x < 2.5 else "R"
        card.legends.append(Legend(side, t.y, obstacle(card, t, side),
                                   [legends[t.name]], 7))
    y0 = -118
    add_leds(card, [(-5, y0 + LED_PITCH * k) for k in range(3)],
             [(0, "L", "Power (E-bus)"), (1, "L", "Diag. (E-bus)"),
              (2, "L", "Run")])
    add_leds(card, [(10, y0), (10, y0 + LED_PITCH)],
             [(0, "R", "Power (power contacts)"),
              (1, "R", "Diagnostic (power contacts)")])
    right = card.maxx_body()
    for y in ROW_Y[1:]:
        card.rect(right, y + 9.25, 3.5, 3.5, BLACK)
    card.legends.append(Legend("R", ROW_Y[1] + 11, right + 3.5 + LEAD_GAP,
                               ["Power contacts", "24 V / 0 V / PE"], 6))


def build_ek1322(card):
    """EK1322 (EtherCAT P junction with feed-in): the two M8 sockets, the
    feed-in terminals (Us left, Up right) and the five LEDs (see
    gen_ek1322.py; the layout is assumed)."""
    import gen_ek1322 as ek
    for t in card.terms:
        if t.name.startswith("X"):
            word = "previous strand" if t.name == "X1" else "next strand"
            card.legends.append(Legend(
                "R", t.y, t.x + t.r + LEAD_GAP,
                ["%s: M8 socket" % t.name, "EtherCAT P, %s" % word], 6))
        else:
            side = "L" if t.x < ek.CX else "R"
            text = dict(ek.FUNCTION)[t.name]
            card.legends.append(Legend(
                side, t.y, t.x + (-1 if side == "L" else 1)
                * (t.r + LEAD_GAP), ["%s: %s" % (t.name, text)], 7))
    words = {"Run": "LED Run", "L/A X1": "LED L/A X1",
             "L/A X2": "LED L/A X2", "Us": "LED Us (system supply)",
             "Up": "LED Up (peripheral supply)"}
    for led, (lx, ly) in ek.LEDS.items():
        card.circle(lx, ly, LED_D)
        card.legends.append(Legend(
            "L" if lx < ek.CX else "R", ly,
            lx + (-1 if lx < ek.CX else 1) * (LED_D / 2 + LEAD_GAP),
            [words[led]], 6))


def build_ek1914(card):
    positions = [(28, -112 + LED_PITCH * k) for k in range(4)]
    add_leds(card, positions,
             [(k, "R", "Status LED %d" % (k + 1)) for k in range(4)])
    rows = ("+24 V (Us) / 0 V", "Input 1 / Input 2", "Input 3 / Input 4",
            "Output 1 / Output 2", "Output 3 / Output 4",
            "Clock 1 / Clock 2", "Safe input 1 / 2", "Safe output 1 / 2")
    for k, text in enumerate(rows):
        card.legends.append(Legend("R", -50 + 25 * k, 47.0,
                                   ["%d / %d: %s" % (k + 1, k + 9, text)], 6))


def build_drive(card):
    centre = card.w / 2.0 - card.hx
    terminal_legends(card, centre, 6)
    leds = [c for c in card.desc.findall("circle")
            if c.get("style", "").find("filling:orange") >= 0
            or (c.get("style", "").find("filling:green") >= 0
                and float(c.get("y")) < 80)]
    if leds:
        right = max(float(c.get("x")) + float(c.get("diameter"))
                    for c in leds)
        cy = float(leds[0].get("y")) + float(leds[0].get("diameter")) / 2
        card.legends.append(Legend("R", cy, right + LEAD_GAP,
                                   ["Status LEDs"], 6))


def build_epp(card):
    """EPP2338: M12 sockets X01 to X04 and M8 EtherCAT P sockets (see
    gen_epp.py for the pinout and the layout)."""
    import gen_epp as epp
    for n, cy in enumerate(epp.SOCKET_Y):
        name = "X%02d" % (n + 1)
        sock = next(t for t in card.terms if t.name == name)
        card.legends.append(Legend(
            "R", sock.y, sock.x + sock.r + LEAD_GAP,
            ["%s: M12 socket" % name, "Pin 1: +24 V DC Up",
             "Pin 2: In-/Output B", "Pin 3: GND P",
             "Pin 4: In-/Output A"], 6))
        # the leader runs between the two label holders of the socket
        card.legends.append(Legend(
            "L", cy, epp.CHANNEL_LED_X[0] - epp.LED_D / 2 - LEAD_GAP,
            ["LEDs %d, %d" % (2 * n + 1, 2 * n + 2)], 6))
    pinout = ["Pin %d: %s" % (i + 1, p)
              for i, p in enumerate(epp.M8_PINOUT)]
    for t in card.terms:
        if t.name in ("IN X50", "OUT X51"):
            side = "L" if t.x < -20 else "R"
            word = "IN" if t.name.startswith("IN") else "OUT"
            card.legends.append(Legend(
                side, t.y + (-6 if side == "R" else 6),
                t.x + (-1 if side == "L" else 1) * (t.r + LEAD_GAP),
                ["%s: M8, EtherCAT P %s" % (t.name.split()[1], word)]
                + pinout, 6))
    # L/A LEDs: leader going up, text above the housing
    top = epp.BODY[1]
    for (lx, ly), word, side in zip(epp.LA_LEDS, ("IN", "OUT"), "LR"):
        card.line(lx, ly - epp.LED_D / 2 - LEAD_GAP, lx, top - 6)
        text = "LED L/A " + word
        x = lx - 2 - twidth(text, 6) if side == "L" else lx + 2
        card.text(x, top - 7, text, 6)
    card.legends.append(Legend(
        "L", epp.RUN_LED[1], epp.RUN_LED[0] - epp.LED_D / 2 - LEAD_GAP,
        ["LED Run"], 6))
    card.legends.append(Legend(
        "R", epp.POWER_LEDS[1][1],
        epp.POWER_LEDS[1][0] + epp.LED_D / 2 + LEAD_GAP,
        ["LED Us (left)", "LED Up (right)"], 6))


def maxx_body(self):
    best = -1e9
    for p in self.desc:
        if p.tag in ("line", "rect", "polygon", "arc"):
            b = box(p)
            if b and b[3] - b[1] > 30:
                best = max(best, b[2])
    return best


Card.maxx_body = maxx_body


# ---------------------------------------------------------------------------
def resolve(card):
    """Place the legend texts and draw the leaders."""
    left_x = card.minx - GAP
    right_x = card.maxx + GAP
    for side in ("L", "R"):
        items = sorted((l for l in card.legends if l.side == side),
                       key=lambda l: l.y)
        prev_end = -1e9
        for lg in items:
            h = lg.height
            lg.yc = max(lg.y, prev_end + h / 2.0)
            prev_end = lg.yc + h / 2.0
        for lg in items:
            lh = line_height(lg.size)
            top = lg.yc - lg.height / 2.0
            if side == "L":
                card.line(lg.x0, lg.y, left_x, lg.yc)
            else:
                card.line(lg.x0, lg.y, right_x, lg.yc)
            for i, text in enumerate(lg.lines):
                w = twidth(text, lg.size)
                x = left_x - 2 - w if side == "L" else right_x + 2
                card.text(x, baseline_of(lg.size, top + i * lh), text, lg.size)


def finish(card):
    e = card.new
    xs, ys = [-card.hx, card.w - card.hx], [-card.hy, card.h - card.hy]
    for p in e:
        if p.tag == "text":
            size = int(p.get("font").split(",")[1])
            x, y = float(p.get("x")), float(p.get("y"))
            xs += [x - 1, x + twidth(p.get("text"), size) + 1]
            ys += [y - 0.93 * size * 4 / 3 - 1, y + 0.25 * size * 4 / 3 + 1]
        else:
            b = box(p)
            xs += [b[0] - 1, b[2] + 1]
            ys += [b[1] - 1, b[3] + 1]
    hx, hy = math.ceil(-min(xs)), math.ceil(-min(ys))
    w, h = hx + math.ceil(max(xs)), hy + math.ceil(max(ys))
    card.root.set("width", str(w))
    card.root.set("height", str(h))
    card.root.set("hotspot_x", str(hx))
    card.root.set("hotspot_y", str(hy))
    first = None
    for child in card.desc:
        if child.tag in ("terminal", "dynamic_text"):
            first = child
            break
    index = list(card.desc).index(first) if first is not None \
        else len(card.desc)
    for k, p in enumerate(e):
        card.desc.insert(index + k, p)


def retag(card):
    r = card.root
    for n in r.find("names"):
        n.text = n.text.replace(", realistic view", ", description view") \
                       .replace(", vue rÃ©aliste", ", vue description")
    r.find("uuid").set("uuid", uid("description", card.stem))
    for t in card.desc.findall("terminal"):
        t.set("uuid", uid("description", card.stem, t.get("name")))
    dyn = card.desc.find("dynamic_text")
    if dyn is not None:
        dyn.set("uuid", uid("description", card.stem, "label"))
    info = r.find("informations")
    if info is not None:
        info.text = "Author: QElectroTech contributors\n" + NOTE


def build(card):
    s = card.stem
    if s == "el9410":
        build_el9410(card)
    elif s in EL60:
        build_el60(card)
    elif s == "el2911":
        build_el2911(card)
    elif s == "ek1914":
        build_ek1914(card)
    elif s == "ek1322":
        build_ek1322(card)
    elif s.startswith("af") or s.startswith("ax"):
        build_drive(card)
    elif s == "epp2338":
        build_epp(card)
    else:
        raise SystemExit("no description layout for " + s)


def check_overlaps(card):
    boxes = []
    for p in card.new:
        if p.tag != "text":
            continue
        size = int(p.get("font").split(",")[1])
        x, y = float(p.get("x")), float(p.get("y"))
        boxes.append((x, y - 0.93 * size * 4 / 3,
                      x + twidth(p.get("text"), size),
                      y + 0.25 * size * 4 / 3, p.get("text")))
    for i, a in enumerate(boxes):
        for b in boxes[i + 1:]:
            if a[0] < b[2] and b[0] < a[2] and a[1] < b[3] and b[1] < a[3]:
                print("  overlap in %s: %r / %r" % (card.stem, a[4], b[4]))


def main(root_dir):
    count = 0
    for path in sorted(glob.glob(os.path.join(root_dir, "**",
                                              "*_realistic.elmt"),
                                 recursive=True)):
        card = Card(path)
        build(card)
        resolve(card)
        check_overlaps(card)
        finish(card)
        retag(card)
        write(card.root, path[:-len("_realistic.elmt")] + "_description.elmt")
        count += 1
    print("%d description views written" % count)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
