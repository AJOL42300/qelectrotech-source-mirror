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
"""Write the EK1322 (2-port EtherCAT P junction with feed-in): the schematic
and the realistic views, and the terminal table.  The description view is
made by gen_description.py (build_ek1322) from the realistic view.

Public facts used (Beckhoff information system, only the facts): the
EK1322 couples EtherCAT P junctions and can be placed anywhere in an
EtherCAT strand, between EtherCAT Terminals.  It has two shielded P-coded M8
sockets, X1 (previous strand) and X2 (next strand), and four spring
terminal points for the feed-in: 1 Us 24 V, 2 Us 0 V, 3 Up 24 V, 4 Up 0 V
(Us = system and sensor supply, Up = peripheral voltage of the actuators).
LEDs: Run, L/A of X1 and X2, Us and Up.

ASSUMED (not public): the position of the terminal points and of the LEDs on
the housing (columns Us on the left and Up on the right, LEDs on top), the
rail between the ports and the feed-in.  No power contact is drawn: the
public documentation does not say that the EK1322 passes them on.

Each M8 socket is ONE terminal (X1, X2), like the connectors of the EPP2338.
The card has no I/O channel: its table lists the function of each terminal
with an empty TwinCAT ID.  Vector drawing only, no logo and no manufacturer
text.

Run it after gen_symbols.py / organize.py (the folder of the card must
exist), before gen_description.py.

Usage: python gen_ek1322.py <extra-elements>/10_electric/20_manufacturers_articles/beckhoff
"""
import copy
import math
import os
import re
import sys
import uuid
import xml.etree.ElementTree as ET

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gen_schematic import (DOTTED, POT_FILLED, POT_STYLE, POT_TEXT,  # noqa
                           THIN, WHITE, Sheet, CLIP_H, CLIP_W)
from realistic_fixes import num, text_width, write  # noqa: E402
import simple_tables  # noqa: E402

NS = uuid.UUID("6f1c2a5e-8b0f-4d57-9a7e-5b1d0c8e2f10")
FAMILY = "ek_couplers"
STEM = "ek1322"
REFERENCE = "EK1322"
DESIGNATION = "EtherCAT P junction, 2 ports, with feed-in"
DESCRIPTION = (
    "EtherCAT P coupler with feed-in: two P-coded M8 sockets, X1 (previous "
    "strand) and X2 (next strand), and four feed-in terminals: 1 Us +24 V "
    "DC (system and sensor supply), 2 Us 0 V, 3 Up +24 V DC (peripheral "
    "voltage), 4 Up 0 V. Each socket is one terminal. The position of the "
    "terminals and of the LEDs (Run, L/A X1, L/A X2, Us, Up) is assumed.")
INFO = ("Author: QElectroTech contributors\nGeneric layout, drawn from the "
        "public description of the product, not copied from a manufacturer "
        "drawing. License: see "
        "http://qelectrotech.org/wiki/doc/elements_license")
FONT = "Roboto Condensed,%d,-1,5,50,0,0,0,0,0,Regular"
FONT_BOLD = "Roboto Condensed,%d,-1,5,75,0,0,0,0,0,Bold"
RED = "line-style:normal;line-weight:thin;filling:red;color:black"
DARK = "line-style:normal;line-weight:thin;filling:gray;color:black"
BLACK = "line-style:normal;line-weight:thin;filling:black;color:black"
LED_STYLE = "line-style:normal;line-weight:thin;filling:green;color:black"
TEXT_COLOR = {"24V": "#ff0000", "0V": "#0000ff"}

# housing: 24 mm wide at the common scale (3.1 px per mm), same height as
# the other cards
BODY = (-32.0, -136.7, 75.0, 277.2)     # x, y, width, height
CX = 5.0                                # axis of the housing
M8_R = 11.0                             # M8 socket
# name -> (x, y, orientation, potential)
PINS = {"X1": (CX, -80, "n", None), "X2": (CX, -30, "s", None),
        "1": (-15, 45, "s", "24V"), "2": (-15, 100, "s", "0V"),
        "3": (25, 45, "s", "24V"), "4": (25, 100, "s", "0V")}
# Run, L/A X1, L/A X2 (left column), Us, Up (right column)
LEDS = {"Run": (-15, -122), "L/A X1": (-15, -111), "L/A X2": (-15, -100),
        "Us": (25, -122), "Up": (25, -111)}
