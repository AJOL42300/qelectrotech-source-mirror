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
YELLOW_RECT = (15.5, 45.4, 39.5, 89.6)   # the TwinSAFE rows, pins 11 to 16
FONT_PX = 7 * 4.0 / 3.0      # label font, 7 pt
CIRCLE_STYLE = "line-style:normal;line-weight:thin;filling:none;color:black"
YELLOW_STYLE = "line-style:normal;line-weight:normal;filling:yellow;color:black"
RING_D = 16.2
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
        if pin % 2:
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


def main():
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    base = sys.argv[1]
    ek = os.path.join(base, "ek_couplers", "ek1914_realistic.elmt")
    fix_ek1914(ek)
    fix_el1918(os.path.join(base, "twinsafe", "el1918_realistic.elmt"), ek)
    for f in sorted(glob.glob(os.path.join(base, "*", "*_realistic.elmt"))):
        if simplify_terminals(f):
            print("simplified", os.path.relpath(f, base))


if __name__ == "__main__":
    main()
