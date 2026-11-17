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
"""Redraw the EPP2338 (schematic and realistic views, I/O data).

The EPP2338 is an EtherCAT P box with a dark housing, not a terminal: four
M12 sockets X01 to X04 (two channels each) and two M8 EtherCAT P sockets
X50 (IN) and X51 (OUT).  Pin of a M12 socket: 1 +24 V DC Up, 2 channel B,
3 GND P, 4 channel A, 5 not connected.  Pin of a M8 socket: 1 Tx+ / GNDs,
2 Rx+ / GNDp, 3 Rx- / Up, 4 Tx- / Us.  The positions of the sockets, of the
pins in a socket and of the LEDs on the housing are ASSUMED (the drawing is
original, redrawn from the structure of the box).

The script patches the files of the box folder in place: it keeps their
header and dynamic text, replaces the drawing, the terminals and the I/O
table data.  Run it after gen_symbols.py / organize.py, before
gen_description.py and simple_tables.py.

Usage: python gen_epp.py <extra-elements>/15_beckhoff_ethercat
"""
import os
import sys
import uuid
import xml.etree.ElementTree as ET

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from realistic_fixes import box, num, write  # noqa: E402

NS = uuid.UUID("6f1c2a5e-8b0f-4d57-9a7e-5b1d0c8e2f10")
FONT = "Roboto Condensed,%d,-1,5,50,0,0,0,0,0,Regular"
FONT_BOLD = "Roboto Condensed,%d,-1,5,75,0,0,0,0,0,Bold"
THIN = "line-style:normal;line-weight:thin;filling:none;color:black"
NORMAL = "line-style:normal;line-weight:normal;filling:none;color:black"
DOTTED = "line-style:dotted;line-weight:normal;filling:none;color:black"
WHITE = "line-style:normal;line-weight:thin;filling:white;color:black"
BLACK = "line-style:normal;line-weight:thin;filling:black;color:black"
DARK = "line-style:normal;line-weight:normal;filling:gray;color:black"
LED = "line-style:normal;line-weight:thin;filling:green;color:black"
REFERENCE = "EPP2338-0002"

BODY = (-40.0, -175.0, 80.0, 380.0)        # x, y, width, height
SOCKET_Y = (-70, 0, 70, 140)               # centres of X01 to X04
SOCKET_R = 25.0                            # M12 socket
PIN_D = 9.0
PIN = 15.0                                 # offset of the pins
M8_X = (-20, 20)
M8_Y = -135
M8_R = 13.0
LED_D = 5.0
FUNCTION = {"A": "In-/Output A", "B": "In-/Output B"}
# pin of the M12 sockets: number -> (dx, dy), then the use of each pin
M12_PINS = {1: (-PIN, -PIN), 2: (PIN, -PIN), 3: (PIN, PIN), 4: (-PIN, PIN)}
M8_PINOUT = ("Tx+ / GNDs", "Rx+ / GNDp", "Rx- / Up", "Tx- / Us")
# A is on the pin 4, B on the pin 2 of every M12 socket
CHANNELS = ((1, "A", 4), (2, "B", 2))


def uid(*parts):
    return "{%s}" % uuid.uuid5(NS, "/".join(parts))


class Drawing:
    def __init__(self, tag):
        self.tag = tag
        self.parts = []
        self.terms = []

    def line(self, x1, y1, x2, y2, style=THIN):
        self.parts.append(ET.Element("line", {
            "end1": "none", "end2": "none", "y1": num(y1),
            "antialias": "false", "y2": num(y2), "style": style,
            "x1": num(x1), "length1": "1.5", "x2": num(x2),
            "length2": "1.5"}))

    def rect(self, x, y, w, h, style=THIN):
        self.parts.append(ET.Element("rect", {
            "x": num(x), "y": num(y), "width": num(w), "height": num(h),
            "antialias": "false", "style": style, "rx": "0", "ry": "0"}))

    def circle(self, cx, cy, d, style=WHITE):
        self.parts.append(ET.Element("circle", {
            "x": num(cx - d / 2), "y": num(cy - d / 2), "antialias": "false",
            "style": style, "diameter": num(d)}))

    def text(self, x, y, s, size=5, bold=False):
        self.parts.append(ET.Element("text", {
            "x": num(x), "y": num(y), "rotation": "0", "color": "#000000",
            "font": (FONT_BOLD if bold else FONT) % size, "text": s}))

    def terminal(self, x, y, name, orientation="e"):
        self.terms.append(ET.Element("terminal", {
            "x": num(x), "y": num(y), "name": name,
            "orientation": orientation, "type": "Generic",
            "uuid": uid(self.tag, name)}))


