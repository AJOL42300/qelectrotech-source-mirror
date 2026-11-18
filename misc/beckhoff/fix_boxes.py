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
"""Give the EK1914 and the EL1918 the housing height of the other cards.

All the terminal cards are 330 px high (hotspot 160).  Run it after
realistic_fixes.py, before gen_schematic.py / gen_description.py:

  * EK1914: the drawing (220 px high) is stretched to 330 px.  The 8 rows of
    terminals go from a pitch of 15 / 20 px to 25 px, on the 5 px grid; the
    circles keep their size and are centred again on the rows, the numbers
    follow their row (outside of the terminal block, same gap), the yellow
    TwinSAFE block covers the rows 11 to 16.  The strip is stretched row by
    row, the sockets of the left part linearly: the traced drawing is
    distorted, it is not a measured one.
  * EL1918: the standard housing of the EL1904 (60 x 330, the traced
    outline, the rail clips) with 16 terminals: 8 rows of 2 columns with a
    pitch of 25 px, the pins 1 to 8 in the left column and the pins 9 to 16
    in the right one.

Usage: python fix_boxes.py <elements dir>
"""
import copy
import os
import re
import sys
import xml.etree.ElementTree as ET

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from realistic_fixes import box, circle, num, text_width, write  # noqa: E402

HEIGHT = 330
HOTSPOT_Y = 160
OLD_ROWS = (-30, -15, 0, 20, 35, 55, 70, 85)
NEW_ROWS = (-50, -25, 0, 25, 50, 75, 100, 125)
EDGE_SLOPE = 1.35           # above the first row and under the last one
BODY_SLOPE = (NEW_ROWS[-1] - NEW_ROWS[0]) / float(OLD_ROWS[-1] - OLD_ROWS[0])
STRIP_X = 12.0              # the terminal block starts here
LABEL_GAP = 14.0            # label of a terminal: top of the text above it

# 20 px pitch in each S+ / SI pair, in two groups of two pairs: the power
# contact rails of the schematic view (y = 15 and 25) run between the groups,
# so no pair is cut by a rail; all on the 5 px grid
EL1918_ROWS = [-55, -35, -15, 5, 35, 55, 75, 95]
EL1918_X = (-5, 10)
EL1918_RING = 10.0          # a 6 pt label fits between two rings at 20 px
EL1918_INNER = 7.8
EL1918_LABEL_Y = 6.5        # baseline of the label above its ring
EL1918_ORDER = (1, 3, 5, 7, 2, 4, 6, 8)


def stretch_strip(y):
    if y <= OLD_ROWS[0]:
        return NEW_ROWS[0] + (y - OLD_ROWS[0]) * EDGE_SLOPE
    if y >= OLD_ROWS[-1]:
        return NEW_ROWS[-1] + (y - OLD_ROWS[-1]) * EDGE_SLOPE
    for k in range(len(OLD_ROWS) - 1):
        if y <= OLD_ROWS[k + 1]:
            f = (y - OLD_ROWS[k]) / float(OLD_ROWS[k + 1] - OLD_ROWS[k])
            return NEW_ROWS[k] + f * (NEW_ROWS[k + 1] - NEW_ROWS[k])


def stretch_body(y):
    if y <= OLD_ROWS[0] or y >= OLD_ROWS[-1]:
        return stretch_strip(y)
    return NEW_ROWS[0] + (y - OLD_ROWS[0]) * BODY_SLOPE


def snap(v):
    return round(v / 5.0) * 5.0


def pick(x):
    return stretch_strip if x >= STRIP_X else stretch_body


def stretch_part(p):
    t = p.tag
    g = lambda n: float(p.get(n))
    if t == "line":
        m = pick((g("x1") + g("x2")) / 2)
        p.set("y1", num(m(g("y1"))))
        p.set("y2", num(m(g("y2"))))
    elif t in ("rect", "arc", "ellipse"):
        m = pick(g("x") + g("width") / 2)
        y0, y1 = g("y"), g("y") + g("height")
        p.set("y", num(m(y0)))
        p.set("height", num(m(y1) - m(y0)))
    elif t == "circle":
        d = g("diameter")
        m = pick(g("x") + d / 2)
        p.set("y", num(m(g("y") + d / 2) - d / 2))
    elif t == "polygon":
        for k, v in list(p.attrib.items()):
            if k[0] == "y" and k[1:].isdigit():
                p.set(k, num(pick(float(p.get("x" + k[1:])))(float(v))))


