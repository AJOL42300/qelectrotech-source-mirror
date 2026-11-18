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
"""Corrections applied in place to the realistic views (idempotent).

Run it on extra-elements/15_beckhoff_ethercat after realistic_from_model.py
and organize.py, and before gen_description.py:

  * a terminal is only a circle (outer and inner ring), without the notch
    or the little square above it; its label is centred above the circle
    (EL60 family, EL1904/EL2904, EL2911);
  * ek1914_realistic.elmt
      - the little squares above the circles are removed;
      - the numbers of the 16 terminals sit just outside the terminal block,
        next to their own circle (odd pins left of the block, even pins
        right of it);
      - the whole bottom block (TwinSAFE, pins 11 to 16) is yellow, not only
        its circles;
  * el1918_realistic.elmt
      - the 16 terminals are laid out like the right part of the EK1914:
        two columns, same pitch, same circles, numbered row by row.

The drives and the EPP2338 already draw their terminals as plain circles with
the label beside them: they are left unchanged.

Usage:
  realistic_fixes.py <elements dir>
"""
import glob
import os
import sys
import xml.etree.ElementTree as ET

NUM_GAP = 2.5       # between the terminal block and a number
NUM_TOP = 5.5       # a number is drawn this far above its row
BLOCK = (15.5, 39.5)         # terminal block of the EK1914 (x)
BLOCK_TOP = -39.8            # top of the first row of terminals
YELLOW_RECT = (15.5, 63.0, 39.5, 131.21)   # the TwinSAFE rows, pins 6-8, 14-16
FONT_PX = 7 * 4.0 / 3.0      # label font, 7 pt
CIRCLE_STYLE = "line-style:normal;line-weight:thin;filling:none;color:black"
YELLOW_STYLE = "line-style:normal;line-weight:normal;filling:yellow;color:black"
RING_D = 16.2
# EK1914 manual, table of the terminal points: the channel of each pin
EK1914_CHANNEL_PINS = {"I1": "2", "I2": "10", "I3": "3", "I4": "11",
                       "O1": "4", "O2": "12", "O3": "5", "O4": "13",
                       "SI1": "7", "SI2": "15", "SO1": "8", "SO2": "16"}
INNER_D = 12.6
PARTS = ("line", "arc", "circle", "rect", "polygon", "ellipse")


def num(v):
    v = round(v, 2)
    return str(int(v)) if v == int(v) else ("%.2f" % v).rstrip("0")


def box(p):
    t = p.tag
    g = lambda n: float(p.get(n))
    if t in ("rect", "arc", "ellipse"):
        x, y, w, h = g("x"), g("y"), g("width"), g("height")
        return min(x, x + w), min(y, y + h), max(x, x + w), max(y, y + h)
    if t == "circle":
        d = g("diameter")
        return g("x"), g("y"), g("x") + d, g("y") + d
    if t == "line":
        return (min(g("x1"), g("x2")), min(g("y1"), g("y2")),
                max(g("x1"), g("x2")), max(g("y1"), g("y2")))
    if t == "polygon":
        xs = [float(v) for k, v in p.attrib.items()
              if k[0] == "x" and k[1:].isdigit()]
        ys = [float(v) for k, v in p.attrib.items()
              if k[0] == "y" and k[1:].isdigit()]
        return min(xs), min(ys), max(xs), max(ys)
    return None


def inside(b, x0, y0, x1, y1):
    return b[0] >= x0 and b[1] >= y0 and b[2] <= x1 and b[3] <= y1


def text_width(text):
    """Width of a label in Roboto Condensed (about, in pixels)."""
    narrow = {"I": 0.25, "1": 0.40, "l": 0.25, "i": 0.25, " ": 0.22,
              "-": 0.30, "/": 0.30}
    return sum(narrow.get(c, 0.5 if c.isalpha() else 0.49)
               for c in text) * FONT_PX


def indent(elem, level=0):
    pad = "\n" + "\t" * level
    if len(elem):
        elem.text = pad + "\t"
        for child in elem:
            indent(child, level + 1)
            child.tail = pad + "\t"
        child.tail = pad
    else:
        elem.tail = pad


def write(root, path):
    indent(root)
    ET.ElementTree(root).write(path, encoding="utf-8", xml_declaration=False)
    with open(path, "r", encoding="utf-8") as fh:
        data = fh.read()
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(data if data.endswith("\n") else data + "\n")


def circle(cx, cy, d):
    return ET.Element("circle", {
        "x": num(cx - d / 2), "y": num(cy - d / 2), "antialias": "false",
        "style": CIRCLE_STYLE, "diameter": num(d)})