def pins_of(n):
    cy = SOCKET_Y[n]
    return {k: (cx_off, cy + dy) for k, (cx_off, dy) in M12_PINS.items()}


def housing(d, dotted):
    x, y, w, h = BODY
    if dotted:
        d.rect(x, y, w, h, DOTTED)
        for cx in (-20, 20):
            d.rect(cx - 8, y - 7, 16, 7)
            d.rect(cx - 8, y + h, 16, 7)
    else:
        d.rect(x, y, w, h, DARK)
        d.rect(x + 3, y + 3, w - 6, h - 6)


def realistic():
    d = Drawing("epp2338-realistic")
    housing(d, False)
    for cx in M8_X:
        d.circle(cx, M8_Y, 2 * M8_R, WHITE)
    d.terminal(M8_X[0], M8_Y, "IN X50")
    d.terminal(M8_X[1], M8_Y, "OUT X51")
    d.text(-31, M8_Y + 14, "IN X50", 5)
    d.text(9, M8_Y + 14, "OUT X51", 5)
    # L/A of IN and OUT, Run
    for cx in (-25, 25, 0):
        d.circle(cx, -163, LED_D, LED)
    for n, cy in enumerate(SOCKET_Y):
        name = "X%02d" % (n + 1)
        d.circle(0, cy, 2 * SOCKET_R, WHITE)
        d.circle(0, cy, 2 * SOCKET_R - 6, WHITE)
        for k, (dx, dy) in M12_PINS.items():
            d.circle(dx, cy + dy, PIN_D, WHITE)
            d.terminal(dx, cy + dy, "%s.%d" % (name, k))
            d.text(dx - 1.5, cy + dy - 3.5, str(k), 5)
        d.circle(0, cy, 6, WHITE)
        d.text(-38, cy - 4, name, 6, True)
        # status LEDs of the two channels, on the right of the socket
        d.circle(33, cy - 8, LED_D, LED)
        d.circle(33, cy + 8, LED_D, LED)
        # channel labels, outside of the socket ring
        d.text(3, cy - 33, "%s(2)" % name, 5)
        d.text(-30, cy + 28, "%s(1)" % name, 5)
    for cx in (-10, 10):
        d.circle(cx, 173, LED_D, LED)
    d.text(-19, 187, REFERENCE.split("-")[0], 8, True)
    return d


def schematic():
    d = Drawing("epp2338")
    housing(d, True)
    for cx in M8_X:
        d.circle(cx, M8_Y, 2 * M8_R, WHITE)
    d.terminal(M8_X[0], M8_Y, "IN X50")
    d.terminal(M8_X[1], M8_Y, "OUT X51")
    d.text(-31, M8_Y + 14, "IN X50", 5)
    d.text(9, M8_Y + 14, "OUT X51", 5)
    d.line(-7, M8_Y, 7, M8_Y, DOTTED)
    rail_l, rail_r = -36, 36
    first, last = SOCKET_Y[0] - PIN, SOCKET_Y[-1] + PIN
    d.line(rail_l, first, rail_l, last - 2 * PIN)
    d.line(rail_r, first + 2 * PIN, rail_r, last)
    d.text(rail_l - 2, first - 11, "+24 V Up", 5)
    d.text(rail_r - 12, last + 6, "GND P", 5)
    for n, cy in enumerate(SOCKET_Y):
        name = "X%02d" % (n + 1)
        for k, (dx, dy) in M12_PINS.items():
            px, py = dx, cy + dy
            d.terminal(px, py, "%s.%d" % (name, k))
            if k == 1:
                d.line(px - PIN_D / 2, py, rail_l, py)
                d.circle(rail_l, py, 2, BLACK)
            elif k == 3:
                d.line(px + PIN_D / 2, py, rail_r, py)
                d.circle(rail_r, py, 2, BLACK)
            else:
                # channel: LED of the input stage, then the load or sensor
                sign = -1 if k == 4 else 1
                d.line(px + sign * PIN_D / 2, py, sign * 30, py)
                d.circle(sign * 25, py, LED_D, LED)
                d.circle(sign * 30, py, 2, BLACK)
        for k, (dx, dy) in M12_PINS.items():
            d.circle(dx, cy + dy, PIN_D, WHITE)
            d.text(dx - 1.5, cy + dy - 3.5, str(k), 5)
        d.text(-38, cy - 4, name, 6, True)
        d.text(3, cy - 33, "%s(2)" % name, 5)
        d.text(-30, cy + 28, "%s(1)" % name, 5)
    d.text(-19, 187, REFERENCE.split("-")[0], 8, True)
    return d


