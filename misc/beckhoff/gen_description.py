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
  * the status LEDs, one per channel with a "Signal n" legend, and the
    diagnostic LED of the TwinSAFE cards;
  * the power contacts (+24 V DC / 0 V) of the terminals that have them.

The legends are on the left and on the right of the drawing, outside of it.
The data (I/O table, terminals) is copied from the realistic view, never
duplicated by hand: regenerate the realistic view first, then run this
script.  The LED sets and the power contacts are generic (they are not taken
from a datasheet): see docs/beckhoff-ethercat.md.

Usage:
  gen_description.py <elements dir>   (e.g. extra-elements/15_beckhoff_ethercat)
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
LED_PITCH = 9.0
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
    "el2652": (4, False),
}
EL60 = ("el1004", "el1008", "el2004", "el2008", "el2522", "el2574", "el5152",
        "el9100", "el1904", "el2904", "el1xxx_generic", "el2xxx_generic",
        "el3xxx_generic", "el4xxx_generic", "el2652")


def uid(*parts):
    return "{%s}" % uuid.uuid5(NS, "/".join(parts))


def line_height(size):
    return size * 4.0 / 3.0 + 1.0


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


def el60_leds(card, x_left, x_right, y0):
    count, diag = EL_LEDS[card.stem]
    n = count + (1 if diag else 0)
    positions, legends = [], []
    for k in range(n):
        cx = x_left if k % 2 == 0 else x_right
        positions.append((cx, y0 + LED_PITCH * (k // 2)))
        text = "Signal %d" % (k + 1) if k < count else "Diagnostic"
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
    el60_leds(card, -5, 10, -112)
    power_contacts(card, card.maxx_body(), 140)


def build_el2911(card):
    terminal_legends(card, 32.5, 7,
                     lambda t: t.x in (-5.0, 70.0))
    count, diag = EL_LEDS["el2911"]
    positions, legends = [], []
    for k in range(count + 1):
        positions.append((-5, -118 + LED_PITCH * k))
        legends.append((k, "L", "Signal %d" % (k + 1) if k < count
                        else "Diagnostic"))
    add_leds(card, positions, legends)
    for k in range(count + 1):
        card.circle(55, -118 + LED_PITCH * k, LED_D)
    power_contacts(card, card.maxx_body(), 140)


def build_el1918(card):
    terminal_legends(card, 25, 7)
    card.rect(-13, -58, 76, 14, YELLOW)
    positions = [(-7 + 8 * k, -51) for k in range(9)]
    for cx, cy in positions:
        card.circle(cx, cy, LED_D)
    card.legends.append(Legend("L", -51, -7 - LED_D / 2 - LEAD_GAP,
                               ["Signal 1 to 8"], 6))
    card.legends.append(Legend("R", -51, 57 + LED_D / 2 + LEAD_GAP,
                               ["Diagnostic"], 6))
    power_contacts(card, card.maxx_body(), 111)


def build_ek1914(card):
    positions = [(28, -79 + LED_PITCH * k) for k in range(4)]
    add_leds(card, positions,
             [(k, "R", "Status LED %d" % (k + 1)) for k in range(4)])
    card.legends.append(Legend("R", 10, 39.5 + LEAD_GAP,
                               ["Terminals 1 to 10", "standard I/O"], 6))
    card.legends.append(Legend("R", 61, 39.5 + LEAD_GAP,
                               ["Terminals 11 to 16", "TwinSAFE I/O"], 6))


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
    gen_epp.py for the pinout)."""
    import gen_epp as epp
    for n, cy in enumerate(epp.SOCKET_Y):
        name = "X%02d" % (n + 1)
        pins = {t.name: t for t in card.terms
                if t.name.startswith(name + ".")}
        left, right = pins[name + ".1"], pins[name + ".2"]
        card.legends.append(Legend(
            "L", left.y, left.x - left.r - LEAD_GAP,
            ["%s: M12 socket" % name, "Pin 1: +24 V DC Up",
             "Pin 3: GND P"], 6))
        card.legends.append(Legend(
            "R", right.y, right.x + right.r + LEAD_GAP,
            ["Pin 2: In-/Output B", "Pin 4: In-/Output A"], 6))
        for dy, text in ((-8, "LED channel B"), (8, "LED channel A")):
            card.legends.append(Legend(
                "R", cy + dy, 33 + epp.LED_D / 2 + LEAD_GAP, [text], 6))
    pinout = ["Pin %d: %s" % (i + 1, p)
              for i, p in enumerate(epp.M8_PINOUT)]
    for t in card.terms:
        if t.name in ("IN X50", "OUT X51"):
            side = "L" if t.x < 0 else "R"
            word = "IN" if t.name.startswith("IN") else "OUT"
            card.legends.append(Legend(
                side, t.y, t.x + (-1 if side == "L" else 1) * (t.r + LEAD_GAP),
                ["%s: M8, EtherCAT P %s" % (t.name.split()[1], word)]
                + pinout, 6))
    card.legends.append(Legend("L", -163, -25 - epp.LED_D / 2 - LEAD_GAP,
                               ["LED L/A (IN)"], 6))
    card.legends.append(Legend("R", -163, 25 + epp.LED_D / 2 + LEAD_GAP,
                               ["LED L/A (OUT)"], 6))
    card.legends.append(Legend("L", 173, -10 - epp.LED_D / 2 - LEAD_GAP,
                               ["LED Us"], 6))
    card.legends.append(Legend("R", 173, 10 + epp.LED_D / 2 + LEAD_GAP,
                               ["LED Up"], 6))


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
                card.text(x, top + i * lh + 0.5, text, lg.size)


def finish(card):
    e = card.new
    xs, ys = [-card.hx, card.w - card.hx], [-card.hy, card.h - card.hy]
    for p in e:
        if p.tag == "text":
            size = int(p.get("font").split(",")[1])
            x, y = float(p.get("x")), float(p.get("y"))
            xs += [x - 1, x + twidth(p.get("text"), size) + 1]
            ys += [y - 1, y + line_height(size) + 1]
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
    if s in EL60:
        build_el60(card)
    elif s == "el2911":
        build_el2911(card)
    elif s == "el1918":
        build_el1918(card)
    elif s == "ek1914":
        build_ek1914(card)
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
        boxes.append((x, y, x + twidth(p.get("text"), size),
                      y + line_height(size) - 1, p.get("text")))
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
