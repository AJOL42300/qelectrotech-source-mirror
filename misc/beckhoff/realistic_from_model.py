#!/usr/bin/env python3
# Copyright 2006 The QElectroTech Team
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
"""Build the realistic appearance of Beckhoff cards from your own drawings.

The drawings are elements that you drew yourself (for instance the ones
embedded in a project) and extracted as .elmt files. They are never copied
into the repository: this script reads them and writes new elements, with:

  * one terminal exactly at the centre of each terminal circle of the
    drawing (the original terminals are replaced);
  * the whole drawing scaled by one uniform factor, with an origin offset,
    so that the centres of the circles fall on the 10 px grid of the
    diagram (5 px if 10 px is not possible, the residual is reported);
  * the elementInformations and the I/O table of the symbol of the same
    card, so that every appearance carries the same data.

Usage:
  realistic_from_model.py <drawings dir> <symbols dir> <output dir>

<drawings dir> holds <name>.elmt.elmt files, <symbols dir> is
extra-elements/15_beckhoff_ethercat (made by gen_symbols.py). The report
(scale, offset, residual) goes to the standard output.
"""
import math
import os
import sys
import uuid
import xml.etree.ElementTree as ET

NS = uuid.UUID("6f1c2a5e-8b0f-4d57-9a7e-5b1d0c8e2f10")

# key of the card -> (name of the drawing without extension, English name,
# French name, names of the terminals that are not on a circle)
CARDS = {
    "el1008": ("el-1008-8-entrees19-12-24", "EL1008 digital input terminal",
               "EL1008 borne d'entrée numérique", ["X1"]),
    "el2008": ("el2008", "EL2008 digital output terminal",
               "EL2008 borne de sortie numérique", ["X1"]),
    "el5152": ("entrees-codeur-ab", "EL5152 encoder input terminal",
               "EL5152 borne d'entrée codeur", []),
    "ek1914": ("ek-1914", "EK1914 EtherCAT Coupler",
               "EK1914 coupleur EtherCAT", ["X000", "X001"]),
}

MIN_CIRCLE = 4         # smaller circles are decorative (LEDs)
NAME_RADIUS = 20       # a circle is named by an integer text this close
MAX_RESIDUAL = 1.5     # above, the 5 px grid is tried


def uid(*parts):
    return "{%s}" % uuid.uuid5(NS, "/".join(parts))


def num(v):
    v = round(v, 2)
    return str(int(v)) if v == int(v) else ("%.2f" % v).rstrip("0")


def circles_of(root):
    out = []
    for c in root.find("description").iter("circle"):
        d = float(c.get("diameter"))
        if d >= MIN_CIRCLE:
            out.append((float(c.get("x")) + d / 2,
                        float(c.get("y")) + d / 2, c))
    return out


def name_circles(root, circles):
    """Name each circle after the nearest integer text (greedy, one to one)."""
    texts = []
    for t in root.find("description").iter("text"):
        s = t.get("text", "").strip()
        if s.isdigit():
            texts.append((float(t.get("x")) + 3, float(t.get("y")) + 5, s))
    pairs = sorted((math.hypot(cx - tx, cy - ty), i, j)
                   for i, (cx, cy, _) in enumerate(circles)
                   for j, (tx, ty, _s) in enumerate(texts))
    names, used = {}, set()
    for dist, i, j in pairs:
        if dist > NAME_RADIUS or i in names or j in used:
            continue
        names[i] = texts[j][2]
        used.add(j)
    for i in range(len(circles)):
        if i not in names:
            names[i] = "C%d" % (i + 1)
    return names


def residual(values, s, offset, grid):
    worst = 0.0
    for v in values:
        p = s * v + offset
        worst = max(worst, abs(p - round(p / grid) * grid))
    return worst


def best_offset(values, s, grid):
    best = (1e9, 0.0)
    steps = int(grid * 4)
    for k in range(steps):
        off = k * grid / steps
        best = min(best, (residual(values, s, off, grid), off))
    return best


def fit(points, grid):
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    candidates = []
    for k in range(80, 141):
        s = k / 100.0
        rx, ox = best_offset(xs, s, grid)
        ry, oy = best_offset(ys, s, grid)
        candidates.append((max(rx, ry), abs(s - 1), s, ox, oy))
    low = min(c[0] for c in candidates)
    near = [c for c in candidates if c[0] <= low + 0.25]
    best = min(near, key=lambda c: c[1])
    return {"s": best[2], "ox": best[3], "oy": best[4],
            "grid": grid, "residual": best[0]}