LED_TEXT = {"Run": "Run", "L/A X1": "L/A1", "L/A X2": "L/A2", "Us": "Us",
            "Up": "Up"}
# function of each terminal (the Label of the table), in table order
FUNCTION = (("X1", "X1: EtherCAT P in (M8)"),
            ("X2", "X2: EtherCAT P out (M8)"),
            ("1", "+24 V DC (Us)"), ("2", "0 V (Us)"),
            ("3", "+24 V DC (Up)"), ("4", "0 V (Up)"))
NAMES = {
    "": ("EK1322 EtherCAT P junction with feed-in",
         "EK1322 jonction EtherCAT P avec alimentation"),
    "_realistic": ("EK1322 EtherCAT P junction with feed-in, realistic "
                   "view",
                   "EK1322 jonction EtherCAT P avec alimentation, vue "
                   "r\u00e9aliste"),
    "_table": ("EK1322 EtherCAT P junction - terminal table",
               "EK1322 jonction EtherCAT P - tableau des bornes"),
}


def uid(*parts):
    return "{%s}" % uuid.uuid5(NS, "/".join((STEM,) + parts))


def header(root, suffix, link="simple"):
    root.set("link_type", link)
    root.find("uuid").set("uuid", uid("view", suffix))
    names = root.find("names")
    for n in list(names):
        names.remove(n)
    for lang, text in zip(("en", "fr"), NAMES.get(suffix, NAMES[""])):
        e = ET.SubElement(names, "name", {"lang": lang})
        e.text = text
    infos = root.find("elementInformations")
    for n in list(infos):
        infos.remove(n)
    for key, value in (("manufacturer", "Beckhoff"),
                       ("manufacturer_reference", REFERENCE),
                       ("designation", DESIGNATION),
                       ("description", DESCRIPTION)):
        e = ET.SubElement(infos, "elementInformation",
                          {"name": key, "show": "1"})
        e.text = value


def terminals(desc):
    for name, (x, y, o, pot) in PINS.items():
        e = ET.Element("terminal", {
            "x": num(x), "y": num(y), "name": name, "orientation": o,
            "type": "Generic", "uuid": uid("terminal", name)})
        if pot:
            e.set("potential", pot)
        desc.append(e)


class Draw:
    """Realistic drawing."""

    def __init__(self):
        self.parts = []

    def line(self, x1, y1, x2, y2, style=THIN):
        self.parts.append(ET.Element("line", {
            "end1": "none", "end2": "none", "y1": num(y1),
            "antialias": "false", "y2": num(y2), "style": style,
            "x1": num(x1), "length1": "4.5", "x2": num(x2),
            "length2": "4.5"}))

    def rect(self, x, y, w, h, style=THIN):
        self.parts.append(ET.Element("rect", {
            "x": num(x), "y": num(y), "width": num(w), "height": num(h),
            "antialias": "false", "style": style, "rx": "0", "ry": "0"}))

    def circle(self, cx, cy, d, style=WHITE):
        self.parts.append(ET.Element("circle", {
            "x": num(cx - d / 2), "y": num(cy - d / 2), "antialias": "false",
            "style": style, "diameter": num(d)}))

    def text(self, x, y, s, size=7, bold=False, color="#000000"):
        self.parts.append(ET.Element("text", {
            "x": num(x), "y": num(y), "rotation": "0", "color": color,
            "font": (FONT_BOLD if bold else FONT) % size, "text": s}))

    def text_centered(self, cx, y, s, size=7, bold=False, color="#000000"):
        w = text_width(s) * size / 7.0
        self.text(cx - w / 2, y, s, size, bold, color)


