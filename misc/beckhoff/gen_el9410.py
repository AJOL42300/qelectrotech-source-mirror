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
"""Write the EL9410 (power feed terminal with diagnostics): the schematic
and the realistic views, and the I/O table.  The description view is made by
gen_description.py (build_el9410) from the realistic view.

Vector drawing made from a reference picture of the user, with no logo and no
manufacturer text.  Terminal points (2 columns of 4, numbered column after
column like the other terminals):

  1 +24 V DC (Us, E-bus supply)      5 0 V (GND, E-bus supply)
  2 Infeed 24 V (Up) -> contacts     6 Infeed 24 V (Up) -> contacts
  3 Infeed 0 V (Up)  -> contacts     7 Infeed 0 V (Up)  -> contacts
  4 PE -> contact                    8 PE -> contact

The three power contacts (24 V, 0 V, PE) pass through the housing.  The card
has no I/O channel: its table lists the function of each terminal point with
an empty TwinCAT ID (the rows are universal I/O entries only to make the
table displayable, they carry no address).  The pin numbering, the contacts and
the LEDs are taken from the picture and are generic: check them against the
datasheet.

The realistic view reuses the housing of the el9100_realistic view; run it
after gen_symbols.py (which creates el9100) and before gen_schematic.py,
gen_description.py and simple_tables.py.

Usage: python gen_el9410.py <extra-elements>/10_electric/20_manufacturers_articles/beckhoff
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
                           POT_WHITE, THIN, WHITE, Sheet, Frame, EL60_BODY,
                           CLIP_H, CLIP_W)
from realistic_fixes import num, text_width, write  # noqa: E402
import simple_tables  # noqa: E402

NS = uuid.UUID("6f1c2a5e-8b0f-4d57-9a7e-5b1d0c8e2f10")
FAMILY = "el9xxx_system"
STEM = "el9410"
REFERENCE = "EL9410"
DESIGNATION = "EtherCAT Terminal, power feed terminal with diagnostics"
DESCRIPTION = (
    "EtherCAT Terminal, power feed terminal with diagnostics. Pins as drawn "
    "after the user's reference picture (left column 1 to 4, right column "
    "5 to 8): 1 +24 V DC (Us), 5 0 V; 2 and 6 Infeed 24 V, 3 and 7 Infeed "
    "0 V, 4 and 8 PE, the last three pairs linked to the power contacts. "
    "The card has no I/O channel.")
INFO = ("Author: QElectroTech contributors\nGeneric layout redrawn from a "
        "reference picture, not copied from a manufacturer drawing. License: "
        "see http://qelectrotech.org/wiki/doc/elements_license")
ORANGE = "orange"
PE_STYLE = ("line-style:normal;line-weight:normal;filling:none;color:orange")
PE_FILLED = ("line-style:normal;line-weight:thin;filling:orange;"
             "color:orange")
PE_WHITE = ("line-style:normal;line-weight:normal;filling:white;"
            "color:orange")
PE_TEXT = "#b87800"
PE_FILL_REAL = "line-style:none;line-weight:thin;filling:orange;color:orange"
FONT = "Roboto Condensed,%d,-1,5,50,0,0,0,0,0,Regular"
FONT_BOLD = "Roboto Condensed,%d,-1,5,75,0,0,0,0,0,Bold"
MINUS = "\u2212"

LEFT_X, RIGHT_X = -5, 10
ROWS = (-45, 5, 60, 115)
# name -> (x, y, orientation, potential, label, colour, function)
PINS = {}
for _row, (_l, _r) in enumerate((("1", "5"), ("2", "6"), ("3", "7"),
                                 ("4", "8"))):
    PINS[_l] = (LEFT_X, ROWS[_row], "w")
    PINS[_r] = (RIGHT_X, ROWS[_row], "e")
# marks on the housing: text, colour of the mark, potential
MARKS = {"1": ("24V", "24V"), "5": ("0V", "0V"), "2": ("+", "24V"),
         "6": ("+", "24V"), "3": (MINUS, "0V"), "7": (MINUS, "0V"),
         "4": ("PE", "PE"), "8": ("PE", "PE")}
# function of each terminal point (the Label of the table)
FUNCTION = {"1": "+24 V DC (Us)", "5": "0 V (GND)",
            "2": "Infeed 24 V (Up)", "6": "Infeed 24 V (Up)",
            "3": "Infeed 0 V (Up)", "7": "Infeed 0 V (Up)",
            "4": "PE", "8": "PE"}
TEXT_COLOR = {"24V": "#ff0000", "0V": "#0000ff", "PE": PE_TEXT}
POT_OF = {"1": "24V", "2": "24V", "6": "24V", "5": "0V", "3": "0V",
          "7": "0V"}
NAMES = {
    "": ("EL9410 power feed terminal with diagnostics",
         "EL9410 borne d'alimentation avec diagnostic"),
    "_realistic": ("EL9410 power feed terminal with diagnostics, realistic "
                   "view",
                   "EL9410 borne d'alimentation avec diagnostic, vue "
                   "r\u00e9aliste"),
    "_table": ("EL9410 power feed terminal - terminal table",
               "EL9410 borne d'alimentation - tableau des bornes"),
}


def uid(*parts):
    return "{%s}" % uuid.uuid5(NS, "/".join((STEM,) + parts))


def header(root, suffix, link="simple"):
    """Names, uuid, information of the view."""
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
    for name, (x, y, o) in PINS.items():
        e = ET.Element("terminal", {
            "x": num(x), "y": num(y), "name": name, "orientation": o,
            "type": "Generic", "uuid": uid("terminal", name)})
        if name in POT_OF:
            e.set("potential", POT_OF[name])
        desc.append(e)


def realistic(template, path):
    root = ET.parse(template).getroot()
    header(root, "_realistic")
    root.find("informations").text = (
        INFO + "\nRealistic view made by misc/beckhoff/gen_el9410.py: "
        "housing of the other terminals, marks and labels as in the "
        "reference picture. License: see "
        "http://qelectrotech.org/wiki/doc/elements_license")
    desc = root.find("description")
    for e in list(desc):
        if e.tag in ("text", "terminal"):
            desc.remove(e)
    dyn = desc.find("dynamic_text")
    first = list(desc).index(dyn) if dyn is not None else len(desc)
    new = []
    # coloured marks and labels above the circles
    for name, (x, y, _o) in PINS.items():
        text, pot = MARKS[name]
        size = 7
        w = text_width(text) * size / 7.0
        style = {"24V": POT_FILLED["24V"], "0V": POT_FILLED["0V"],
                 "PE": PE_FILL_REAL}[pot]
        new.append(ET.Element("rect", {
            "x": num(x - 5.5), "y": num(y - 11.2), "width": "11",
            "height": "1.6", "rx": "0", "ry": "0", "style": style,
            "antialias": "false"}))
        new.append(ET.Element("text", {
            "x": num(x - w / 2), "y": num(y - 12.6), "rotation": "0",
            "color": TEXT_COLOR[pot],
            "font": FONT_BOLD % size, "text": text}))
    new.append(ET.Element("text", {
        "x": "-10.7", "y": "135.74", "rotation": "0", "color": "#000000",
        "font": FONT_BOLD % 8, "text": REFERENCE}))
    for e in new:
        desc.insert(first, e)
        first += 1
    terminals(desc)
    if dyn is not None:
        dyn.set("uuid", uid("dynamic", "realistic"))
    write(root, path)
    return root


def schematic(real, path):
    root = copy.deepcopy(real)
    header(root, "")
    root.find("informations").text = (
        INFO + "\nSchematic view made by misc/beckhoff/gen_el9410.py: the "
        "supply, the infeeds and the power contacts are generic, check them "
        "against the datasheet. License: see "
        "http://qelectrotech.org/wiki/doc/elements_license")
    s = Sheet()
    x0, y0, x1, y1 = EL60_BODY
    s.rect(x0, y0, x1 - x0, y1 - y0, DOTTED)
    cx = (x0 + x1) / 2 - 2.0
    for top in (True, False):
        y = y0 - CLIP_H if top else y1
        s.rect(cx - CLIP_W / 2, y, CLIP_W, CLIP_H, THIN)
        inner = y + (2.0 if top else CLIP_H - 2.0)
        s.line(cx - CLIP_W / 2 + 3, inner, cx + CLIP_W / 2 - 3, inner)
    d = 9.0
    r = d / 2
    style = {"24V": (POT_STYLE["24V"], POT_FILLED["24V"], POT_WHITE["24V"],
                     POT_TEXT["24V"]),
             "0V": (POT_STYLE["0V"], POT_FILLED["0V"], POT_WHITE["0V"],
                    POT_TEXT["0V"]),
             "PE": (PE_STYLE, PE_FILLED, PE_WHITE, PE_TEXT)}
    # Us / E-bus supply block above the first row
    bx0, bx1, by0, by1 = -12.0, 17.0, -122.0, -92.0
    s.rect(bx0, by0, bx1 - bx0, by1 - by0, WHITE)
    s.text_centered((bx0 + bx1) / 2, by0 + 10, "E-bus", 5)
    s.text_centered((bx0 + bx1) / 2, by0 + 18, "Us", 5)
    s.text(bx0 + 2, by1 - 3, "24V", 4, color=POT_TEXT["24V"])
    s.text(bx1 - 8, by1 - 3, "0V", 4, color=POT_TEXT["0V"])
    for name, pot in (("1", "24V"), ("5", "0V")):
        x, y, _o = PINS[name]
        s.line(x, y - r, x, by1, style[pot][0])
        s.junction(x, by1, pot)
    # the links between the two pins of a row and the power contacts
    for row, pot in ((1, "24V"), (2, "0V"), (3, "PE")):
        y = ROWS[row]
        st, filled, white, _t = style[pot]
        s.line(LEFT_X + r, y, RIGHT_X - r, y, st)
        mx = (LEFT_X + RIGHT_X) / 2
        s.circle(mx, y, 2.4, filled)
        s.line(x0 + 2.5, y, LEFT_X - r, y, st)
        s.rect(x0 - 2.5, y - 2.5, 5, 5, filled)
        s.line(RIGHT_X + r, y, x1 - 2.5, y, st)
        s.circle(x1, y, 5, white)
    # earth symbol of the PE link (up from the link, ochre)
    f = Frame((LEFT_X + RIGHT_X) / 2, ROWS[3], 0, -1)
    s.line(*f.p(0), *f.p(9), PE_STYLE)
    for i, half in enumerate((4.5, 3.0, 1.5)):
        s.line(*f.p(9 + 2.2 * i, -half), *f.p(9 + 2.2 * i, half), PE_STYLE)
    s.text(RIGHT_X - 2, ROWS[3] - 16, "PE", 4, color=PE_TEXT)
    for name, (x, y, _o) in PINS.items():
        s.circle(x, y, d, WHITE)
        s.text_centered(x, y, name, 5)
    s.parts.append(ET.fromstring(
        '<text x="-10.7" y="135.74" rotation="0" color="#000000" '
        'font="%s" text="%s"/>' % (FONT_BOLD % 8, REFERENCE)))
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
    """The I/O table: one row per terminal point (pin, label, no ID)."""
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
        % (FUNCTION[n], n) for n in sorted(PINS, key=int))
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
                  "\nTable made by misc/beckhoff/gen_el9410.py. "
                  "</informations>", text, count=1, flags=re.S)
    text = simple_tables.convert(text)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


def main(base):
    folder = os.path.join(base, FAMILY, STEM)
    os.makedirs(folder, exist_ok=True)
    template = os.path.join(base, FAMILY, "el9100", "el9100_realistic.elmt")
    table_template = os.path.join(base, "el1xxx_digital_in", "el1004",
                                  "el1004_table.elmt")
    real = realistic(template, os.path.join(folder, STEM + "_realistic.elmt"))
    schematic(real, os.path.join(folder, STEM + ".elmt"))
    table(table_template, os.path.join(folder, STEM + "_table.elmt"))
    print("el9410: schematic, realistic and table written")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