def fix_ek1914(path):
    tree = ET.parse(path)
    root = tree.getroot()
    if root.get("height") == str(HEIGHT):
        return False
    desc = root.find("description")
    for p in desc:
        if p.tag in ("line", "rect", "arc", "ellipse", "circle", "polygon"):
            stretch_part(p)
        elif p.tag == "terminal":
            y = float(p.get("y"))
            if p.get("name").isdigit():
                p.set("y", num(stretch_strip(y)))
            else:
                p.set("y", num(snap(stretch_body(y))))
        elif p.tag == "text":
            y = float(p.get("y"))
            if p.get("text").isdigit():
                p.set("y", num(stretch_strip(y + 5.5) - 5.5))
            else:
                p.set("y", num(stretch_body(y)))
        elif p.tag == "dynamic_text":
            p.set("y", num(stretch_body(float(p.get("y")))))
    for t in desc.findall("terminal"):
        if not t.get("name").isdigit():
            continue
        tx, ty = float(t.get("x")), float(t.get("y"))
        for c in desc.findall("circle"):
            d = float(c.get("diameter"))
            cx = float(c.get("x")) + d / 2
            cy = float(c.get("y")) + d / 2
            if abs(cx - tx) < 3 and abs(cy - ty) < 3:
                c.set("x", num(tx - d / 2))
                c.set("y", num(ty - d / 2))
    low = max(box(p)[3] for p in desc if p.tag in
              ("line", "rect", "arc", "ellipse", "circle", "polygon"))
    high = min(box(p)[1] for p in desc if p.tag in
               ("line", "rect", "arc", "ellipse", "circle", "polygon"))
    if high < -HOTSPOT_Y or low > HEIGHT - HOTSPOT_Y:
        raise SystemExit("EK1914: the drawing does not fit in the housing")
    root.set("height", str(HEIGHT))
    root.set("hotspot_y", str(HOTSPOT_Y))
    write(root, path)
    return True


def label_of(pin):
    order = EL1918_ORDER[(pin - 1) // 2]
    return "S%d+" % order if pin % 2 else "SI%d" % order


def fix_el1918(path, donor_path):
    root = ET.parse(path).getroot()
    old = root.find("description")
    terminals = {t.get("name"): t for t in old.findall("terminal")}
    rows = [float(terminals[str(n)].get("y")) for n in range(1, 9)]
    rings = [float(c.get("diameter")) for c in old.findall("circle")]
    if (root.get("height") == str(HEIGHT)
            and rows == [float(y) for y in EL1918_ROWS]
            and rings and rings[0] == EL1918_RING
            and not any("filling:blue" in r.get("style", "")
                        for r in old.findall("rect"))):
        return False
    donor = ET.parse(donor_path).getroot()
    dynamic = old.find("dynamic_text")
    desc = ET.Element("description")
    ref = None
    for p in donor.find("description"):
        if p.tag == "text":
            if p.get("text") == "EL1904":
                ref = copy.deepcopy(p)
                ref.set("text", "EL1918")
            continue
        if p.tag in ("terminal", "circle", "dynamic_text"):
            continue
        # underline of a supply label of the donor: the labels are not kept
        if p.tag == "rect" and re.search(
                r"filling:(red|blue|orange)", p.get("style", "")):
            continue
        b = box(p)
        if p.tag == "line" and b[1] > -82.0 and b[3] < 139.0:
            continue
        desc.append(copy.deepcopy(p))
    # the sides of the terminal zone are one line each
    side = {"end1": "none", "end2": "none", "antialias": "false",
            "style": "line-style:normal;line-weight:thin;filling:none;"
                     "color:black", "length1": "1.5", "length2": "1.5"}
    for x in (-12.6, 23.4):
        desc.append(ET.Element("line", dict(
            side, x1=num(x), y1="-82.4", x2=num(x), y2="139.3")))
    for pin in range(1, 17):
        col, row = divmod(pin - 1, 8)
        x, y = EL1918_X[col], EL1918_ROWS[row]
        desc.append(circle(x, y, EL1918_RING))
        desc.append(circle(x, y, EL1918_INNER))
        label = label_of(pin)
        desc.append(ET.Element("text", {
            "x": num(x - text_width(label) / 2),             "y": num(y - EL1918_LABEL_Y),
            "rotation": "0", "color": "#000000",
                        "font": "Roboto Condensed,6,-1,5,50,0,0,0,0,0,Regular",
            "text": label}))
    desc.append(ref)
    for pin in range(1, 17):
        col, row = divmod(pin - 1, 8)
        t = terminals[str(pin)]
        t.set("x", num(EL1918_X[col]))
        t.set("y", num(EL1918_ROWS[row]))
        desc.append(t)
    dyn_donor = donor.find("description").find("dynamic_text")
    dynamic.set("x", dyn_donor.get("x"))
    dynamic.set("y", dyn_donor.get("y"))
    desc.append(dynamic)
    index = list(root).index(old)
    root.remove(old)
    root.insert(index, desc)
    for k in ("width", "height", "hotspot_x", "hotspot_y"):
        root.set(k, donor.get(k))
    write(root, path)
    return True


def main(base):
    if fix_ek1914(os.path.join(base, "ek_couplers", "ek1914",
                               "ek1914_realistic.elmt")):
        print("EK1914 stretched to the height of the other cards")
    if fix_el1918(os.path.join(base, "twinsafe", "el1918",
                               "el1918_realistic.elmt"),
                  os.path.join(base, "twinsafe", "el1904",
                               "el1904_realistic.elmt")):
        print("EL1918 redrawn in the standard housing, 16 terminals")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