def realistic(template, path):
    root = ET.parse(template).getroot()
    header(root, "_realistic")
    root.find("informations").text = (
        INFO + "\nRealistic view made by misc/beckhoff/gen_ek1322.py: scale "
        "1.00, grid 5 px. License: see "
        "http://qelectrotech.org/wiki/doc/elements_license")
    desc = root.find("description")
    dyn = desc.find("dynamic_text")
    for e in list(desc):
        desc.remove(e)
    d = Draw()
    x, y, w, h = BODY
    d.rect(x, y, w, h, THIN)
    d.line(x, y + 12, x + w, y + 12)
    d.line(x, y + h - 12, x + w, y + h - 12)
    # P-coded M8 sockets, red like the EtherCAT P connectors
    for name in ("X1", "X2"):
        sx, sy, _o, _p = PINS[name]
        d.circle(sx, sy, 2 * M8_R, RED)
        d.circle(sx, sy, 14, DARK)
        for k in range(4):
            a = math.radians(45 + 90 * k)
            d.circle(sx + 4.2 * math.cos(a), sy - 4.2 * math.sin(a), 2.2,
                     BLACK)
        d.text(sx - 27, sy + 2, name, 6)
    d.text_centered(CX, -52, "EtherCAT P", 6, True)
    # LEDs: the left column is labelled on its right, the right one on its
    # left, so that the legends of the description view stay free
    for led, (lx, ly) in LEDS.items():
        d.circle(lx, ly, 4, LED_STYLE)
        t = LED_TEXT[led]
        if lx < CX:
            d.text(lx + 4, ly + 2.2, t, 4)
        else:
            d.text(lx - 4 - text_width(t) * 4 / 7.0, ly + 2.2, t, 4)
    # feed-in terminals: Us on the left, Up on the right
    for name in ("1", "2", "3", "4"):
        px, py, _o, pot = PINS[name]
        d.circle(px, py, 16.2, WHITE)
        d.circle(px, py, 12.6, WHITE)
        mark = "24V" if pot == "24V" else "0V"
        style = POT_FILLED[pot]
        d.rect(px - 5.5, py - 11.2, 11, 1.6, style.replace(
            "line-style:normal", "line-style:none"))
        d.text_centered(px, py - 12.6, mark, 7, True, TEXT_COLOR[pot])
    d.text_centered(-15, 22, "Us", 7, True)
    d.text_centered(25, 22, "Up", 7, True)
    d.text_centered(CX, 130, REFERENCE, 8, True)
    for e in d.parts:
        desc.append(e)
    terminals(desc)
    if dyn is not None:
        dyn.set("uuid", uid("dynamic", "realistic"))
        dyn.set("x", num(BODY[0]))
        desc.append(dyn)
    root.set("width", "84")
    root.set("height", "330")
    root.set("hotspot_x", "38")
    root.set("hotspot_y", "160")
    write(root, path)
    return root


def schematic(real, path):
    root = copy.deepcopy(real)
    header(root, "")
    root.find("informations").text = (
        INFO + "\nSchematic view made by misc/beckhoff/gen_ek1322.py: the "
        "junction between the ports and the feed-in is generic. License: "
        "see http://qelectrotech.org/wiki/doc/elements_license")
    s = Sheet()
    x0, y0, w, h = BODY
    s.rect(x0, y0, w, h, DOTTED)
    for top in (True, False):
        y = y0 - CLIP_H if top else y0 + h
        s.rect(CX - CLIP_W / 2, y, CLIP_W, CLIP_H, THIN)
        inner = y + (2.0 if top else CLIP_H - 2.0)
        s.line(CX - CLIP_W / 2 + 3, inner, CX + CLIP_W / 2 - 3, inner)
    d = 9.0
    r = d / 2
    # EtherCAT P junction between the two ports
    bx0, bx1, by0, by1 = -30.0, 40.0, -62.0, -48.0
    s.rect(bx0, by0, bx1 - bx0, by1 - by0, WHITE)
    s.text_centered(CX, (by0 + by1) / 2, "EtherCAT P", 5)
    for name, y_edge in (("X1", by0), ("X2", by1)):
        px, py, _o, _p = PINS[name]
        s.line(px, py + (r + 2.5 if name == "X1" else -(r + 2.5)),
               px, y_edge)
    # feed-in: Us (left) and Up (right) to the junction
    rail = {"1": -15, "3": 25, "2": -27, "4": 35}
    for name in ("1", "2", "3", "4"):
        px, py, _o, pot = PINS[name]
        st, filled = POT_STYLE[pot], POT_FILLED[pot]
        if name in ("1", "3"):
            s.line(px, py - r, px, by1, st)
        else:
            sign = -1 if px < CX else 1
            s.line(px + sign * r, py, rail[name], py, st)
            s.line(rail[name], py, rail[name], by1, st)
        top_x = rail[name]
        s.circle(top_x, by1, 2.4, filled)
    s.text(-13, -10, "+24 V", 4, color=POT_TEXT["24V"])
    s.text(-25, 15, "0 V", 4, color=POT_TEXT["0V"])
    s.text(27, -10, "+24 V", 4, color=POT_TEXT["24V"])
    s.text(28, 15, "0 V", 4, color=POT_TEXT["0V"])
    s.text(-9.5, 47, "Us", 6, True)
    s.text(10.5, 47, "Up", 6, True)
    for name, (px, py, _o, _p) in PINS.items():
        s.circle(px, py, 14 if name.startswith("X") else d, WHITE)
        s.text_centered(px, py, name, 5)
    s.text_centered(CX, 130, REFERENCE, 8)
    s.parts[-1].set("font", FONT_BOLD % 8)
    old = root.find("description")
    desc = ET.Element("description")
    for e in s.parts:
        desc.append(e)
    terminals(desc)
    dyn = old.find("dynamic_text")
    if dyn is not None:
        dyn.set("uuid", uid("dynamic", "schematic"))
        desc.append(dyn)
    idx = list(root).index(old)
    root.remove(old)
    root.insert(idx, desc)
    xs = [x for x, _ in s.bits] + [p[0] for p in PINS.values()]
    ys = [y for _, y in s.bits] + [p[1] for p in PINS.values()]
    x_min, y_min = min(xs) - 5, min(ys) - 5
    root.set("width", str(int(math.ceil(max(xs) + 5 - x_min))))
    root.set("height", str(int(math.ceil(max(ys) + 5 - y_min))))
    root.set("hotspot_x", str(int(math.ceil(-x_min))))
    root.set("hotspot_y", str(int(math.ceil(-y_min))))
    write(root, path)