def choose_scale(points):
    result = fit(points, 10)
    if result["residual"] > MAX_RESIDUAL:
        alt = fit(points, 5)
        if alt["residual"] <= MAX_RESIDUAL:
            return alt
    return result


def scale_font(font, s):
    parts = font.split(",")
    try:
        parts[1] = str(max(3, int(round(float(parts[1]) * s))))
    except (IndexError, ValueError):
        pass
    return ",".join(parts)


def transform(root, fitted, snap):
    s, ox, oy = fitted["s"], fitted["ox"], fitted["oy"]
    desc = root.find("description")

    def X(v):
        return s * v + ox

    def Y(v):
        return s * v + oy

    for part in list(desc):
        tag = part.tag
        if tag in ("rect", "arc"):
            part.set("x", num(X(float(part.get("x")))))
            part.set("y", num(Y(float(part.get("y")))))
            part.set("width", num(s * float(part.get("width"))))
            part.set("height", num(s * float(part.get("height"))))
        elif tag == "line":
            for a, f in (("x1", X), ("y1", Y), ("x2", X), ("y2", Y)):
                part.set(a, num(f(float(part.get(a)))))
        elif tag == "polygon":
            for a in list(part.attrib):
                if a[0] == "x" and a[1:].isdigit():
                    part.set(a, num(X(float(part.get(a)))))
                elif a[0] == "y" and a[1:].isdigit():
                    part.set(a, num(Y(float(part.get(a)))))
        elif tag == "circle":
            d = float(part.get("diameter"))
            cx, cy = float(part.get("x")) + d / 2, float(part.get("y")) + d / 2
            nd = max(1.0, s * d)
            if id(part) in snap:
                cx, cy = snap[id(part)]
            else:
                cx, cy = X(cx), Y(cy)
            part.set("diameter", num(nd))
            part.set("x", num(cx - nd / 2))
            part.set("y", num(cy - nd / 2))
        elif tag in ("text", "dynamic_text"):
            part.set("x", num(X(float(part.get("x")))))
            part.set("y", num(Y(float(part.get("y")))))
            if part.get("font"):
                part.set("font", scale_font(part.get("font"), s))
        elif tag == "terminal":
            desc.remove(part)


def bounds(root):
    xs, ys = [], []
    for p in root.find("description"):
        a = p.attrib
        t = p.tag
        if t in ("rect", "arc", "circle"):
            w = float(a.get("width", a.get("diameter", 0)))
            h = float(a.get("height", a.get("diameter", 0)))
            x, y = float(a["x"]), float(a["y"])
            xs += [x, x + w]
            ys += [y, y + h]
        elif t == "line":
            xs += [float(a["x1"]), float(a["x2"])]
            ys += [float(a["y1"]), float(a["y2"])]
        elif t == "polygon":
            xs += [float(v) for k, v in a.items()
                   if k[0] == "x" and k[1:].isdigit()]
            ys += [float(v) for k, v in a.items()
                   if k[0] == "y" and k[1:].isdigit()]
        elif t in ("text", "dynamic_text", "terminal"):
            xs.append(float(a["x"]))
            ys.append(float(a["y"]))
    return min(xs), min(ys), max(xs), max(ys)


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


def layout_of(points):
    xs = sorted(round(p[0]) for p in points)
    clusters = 1
    for a, b in zip(xs, xs[1:]):
        if b - a > 6:
            clusters += 1
    return "rows" if clusters > 2 else "columns"