def simplify_terminals(path):
    """Plain circle terminals with the label centred above (EL60 style)."""
    root = ET.parse(path).getroot()
    desc = root.find("description")
    if not any(abs(float(a.get("width")) - RING_D) < 0.3
               for a in desc.findall("arc")):
        return False
    texts = desc.findall("text")
    removed = []
    placed = []
    left = min(float(t.get("x")) for t in desc.findall("terminal")) - 8.1
    for t in desc.findall("terminal"):
        tx, ty = float(t.get("x")), float(t.get("y"))
        for p in list(desc):
            b = box(p) if p.tag in PARTS else None
            if b is None or p in removed:
                continue
            ring = p.tag == "arc" and b[2] - b[0] > 12 and \
                abs((b[0] + b[2]) / 2 - tx) < 0.4 and \
                abs((b[1] + b[3]) / 2 - ty) < 0.4
            notch = p.tag in ("arc", "polygon") and \
                inside(b, tx - 4.4, ty - 10.6, tx + 4.4, ty - 2.0) and \
                b[2] - b[0] < 8.2
            detail = p.tag in ("line", "polygon") and \
                inside(b, tx - 8.3, ty - 8.3, tx + 8.3, ty + 8.3) and \
                b[2] - b[0] < 7 and b[3] - b[1] < 7
            frame = p.tag == "line" and \
                inside(b, tx - 6.0, ty - 25.5, tx + 6.5, ty - 9.5)
            if ring or notch or detail or frame:
                removed.append(p)
        best = None
        for lab in texts:
            if not ty - 28 < float(lab.get("y")) < ty - 14:
                continue
            cx = float(lab.get("x")) + text_width(lab.get("text")) / 2
            if abs(cx - tx) < 12:
                best = lab
        if best is not None:
            w = text_width(best.get("text"))
            best.set("x", num(max(tx - w / 2, left)))
            best.set("y", num(ty - 22))
            placed.append(best)
    # labels that would touch their neighbour are staggered one line up
    for lab in sorted(placed, key=lambda l: float(l.get("x"))):
        for other in placed:
            if other is lab or other.get("y") != lab.get("y"):
                continue
            x0 = float(other.get("x"))
            if x0 < float(lab.get("x")) and x0 + text_width(
                    other.get("text")) + 1 > float(lab.get("x")):
                lab.set("y", num(float(lab.get("y")) - FONT_PX - 1))
                break
    first = next((i for i, p in enumerate(desc)
                  if p.tag in ("terminal", "text", "dynamic_text")), len(desc))
    for p in removed:
        desc.remove(p)
    first -= len(removed)
    for t in desc.findall("terminal"):
        tx, ty = float(t.get("x")), float(t.get("y"))
        desc.insert(first, circle(tx, ty, RING_D))
        desc.insert(first, circle(tx, ty, INNER_D))
        first += 2
    write(root, path)
    return True


def fix_ek1914(path):
    root = ET.parse(path).getroot()
    desc = root.find("description")
    terminals = {t.get("name"): t for t in desc.findall("terminal")}
    # little squares above the circles; the top edge of each one stays as
    # the edge of the cell
    circles = [p for p in desc.findall("circle")
               if abs(float(p.get("diameter")) - 8.5) < 0.1]
    for c in circles:
        cx = float(c.get("x")) + 4.25
        cy = float(c.get("y")) + 4.25
        lines = [p for p in desc.findall("line")
                 if inside(box(p), cx - 3.6, cy - 11, cx + 3.6, cy - 3.5)]
        if not lines:
            continue
        top = min(box(p)[1] for p in lines)
        for p in lines:
            b = box(p)
            if abs(b[1] - top) < 0.05 and abs(b[3] - top) < 0.05 \
                    and 4.8 < b[2] - b[0] < 5.6:
                continue
            desc.remove(p)
    # numbers next to their circle, outside the terminal block
    for t in desc.findall("text"):
        name = t.get("text")
        if not name.isdigit() or name not in terminals:
            continue
        pin = int(name)
        yc = float(terminals[name].get("y"))
        if pin <= 8:
            x = BLOCK[0] - NUM_GAP - text_width(name)
        else:
            x = BLOCK[1] + NUM_GAP
        t.set("x", num(x))
        t.set("y", num(yc - NUM_TOP))
    # the bottom block is yellow
    x0, y0, x1, y1 = YELLOW_RECT
    wanted = {"x": num(x0), "y": num(y0), "width": num(x1 - x0),
              "height": num(y1 - y0)}
    if not any(all(p.get(k) == v for k, v in wanted.items())
               for p in desc.findall("rect")):
        rect = ET.Element("rect", dict(
            wanted, antialias="false", style=YELLOW_STYLE, rx="0", ry="0"))
        desc.insert(0, rect)
    write(root, path)