def bounds(parts, terms):
    xs, ys = [], []
    for p in parts:
        if p.tag == "text":
            size = int(p.get("font").split(",")[1])
            x, y = float(p.get("x")), float(p.get("y"))
            xs += [x, x + len(p.get("text")) * size * 0.7]
            ys += [y, y + size * 1.4]
        else:
            b = box(p)
            xs += [b[0], b[2]]
            ys += [b[1], b[3]]
    for t in terms:
        xs.append(float(t.get("x")))
        ys.append(float(t.get("y")))
    return min(xs), min(ys), max(xs), max(ys)


def patch_io(root):
    """Pin of the M12 sockets and a function for each channel."""
    for io in root.iter("plcIO"):
        term = io.find("terminal")
        pin = term.text
        io.set("functionText", FUNCTION["A" if pin.endswith(".4") else "B"])
    info = {
        "manufacturer_reference": REFERENCE,
        "designation": "EtherCAT P Box, 8-channel digital input/output, "
                       "24 V DC, M12",
        "description": "EtherCAT P box with a dark housing: four M12 sockets "
                       "X01 to X04, two channels each (pin 4 = channel A, "
                       "pin 2 = channel B, pin 1 = +24 V DC Up, pin 3 = "
                       "GND P, pin 5 not connected), and two M8 EtherCAT P "
                       "sockets, X50 IN and X51 OUT. The pins X01.1 to "
                       "X04.4 are drawn on the sockets; the channel "
                       "(X01(1)...) is the signal of the I/O table, not the "
                       "pin. The positions on the housing are assumed. The "
                       "label of the box is the card id (A1.10).",
    }
    for e in root.iter("elementInformation"):
        if e.get("name") in info:
            e.text = info[e.get("name")]


def patch(path, drawing, rename=None):
    tree = ET.parse(path)
    root = tree.getroot()
    desc = root.find("description")
    keep = [c for c in desc if c.tag == "dynamic_text"]
    for c in list(desc):
        desc.remove(c)
    x0, y0, x1, y1 = bounds(drawing.parts, drawing.terms)
    hx, hy = int(-x0) + 10, int(-y0) + 20
    root.set("hotspot_x", str(hx))
    root.set("hotspot_y", str(hy))
    root.set("width", str(hx + int(x1) + 10))
    root.set("height", str(hy + int(y1) + 40))
    for p in drawing.parts + drawing.terms:
        desc.append(p)
    for k in keep:
        below = k.findtext("info_name") != "label"
        k.set("x", num(BODY[0]))
        k.set("y", num(BODY[1] + BODY[3] + 12 if below else BODY[1] - 20))
        desc.append(k)
    patch_io(root)
    write(root, path)


def patch_table(path):
    root = ET.parse(path).getroot()
    patch_io(root)
    write(root, path)


def main(folder):
    box_dir = os.path.join(folder, "epp_box")
    patch(os.path.join(box_dir, "epp2338.elmt"), schematic())
    patch(os.path.join(box_dir, "epp2338_realistic.elmt"), realistic())
    patch_table(os.path.join(box_dir, "epp2338_table.elmt"))
    print("EPP2338 redrawn")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