def table(template, path):
    """The I/O table: one row per terminal (pin, label, no ID)."""
    with open(template, "r", encoding="utf-8") as f:
        text = f.read()
    names = "".join('\t\t<name lang="%s">%s</name>\n' % (lang, t)
                    for lang, t in zip(("en", "fr"), NAMES["_table"]))
    text = re.sub(r"<names>.*?</names>", "<names>\n" + names + "\t</names>",
                  text, count=1, flags=re.S)
    infos = "".join(
        '\t\t<elementInformation name="%s" show="1">%s</elementInformation>\n'
        % (k, v) for k, v in (("manufacturer", "Beckhoff"),
                              ("manufacturer_reference", REFERENCE),
                              ("designation", DESIGNATION),
                              ("description", DESCRIPTION)))
    text = re.sub(r"<elementInformations>.*?</elementInformations>",
                  "<elementInformations>\n" + infos +
                  "\t</elementInformations>", text, count=1, flags=re.S)
    ios = "".join(
        '\t\t\t\t<plcIO type="entree_universelle" address="" '
        'functionText="%s" comment="" crossRef="" terminalCount="1">\n'
        "\t\t\t\t\t<terminal>%s</terminal>\n\t\t\t\t</plcIO>\n"
        % (label, name) for name, label in FUNCTION)
    text = re.sub(r"<plcIOs>.*?</plcIOs>",
                  lambda m: "<plcIOs>\n" + ios + "\t\t\t</plcIOs>", text,
                  count=1, flags=re.S)
    text = re.sub(r'<uuid uuid="[^"]*"/>',
                  '<uuid uuid="%s"/>' % uid("view", "_table"), text, 1)
    k = [0]

    def retag(m):
        k[0] += 1
        return 'uuid="%s"' % uid("dynamic", "table", str(k[0]))
    text = re.sub(r'(?<=text_from="ElementInfo" )uuid="[^"]*"', retag, text)
    text = re.sub(r"<informations>.*?</informations>",
                  lambda m: "<informations>" + INFO +
                  "\nTable made by misc/beckhoff/gen_ek1322.py. "
                  "</informations>", text, count=1, flags=re.S)
    text = simple_tables.convert(text)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


def main(base):
    folder = os.path.join(base, FAMILY, STEM)
    os.makedirs(folder, exist_ok=True)
    template = os.path.join(base, "el9xxx_system", "el9100",
                            "el9100_realistic.elmt")
    table_template = os.path.join(base, "el1xxx_digital_in", "el1004",
                                  "el1004_table.elmt")
    real = realistic(template, os.path.join(folder, STEM + "_realistic.elmt"))
    schematic(real, os.path.join(folder, STEM + ".elmt"))
    table(table_template, os.path.join(folder, STEM + "_table.elmt"))
    print("ek1322: schematic, realistic and table written")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