def renumber_ek1914(path, symbol_path):
    """The terminals of the EK1914 are numbered down the left column (1 to
    8) and then down the right one (9 to 16), as in the manual of the
    coupler; the drawing had them row by row. Renames the terminals and the
    numbers, puts +24 V on pin 1 and 0 V on pin 9, and moves the channels of
    the symbol to the pins of the manual."""
    root = ET.parse(path).getroot()
    desc = root.find("description")
    terminals = {t.get("name"): t for t in desc.findall("terminal")}
    if terminals["1"].get("potential"):
        return False

    def new(name):
        k = int(name) - 1
        return str(k % 2 * 8 + k // 2 + 1)

    for t in desc.findall("text"):
        if t.get("text").isdigit() and t.get("text") in terminals:
            t.set("text", new(t.get("text")))
    names = {n: new(n) for n in terminals if n.isdigit()}
    for old, t in list(terminals.items()):
        if old in names:
            t.set("name", names[old])
    for t in desc.findall("terminal"):
        if t.get("name") == "1":
            t.set("potential", "24V")
        elif t.get("name") == "9":
            t.set("potential", "0V")
    write(root, path)
    sym = ET.parse(symbol_path).getroot()
    for io in sym.iter("plcIO"):
        for t in io.findall("terminal"):
            if t.text in names:
                t.text = EK1914_CHANNEL_PINS[io.get("address")]
    write(sym, symbol_path)
    return True


def terminal_rows(desc):
    return sorted({float(t.get("y")) for t in desc.findall("terminal")
                   if t.get("name").isdigit()})


def fix_el1918(path, ek_path):
    ek = ET.parse(ek_path).getroot().find("description")
    rows = terminal_rows(ek)
    root = ET.parse(path).getroot()
    desc = root.find("description")
    old_rows = terminal_rows(desc)
    if len(rows) != 8 or len(old_rows) != 8:
        raise SystemExit("unexpected number of rows of terminals")
    row_of = {y: i for i, y in enumerate(old_rows)}
    # the geometry of the terminal block of the EK1914
    block = []
    for p in ek:
        b = box(p) if p.tag in PARTS else None
        if b is not None and inside(b, BLOCK[0] - 0.5, BLOCK_TOP - 0.1,
                                    BLOCK[1] + 0.5, YELLOW_RECT[3] + 0.1):
            block.append(ET.fromstring(ET.tostring(p)))
    texts = []
    for t in desc.findall("text"):
        if t.get("text") == "EL1918":
            texts.append((t, None))
            continue
        y = float(t.get("y"))
        texts.append((t, min(range(8), key=lambda i: abs(old_rows[i] - 5 - y))))
    for p in [p for p in desc if p.tag in PARTS]:
        desc.remove(p)
    left, right = BLOCK[0] - 28.5, BLOCK[1] + 23.5
    top, bottom = rows[0] - 14, rows[-1] + 26
    desc.insert(0, ET.Element("rect", {
        "antialias": "false", "style": YELLOW_STYLE, "x": num(left),
        "y": num(top), "width": num(right - left),
        "height": num(bottom - top), "rx": "0", "ry": "0"}))
    for i, p in enumerate(block):
        desc.insert(1 + i, p)
    for t, k in texts:
        t.set("y", num(bottom - 14 if k is None else rows[k] - 5))
    for t in desc.findall("terminal"):
        t.set("y", num(rows[row_of[float(t.get("y"))]]))
    for d in desc.findall("dynamic_text"):
        d.set("y", num(bottom + 4))
    top_edge = int(min(top, -50))
    root.set("hotspot_y", str(-top_edge))
    root.set("height", str(int(bottom + 16) - top_edge))
    write(root, path)


def label_font(size):
    return "Roboto Condensed,%d,-1,5,50,0,0,0,0,0,Regular" % size


def set_label(t, text, tx, ty, size, up):
    """Label centred on the terminal column, baseline `up` above it."""
    t.set("text", text)
    t.set("font", label_font(size))
    t.set("x", num(tx - text_width(text) * size / 7.0 / 2))
    t.set("y", num(ty - up))


def tight_labels(path):
    """EL1918: the rows are only 25 apart, the labels of the usual size
    would touch the circle of the row above. 6 pt labels, baseline 10 px
    above the terminal (idempotent)."""
    root = ET.parse(path).getroot()
    desc = root.find("description")
    terminals = [(float(t.get("x")), float(t.get("y")))
                 for t in desc.findall("terminal")]
    for lab in desc.findall("text"):
        if lab.get("text") == "EL1918":
            continue
        size = int(lab.get("font").split(",")[1])
        cx = float(lab.get("x")) + text_width(lab.get("text")) * size / 7.0 / 2
        ly = float(lab.get("y"))
        near = [(ty - ly, tx, ty) for tx, ty in terminals
                if abs(cx - tx) < 8 and 0 < ty - ly < 20]
        if not near:
            continue
        _, tx, ty = min(near)
        set_label(lab, lab.get("text"), tx, ty, 6, 10)
    write(root, path)


# EL2911, Beckhoff manual v2.2.0 (Connection): pins 1 to 8 are the inputs
# (+ = clock output, - = safe input), 2' and 6' the safe output 1, 3' and
# 7' GND UP, 4' and 8' 24 V UP; 1' and 5' are not used.
EL2911_LABELS = {1: "S1+", 2: "SI1", 3: "S3+", 4: "SI3", 5: "S2+", 6: "SI2",
                 7: "S4+", 8: "SI4", 10: "SO1", 14: "SO1", 11: "0V",
                 15: "0V", 12: "24V", 16: "24V"}


def fix_el2911(path, symbol_path):
    """Labels and potentials of the EL2911 from the manual; the drawing's
    own legends are removed. The data are those of the symbol."""
    root = ET.parse(path).getroot()
    symbol = ET.parse(symbol_path).getroot()
    for tag in ("elementInformations", "kindInformations"):
        old, new = root.find(tag), symbol.find(tag)
        if old is not None and new is not None:
            root[list(root).index(old)] = ET.fromstring(ET.tostring(new))
    desc = root.find("description")
    sym_pot = {t.get("name"): t.get("potential")
               for t in symbol.find("description").iter("terminal")}
    terminals = {}
    for t in desc.findall("terminal"):
        name = t.get("name")
        terminals[int(name)] = t
        if sym_pot.get(name):
            t.set("potential", sym_pot[name])
        else:
            t.attrib.pop("potential", None)
    model = None
    for lab in list(desc.findall("text")):
        if lab.get("text") == "EL2911":
            continue
        if model is None:
            model = lab
        desc.remove(lab)
    index = max(list(desc).index(t) for t in desc.findall("text")) + 1 \
        if desc.findall("text") else 0
    for pin, text in sorted(EL2911_LABELS.items()):
        t = ET.Element("text", dict(model.attrib))
        set_label(t, text, float(terminals[pin].get("x")),
                  float(terminals[pin].get("y")), 6, 12)
        desc.insert(index, t)
        index += 1
    write(root, path)


LABEL_SHIFT = 10.0   # labels end 3 px above the circle instead of 13


def closer_labels(path):
    """Move the terminal labels down to a small constant gap above their
    circle. Labels still at the former offsets (22 or 32.3 above the
    terminal) are moved, so the pass can safely run twice."""
    root = ET.parse(path).getroot()
    desc = root.find("description")
    circles = [c for c in desc.findall("circle")
               if abs(float(c.get("diameter")) - RING_D) < 0.3]
    terminals = desc.findall("terminal")
    if not terminals or len(circles) < len(terminals):
        return False
    moved = False
    for lab in desc.findall("text"):
        ly = float(lab.get("y"))
        for t in terminals:
            tx, ty = float(t.get("x")), float(t.get("y"))
            cx = float(lab.get("x")) + text_width(lab.get("text")) / 2
            if abs(cx - tx) > 12:
                continue
            if abs(ly - (ty - 22)) < 0.05 or abs(ly - (ty - 32.3)) < 0.05:
                lab.set("y", num(ly + LABEL_SHIFT))
                moved = True
                break
    if moved:
        write(root, path)
    return moved


def main():
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    base = sys.argv[1]
    ek = os.path.join(base, "ek_couplers", "ek1914", "ek1914_realistic.elmt")
    renumber_ek1914(ek, os.path.join(base, "ek_couplers", "ek1914",
                                     "ek1914.elmt"))
    fix_ek1914(ek)
    safe = os.path.join(base, "twinsafe")
    fix_el1918(os.path.join(safe, "el1918", "el1918_realistic.elmt"), ek)
    for f in sorted(glob.glob(os.path.join(base, "*", "*",
                                           "*_realistic.elmt"))):
        if simplify_terminals(f):
            print("simplified", os.path.relpath(f, base))
        if closer_labels(f):
            print("labels closer", os.path.relpath(f, base))
    tight_labels(os.path.join(safe, "el1918", "el1918_realistic.elmt"))
    fix_el2911(os.path.join(safe, "el2911", "el2911_realistic.elmt"),
               os.path.join(safe, "el2911", "el2911.elmt"))


if __name__ == "__main__":
    main()