def build(key, drawings, symbols, out_dir):
    src, name_en, name_fr, extra_names = CARDS[key]
    root = ET.parse(os.path.join(drawings, src + ".elmt.elmt")).getroot()
    symbol = ET.parse(os.path.join(symbols, key + ".elmt")).getroot()

    circles = circles_of(root)
    names = name_circles(root, circles)
    fitted = choose_scale([(c[0], c[1]) for c in circles])
    s, ox, oy, grid = fitted["s"], fitted["ox"], fitted["oy"], fitted["grid"]

    snap, centres = {}, []
    for i, (cx, cy, node) in enumerate(circles):
        gx = round((s * cx + ox) / grid) * grid
        gy = round((s * cy + oy) / grid) * grid
        snap[id(node)] = (gx, gy)
        centres.append((gx, gy, names[i]))

    originals = [(float(t.get("x")), float(t.get("y")), t.get("orientation"))
                 for t in root.find("description").iter("terminal")]
    extras = [t for t in originals
              if all(math.hypot(t[0] - c[0], t[1] - c[1]) > 30
                     for c in circles)]

    transform(root, fitted, snap)
    desc = root.find("description")

    layout = layout_of([(c[0], c[1]) for c in centres])
    mid_x = sum(c[0] for c in centres) / len(centres)
    mid_y = sum(c[1] for c in centres) / len(centres)
    for gx, gy, name in sorted(centres, key=lambda c: (c[1], c[0])):
        if layout == "rows":
            orient = "n" if gy < mid_y else "s"
        else:
            orient = "w" if gx < mid_x else "e"
        ET.SubElement(desc, "terminal", {
            "x": num(gx), "y": num(gy), "name": name, "orientation": orient,
            "type": "Generic", "uuid": uid(key, "realistic", "term", name)})
    for n, (ex, ey, orient) in enumerate(sorted(extras)):
        label = extra_names[n] if n < len(extra_names) else "X%d" % (n + 1)
        gx = round((s * ex + ox) / grid) * grid
        gy = round((s * ey + oy) / grid) * grid
        ET.SubElement(desc, "terminal", {
            "x": num(gx), "y": num(gy), "name": label,
            "orientation": orient or "n", "type": "Generic",
            "uuid": uid(key, "realistic", "term", label)})

    labels = False
    for d in desc.iter("dynamic_text"):
        d.set("uuid", uid(key, "realistic", "dyn", d.get("x"), d.get("y")))
        labels |= (d.findtext("info_name") == "label")
    if not labels:
        left, top, right, bottom = bounds(root)
        dyn = ET.SubElement(desc, "dynamic_text", {
            "x": num(left), "y": num(bottom + 4), "z": "25",
            "text_width": "-1", "Halignment": "AlignLeft",
            "Valignment": "AlignTop", "frame": "false", "rotation": "0",
            "keep_visual_rotation": "true", "text_from": "ElementInfo",
            "uuid": uid(key, "realistic", "dyn", "label"),
            "font": "Liberation Sans,9,-1,5,50,0,0,0,0,0,Regular"})
        ET.SubElement(dyn, "text")
        ET.SubElement(dyn, "info_name").text = "label"

    left, top, right, bottom = bounds(root)
    left = math.floor((left - 5) / 10) * 10
    top = math.floor((top - 5) / 10) * 10
    width = int(math.ceil((right + 5 - left) / 10) * 10)
    height = int(math.ceil((bottom + 5 - top) / 10) * 10)

    out = ET.Element("definition", {
        "version": "0.100.0", "type": "element",
        "link_type": symbol.get("link_type"),
        "width": str(width), "height": str(height),
        "hotspot_x": str(int(-left)), "hotspot_y": str(int(-top))})
    ET.SubElement(out, "uuid", {"uuid": uid(key, "realistic", "element")})
    names_el = ET.SubElement(out, "names")
    ET.SubElement(names_el, "name", {"lang": "en"}).text = \
        name_en + ", realistic view"
    ET.SubElement(names_el, "name", {"lang": "fr"}).text = \
        name_fr + ", vue réaliste"
    for tag in ("elementInformations", "kindInformations"):
        node = symbol.find(tag)
        if node is not None:
            out.append(node)
    ET.SubElement(out, "informations").text = (
        "Author: QElectroTech contributors\n"
        "Realistic view made by misc/beckhoff/realistic_from_model.py from "
        "a drawing of the user: scale %.2f, grid %d px, residual %.2f px. "
        "License: see http://qelectrotech.org/wiki/doc/elements_license"
        % (s, grid, fitted["residual"]))
    out.append(desc)
    indent(out)
    path = os.path.join(out_dir, key + "_realistic.elmt")
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(ET.tostring(out, encoding="unicode") + "\n")
    print("%-7s scale %.2f  offset (%.1f, %.1f)  grid %d px  "
          "max residual %.2f px  circles %d  extra terminals %d  size %dx%d"
          % (key, s, ox, oy, grid, fitted["residual"], len(circles),
             len(extras), width, height))


def main():
    if len(sys.argv) != 4:
        print(__doc__)
        return 2
    drawings, symbols, out_dir = sys.argv[1:]
    os.makedirs(out_dir, exist_ok=True)
    for key in CARDS:
        build(key, drawings, symbols, out_dir)
    return 0


if __name__ == "__main__":
    sys.exit(main())
