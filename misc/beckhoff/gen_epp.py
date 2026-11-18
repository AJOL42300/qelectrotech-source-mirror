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
2 Rx+ / GNDp, 3 Rx- / Up, 4 Tx- / Us.  In QET each M12 socket is ONE terminal
(X01 to X04) shared by its two channels (their plcIO name the same terminal).
The positions of the sockets, of the pins in a socket and of the LEDs on the
housing are ASSUMED (the drawing is original and vector only, traced by eye on
the layout of the box: dark housing with a lighter lower band, chamfered top,
two fixing holes, red M8 sockets, grey M12 sockets, two LEDs and two label
holders per M12 socket, L/A and Run LEDs between the M8 sockets, Us and Up LEDs
under the last socket).

The script patches the files of the box folder in place: it keeps their
header and dynamic text, replaces the drawing, the terminals and the I/O
table data.  Run it after gen_symbols.py / organize.py, before
gen_description.py and simple_tables.py.

Usage: python gen_epp.py <extra-elements>/10_electric/20_manufacturers_articles/beckhoff
"""
import math
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
RAIL_24 = "line-style:normal;line-weight:normal;filling:none;color:red"
RAIL_0 = "line-style:normal;line-weight:normal;filling:none;color:blue"
DOT_24 = "line-style:normal;line-weight:thin;filling:red;color:red"
DOT_0 = "line-style:normal;line-weight:thin;filling:blue;color:blue"
DARK = "line-style:normal;line-weight:normal;filling:gray;color:black"
BAND = "line-style:normal;line-weight:thin;filling:lightgray;color:black"
RED = "line-style:normal;line-weight:thin;filling:red;color:black"
METAL = "line-style:normal;line-weight:thin;filling:lightgray;color:black"
LED = "line-style:normal;line-weight:thin;filling:green;color:black"
REFERENCE = "EPP2338-0002"

# Layout traced (by eye, at 0.71 of the photo scale) on the structure of the
# box: the M12 sockets are on the right of the housing, the LEDs and the label
# holders on their left.  The M12 axis is x = 0.
BODY = (-70.0, -160.0, 100.0, 417.0)       # x, y, width, height
SOCKET_Y = (-70, 0, 70, 140)               # centres of X01 to X04
SOCKET_R = 22.0                            # M12 socket
PIN_D = 8.0                                # insert of the M12 socket
PIN = 15.0                                 # offset of the pins (schematic)
SCH_R = 20.0                               # M12 socket (schematic)
M8_X = (-50, 5)
M8_Y = -135
M8_R = 13.0
LED_D = 5.0
CHANNEL_LED_X = (-57, -38)                 # the two LEDs of a M12 socket
HOLDER = (-67.0, 33.0, 18.0)               # label holder: x, width, height
RUN_LED = (-20, -115)
LA_LEDS = ((-30, -142), (-15, -142))       # L/A IN, L/A OUT
POWER_LEDS = ((-4, 185), (11, 185))        # Us, Up
FUNCTION = {"A": "In-/Output A", "B": "In-/Output B"}
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

    def polygon(self, points, style=THIN):
        a = {"closed": "true", "style": style, "antialias": "false"}
        for k, (x, y) in enumerate(points, 1):
            a["x%d" % k], a["y%d" % k] = num(x), num(y)
        self.parts.append(ET.Element("polygon", a))

    def text(self, x, y, s, size=5, bold=False, color="#000000"):
        """y is the baseline of the text (QET draws it so)."""
        self.parts.append(ET.Element("text", {
            "x": num(x), "y": num(y), "rotation": "0", "color": color,
            "font": (FONT_BOLD if bold else FONT) % size, "text": s}))

    def terminal(self, x, y, name, orientation="e"):
        self.terms.append(ET.Element("terminal", {
            "x": num(x), "y": num(y), "name": name,
            "orientation": orientation, "type": "Generic",
            "uuid": uid(self.tag, name)}))


def body_outline(d, style):
    x, y, w, h = BODY
    c_top, c_bottom = 10.0, 7.0
    d.polygon([(x + c_top, y), (x + w - c_top, y), (x + w, y + c_top),
               (x + w, y + h - c_bottom), (x + w - c_bottom, y + h),
               (x + c_bottom, y + h), (x, y + h - c_bottom),
               (x, y + c_top)], style)


def housing(d, dotted):
    x, y, w, h = BODY
    if dotted:
        d.rect(x, y, w, h, DOTTED)
        for cx in (-40, 0):
            d.rect(cx - 8, y - 7, 16, 7)
            d.rect(cx - 8, y + h, 16, 7)
    else:
        body_outline(d, DARK)
        # inclined lower band, a little lighter than the body
        d.polygon([(x, y + h - 62), (x + w, y + h - 67),
                   (x + w, y + h - 7), (x + w - 7, y + h),
                   (x + 7, y + h), (x, y + h - 7)], BAND)
        # fixing holes: top right and bottom left
        d.circle(23, -155, 7, WHITE)
        d.circle(-61, 246, 7, WHITE)


def reference_text(d, bottom=False):
    d.text(-43, 245 if bottom else 241, REFERENCE, 7, True)


def m8_sockets(d, detail):
    for cx in M8_X:
        d.circle(cx, M8_Y, 2 * M8_R, RED if detail else WHITE)
        d.circle(cx, M8_Y, 2 * M8_R - 10, DARK)
    # the M8 sockets send their conductors up, the M12 ones to the right
    d.terminal(M8_X[0], M8_Y, "IN X50", "n")
    d.terminal(M8_X[1], M8_Y, "OUT X51", "n")
    d.text(-58, M8_Y - 15, "IN", 5)
    d.text(M8_X[1] + 3, M8_Y - 15, "OUT", 5)
    d.text(-66, -118, "X50", 4)
    d.text(M8_X[1] + 10, -118, "X51", 4)
    if detail:
        # IN / OUT arrows under the sockets
        for x0, x1 in ((-45, -30), (-12, 3)):
            d.line(x0, -104, x1, -104)
            d.line(x1, -104, x1 - 3, -106.5)
            d.line(x1, -104, x1 - 3, -101.5)


def realistic():
    d = Drawing("epp2338-realistic")
    housing(d, False)
    m8_sockets(d, True)
    d.text(-29, -150, "L/A", 5)
    for cx, cy in LA_LEDS + (RUN_LED,):
        d.circle(cx, cy, LED_D, LED)
    d.text(-30, -108.5, "Run", 4)
    for n, cy in enumerate(SOCKET_Y):
        name = "X%02d" % (n + 1)
        # M12 socket: metal ring, insert and five pins
        d.circle(0, cy, 2 * SOCKET_R, METAL)
        d.circle(0, cy, 2 * SOCKET_R - 8, WHITE)
        d.circle(0, cy, 2 * PIN_D, DARK)
        for k in range(5):
            a = math.radians(90 + 72 * k)
            d.circle(5.5 * math.cos(a), cy - 5.5 * math.sin(a), 2.2, BLACK)
        d.terminal(0, cy, name)
        d.text(-26, cy + 25, name, 4)
        # the two channel LEDs, numbered, between two label holders
        for k, lx in enumerate(CHANNEL_LED_X):
            d.circle(lx, cy, LED_D, LED)
            d.text(lx - 8 if k == 0 else lx + 4, cy - 3, str(2 * n + k + 1),
                   4)
        hx, hw, hh = HOLDER
        d.rect(hx, cy - 25, hw, hh)
        d.rect(hx, cy + 9, hw, hh)
    hx, hw, hh = HOLDER
    d.rect(hx, SOCKET_Y[-1] + 33, hw, hh)
    for (lx, ly), s in zip(POWER_LEDS, ("Us", "Up")):
        d.circle(lx, ly, LED_D, LED)
        d.text(lx - 4, ly - 5, s, 4)
    d.text(-59, 220, "EtherCAT P", 6)
    reference_text(d)
    return d


def schematic():
    d = Drawing("epp2338")
    housing(d, True)
    m8_sockets(d, False)
    d.line(M8_X[0] + M8_R, M8_Y, M8_X[1] - M8_R, M8_Y, DOTTED)
    rail_l, rail_r = -27, 27
    first, last = SOCKET_Y[0] - PIN, SOCKET_Y[-1] + PIN
    d.line(rail_l, first, rail_l, last - 2 * PIN, RAIL_24)
    d.line(rail_r, first + 2 * PIN, rail_r, last, RAIL_0)
    d.text(rail_l - 18, first - 6, "+24 V Up", 5, color="#ff0000")
    d.text(rail_r - 12, last + 13, "GND P", 5, color="#0000ff")
    for n, cy in enumerate(SOCKET_Y):
        name = "X%02d" % (n + 1)
        # the socket is one terminal; its pins 1 (+24 V Up) and 3 (GND P) go
        # to the rails, the pins 4 (A) and 2 (B) to the channel LEDs
        edge = (SCH_R ** 2 - PIN ** 2) ** 0.5
        for sign, dy, rail, wire, dot in ((-1, -PIN, rail_l, RAIL_24, DOT_24),
                                          (1, PIN, rail_r, RAIL_0, DOT_0)):
            d.line(sign * edge, cy + dy, rail, cy + dy, wire)
            d.circle(rail, cy + dy, 2, dot)
        for sign, dy in ((-1, PIN), (1, -PIN)):
            d.line(sign * edge, cy + dy, sign * 21, cy + dy)
            d.circle(sign * 17.5, cy + dy, LED_D, LED)
            d.circle(sign * 21, cy + dy, 2, BLACK)
        d.circle(0, cy, 2 * SCH_R, WHITE)
        d.circle(0, cy, 2 * PIN_D, WHITE)
        d.terminal(0, cy, name)
        d.text(-47, cy + 2, name, 6, True)
        d.text(3, cy - 25, "%s(2)" % name, 5)
        d.text(-24, cy + 33, "%s(1)" % name, 5)
    reference_text(d, True)
    return d


def bounds(parts, terms):
    xs, ys = [], []
    for p in parts:
        if p.tag == "text":
            size = int(p.get("font").split(",")[1])
            x, y = float(p.get("x")), float(p.get("y"))
            xs += [x, x + len(p.get("text")) * size * 0.7]
            ys += [y - size * 1.3, y + size * 0.4]
        else:
            b = box(p)
            xs += [b[0], b[2]]
            ys += [b[1], b[3]]
    for t in terms:
        xs.append(float(t.get("x")))
        ys.append(float(t.get("y")))
    return min(xs), min(ys), max(xs), max(ys)


def patch_io(root):
    """Each M12 socket is one terminal carrying its two channels: the
    function of a channel follows its number (1 = A on pin 4, 2 = B on
    pin 2)."""
    for io in root.iter("plcIO"):
        term = io.find("terminal")
        term.text = term.text.split(".")[0]
        io.set("functionText",
               FUNCTION["A" if io.get("address").endswith("(1)") else "B"])
    info = {
        "manufacturer_reference": REFERENCE,
        "designation": "EtherCAT P Box, 8-channel digital input/output, "
                       "24 V DC, M12",
        "description": "EtherCAT P box with a dark housing: four M12 sockets "
                       "X01 to X04, two channels each (pin 4 = channel A, "
                       "pin 2 = channel B, pin 1 = +24 V DC Up, pin 3 = "
                       "GND P, pin 5 not connected), and two M8 EtherCAT P "
                       "sockets, X50 IN and X51 OUT. Each M12 socket is ONE "
                       "terminal (X01 to X04) shared by its two channels "
                       "(X01(1), X01(2)...), the channel is the signal of "
                       "the I/O table, not the pin. The positions on the "
                       "housing are assumed. The label of the box is the "
                       "card id (A1.10).",
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
        k.set("y", num(BODY[1] + BODY[3] + 12 if below else BODY[1] - 40))
        desc.append(k)
    patch_io(root)
    write(root, path)


def patch_table(path):
    root = ET.parse(path).getroot()
    patch_io(root)
    write(root, path)


def main(folder):
    box_dir = os.path.join(folder, "epp_box", "epp2338")
    patch(os.path.join(box_dir, "epp2338.elmt"), schematic())
    patch(os.path.join(box_dir, "epp2338_realistic.elmt"), realistic())
    patch_table(os.path.join(box_dir, "epp2338_table.elmt"))
    print("EPP2338 redrawn")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
