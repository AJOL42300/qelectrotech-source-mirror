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

The drawings are elements that you drew yourself (Beckhoff_<REF>.elmt, in
your own element collection). They are read-only inputs: they are never
modified and never copied into the repository. This script reads them and
writes new elements, <key>_realistic.elmt, next to the symbol of the card:

  * the drawing is kept as drawn (geometry, colours, yellow of the safety
    cards) and scaled by ONE uniform factor, the same for every card (the
    scale of the EK1914 drawing, 1.00), with an origin offset;
  * the reference of the card (EL1004, EL2911...) is always inside the card,
    at the bottom; the bottom terminals of the EK1914 are yellow (TwinSAFE);
  * the EL1918 (16 terminals) is built from the terminal block of the
    EK1914 drawing, with a yellow body, because no drawing of it is used;
  * the circles of the drawing (screw circles, drawn as arcs or circles) are
    detected: a terminal is created exactly at the centre of each of them,
    and the centres are on the 10 px grid of the diagram (5 px when 10 px
    is not possible, see the report). The parts of the drawing that belong
    to a circle move with it by the residual of the fit;
  * the terminals are named after the physical pin number of the card
    (left column 1 to 4, right column 5 to 8, the second group 9 to 16),
    never after the channel: the channel is in the I/O table;
  * the elementInformations and the I/O table of the symbol of the same
    card are copied, so that every appearance carries the same data.

Usage:
  realistic_from_model.py <drawings dir> <elements dir>

<drawings dir> holds the Beckhoff_<REF>.elmt files, <elements dir> is
extra-elements/10_electric/20_manufacturers_articles/beckhoff (made by gen_symbols.py and
gen_safety.py). The report (scale, offset, residual) goes to the standard
output.
"""
import math
import os
import sys
import uuid
import xml.etree.ElementTree as ET

NS = uuid.UUID("6f1c2a5e-8b0f-4d57-9a7e-5b1d0c8e2f10")

# key -> (drawing, folder of the symbol)
CARDS = {
    "ek1914": ("EK1914", "."), "el1004": ("EL1004", "."),
    "el1008": ("EL1008", "."), "el1904": ("EL1904", "twinsafe"),
    "el2004": ("EL2004", "."),
    "el2008": ("EL2008", "."), "el2522": ("EL2522", "."),
    "el2574": ("EL2574", "."), "el2904": ("EL2904", "twinsafe"),
    "el2911": ("EL2911", "."), "el5152": ("EL5152", "."),
    "el9100": ("EL9100", "."),
    "el1918": ("EK1914", "twinsafe"),
}

SCALE = (0.95, 1.05)   # every realistic view is at the scale of the EK1914
YELLOW_FROM = 50.0     # EK1914: circles below this y (drawing units) are yellow
# reference text moved inside the card: key -> (x, y) in drawing units
REFERENCE_AT = {"ek1914": (-40.0, 84.0), "el2911": (-12.95, 134.24)}
# terminal block of the EK1914, kept for the EL1918 (x in drawing units)
BLOCK_X = 12.0

MIN_ARC = 10           # arcs smaller than this are not screw circles
MIN_CIRCLE = 7         # same for <circle> parts
MAX_RESIDUAL = 1.5     # above, the 5 px grid is tried
# EtherCAT sockets of the EK1914 drawing, measured on it (drawing units)
EK1914_SOCKETS = [("EtherCAT IN", -22.0, -50.0), ("EtherCAT OUT", -22.0, 8.0)]


def uid(*parts):
    return "{%s}" % uuid.uuid5(NS, "/".join(parts))


def num(v):
    v = round(v, 2)
    return str(int(v)) if v == int(v) else ("%.2f" % v).rstrip("0")


def f(part, name):
    return float(part.get(name))


def box(p):
    """Bounding box (x0, y0, x1, y1) of a drawing part, or None."""
    t, a = p.tag, p.attrib
    if t in ("rect", "arc", "ellipse"):
        x, y, w, h = f(p, "x"), f(p, "y"), f(p, "width"), f(p, "height")
        return min(x, x + w), min(y, y + h), max(x, x + w), max(y, y + h)
    if t == "circle":
        d = f(p, "diameter")
        return f(p, "x"), f(p, "y"), f(p, "x") + d, f(p, "y") + d
    if t == "line":
        xs, ys = [f(p, "x1"), f(p, "x2")], [f(p, "y1"), f(p, "y2")]
        return min(xs), min(ys), max(xs), max(ys)
    if t == "polygon":
        xs = [float(v) for k, v in a.items() if k[0] == "x" and k[1:].isdigit()]
        ys = [float(v) for k, v in a.items() if k[0] == "y" and k[1:].isdigit()]
        return min(xs), min(ys), max(xs), max(ys)
    return None


def circles_of(desc):
    """Centres of the screw circles: [(cx, cy, diameter)], sorted."""
    found = {}
    for p in desc:
        d = None
        if p.tag == "arc" and abs(f(p, "width")) >= MIN_ARC:
            d = abs(f(p, "width"))
        elif p.tag == "circle" and f(p, "diameter") >= MIN_CIRCLE:
            d = f(p, "diameter")
        if d is None:
            continue
        x0, y0, x1, y1 = box(p)
        key = (round((x0 + x1) / 2, 1), round((y0 + y1) / 2, 1))
        found[key] = max(found.get(key, 0), d)
    return [(k[0], k[1], d) for k, d in sorted(found.items())]


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
    for k in range(int(SCALE[0] * 100), int(SCALE[1] * 100) + 1):
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
        if alt["residual"] < result["residual"]:
            return alt
    return result


def scale_font(font, s):
    parts = font.split(",")
    try:
        parts[1] = str(max(3, int(round(float(parts[1]) * s))))
    except (IndexError, ValueError):
        pass
    return ",".join(parts)


def shift(part, dx, dy):
    t = part.tag
    if t in ("rect", "arc", "ellipse", "circle"):
        part.set("x", num(f(part, "x") + dx))
        part.set("y", num(f(part, "y") + dy))
    elif t == "line":
        for a, d in (("x1", dx), ("y1", dy), ("x2", dx), ("y2", dy)):
            part.set(a, num(f(part, a) + d))
    elif t == "polygon":
        for a in list(part.attrib):
            if a[0] in "xy" and a[1:].isdigit():
                part.set(a, num(f(part, a) + (dx if a[0] == "x" else dy)))


def transform(desc, s, ox, oy):
    def X(v):
        return s * v + ox

    def Y(v):
        return s * v + oy

    for part in list(desc):
        t = part.tag
        if t in ("rect", "arc", "ellipse"):
            part.set("x", num(X(f(part, "x"))))
            part.set("y", num(Y(f(part, "y"))))
            part.set("width", num(s * f(part, "width")))
            part.set("height", num(s * f(part, "height")))
        elif t == "circle":
            part.set("x", num(X(f(part, "x"))))
            part.set("y", num(Y(f(part, "y"))))
            part.set("diameter", num(s * f(part, "diameter")))
        elif t == "line":
            for a, fn in (("x1", X), ("y1", Y), ("x2", X), ("y2", Y)):
                part.set(a, num(fn(f(part, a))))
        elif t == "polygon":
            for a in list(part.attrib):
                if a[0] == "x" and a[1:].isdigit():
                    part.set(a, num(X(f(part, a))))
                elif a[0] == "y" and a[1:].isdigit():
                    part.set(a, num(Y(f(part, a))))
        elif t in ("text", "dynamic_text"):
            part.set("x", num(X(f(part, "x"))))
            part.set("y", num(Y(f(part, "y"))))
            if part.get("font"):
                part.set("font", scale_font(part.get("font"), s))
        elif t == "terminal":
            desc.remove(part)


def snap_circles(desc, centres, grid):
    """Move every circle (and the small parts inside it) onto the grid."""
    maxd = max(c[2] for c in centres)
    moves = []
    for cx, cy, d in centres:
        moves.append((cx, cy, round(cx / grid) * grid - cx,
                      round(cy / grid) * grid - cy))
    for part in list(desc):
        b = box(part)
        if b is None or max(b[2] - b[0], b[3] - b[1]) > 1.5 * maxd:
            continue
        mx, my = (b[0] + b[2]) / 2, (b[1] + b[3]) / 2
        near = min(moves, key=lambda m: math.hypot(m[0] - mx, m[1] - my))
        if math.hypot(near[0] - mx, near[1] - my) <= maxd * 0.6:
            shift(part, near[2], near[3])


def bounds(desc):
    xs, ys = [], []
    for p in desc:
        b = box(p)
        if b:
            xs += [b[0], b[2]]
            ys += [b[1], b[3]]
        elif p.tag in ("text", "dynamic_text", "terminal"):
            xs.append(f(p, "x"))
            ys.append(f(p, "y"))
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


STYLE = "line-style:normal;line-weight:%s;filling:%s;color:black"


def part_style(name, weight="thin", filling="none"):
    return {"antialias": "false", "style": STYLE % (weight, filling)}


def text_part(desc, x, y, text, size=7, bold=False):
    font = "Roboto Condensed,%d,-1,5,%d,0,0,0,0,0,%s" % (
        size, 75 if bold else 50, "Bold" if bold else "Regular")
    ET.SubElement(desc, "text", {"x": num(x), "y": num(y), "font": font,
                                 "rotation": "0", "color": "#000000",
                                 "text": text})


def mark_yellow_bottom(desc):
    """EK1914: the outer circles of the bottom terminals are yellow."""
    for p in desc.findall("circle"):
        if abs(f(p, "diameter") - 8.5) < 0.1 and \
                f(p, "y") + 4.25 >= YELLOW_FROM:
            p.set("style", STYLE % ("thin", "yellow"))


def move_reference(desc, key, ref):
    """The reference of the card is drawn inside the card, at the bottom."""
    for p in desc.findall("text"):
        if p.get("text") == ref:
            p.set("x", num(REFERENCE_AT[key][0]))
            p.set("y", num(REFERENCE_AT[key][1]))


def prepare_el1918(desc, signals):
    """Keep only the screw circles of the EK1914 terminal block, add a
    yellow body, the signal of every terminal and the reference."""
    for p in list(desc):
        if p.tag == "circle" and f(p, "x") >= BLOCK_X and \
                f(p, "diameter") >= MIN_CIRCLE:
            continue
        desc.remove(p)
    # even pitch of 15 between the rows (the source rows are not regular)
    rows = sorted({round(c[1]) for c in circles_of(desc)})
    for p in desc.findall("circle"):
        cy = f(p, "y") + f(p, "diameter") / 2
        k = min(range(len(rows)), key=lambda i: abs(rows[i] - cy))
        p.set("y", num(f(p, "y") + rows[0] + 15 * k - rows[k]))
    circles = circles_of(desc)
    xs = sorted({round(c[0]) for c in circles})
    ys = sorted({round(c[1]) for c in circles})
    left, right = xs[0] - 33, xs[-1] + 33
    top, bottom = ys[0] - 14, ys[-1] + 26
    body = ET.Element("rect", dict(
        part_style("body", "normal", "yellow"), x=num(left), y=num(top),
        width=num(right - left), height=num(bottom - top), rx="0", ry="0"))
    desc.insert(0, body)
    for cx, cy, _ in circles:
        pin = 2 * ys.index(round(cy)) + xs.index(round(cx)) + 1
        name = signals.get(str(pin), "")
        if xs.index(round(cx)) == 0:
            text_part(desc, left + 4, cy - 5, name, 7)
        else:
            text_part(desc, cx + 12, cy - 5, name, 7)
    text_part(desc, left + 6, bottom - 14, "EL1918", 8, True)


def pin_names(key, grid_pts):
    """Physical pin number of each circle (same order as grid_pts).

    EL cards: two columns of four rows, left 1 to 4, right 5 to 8; the
    second group of the EL2911 is 9 to 16. EK1914: rows of two, 1 to 16."""
    xs = sorted({round(p[0]) for p in grid_pts})
    ys = sorted({round(p[1]) for p in grid_pts})
    names = []
    for x, y in grid_pts:
        c, r = xs.index(round(x)), ys.index(round(y))
        if key in ("ek1914", "el1918"):
            names.append(str(2 * r + c + 1))
        else:
            names.append(str(8 * (c // 2) + 4 * (c % 2) + r + 1))
    return names


def el1918_signals(symbol):
    """terminal number -> signal: clock output Sn+, safe input SIn."""
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import gen_safety
    out = {}
    for n, (clock, safe) in gen_safety.EL1918_TERMINALS.items():
        out[str(clock)] = "S%d+" % n
        out[str(safe)] = "SI%d" % n
    return out


def build(key, drawings, elements):
    ref, folder = CARDS[key]
    out_dir = os.path.join(elements, folder)
    root = ET.parse(os.path.join(drawings, "Beckhoff_%s.elmt" % ref)).getroot()
    symbol = ET.parse(os.path.join(out_dir, key + ".elmt")).getroot()
    potentials = {t.get("name"): t.get("potential")
                  for t in symbol.find("description").iter("terminal")
                  if t.get("potential")}
    desc = root.find("description")
    if key == "ek1914":
        mark_yellow_bottom(desc)
    if key in REFERENCE_AT:
        move_reference(desc, key, ref)
    if key == "el1918":
        prepare_el1918(desc, el1918_signals(symbol))

    centres = circles_of(desc)
    points = [(c[0], c[1]) for c in centres]
    sockets = EK1914_SOCKETS if key == "ek1914" else []
    fitted = choose_scale(points + [(x, y) for _, x, y in sockets])
    s, ox, oy, grid = fitted["s"], fitted["ox"], fitted["oy"], fitted["grid"]

    transform(desc, s, ox, oy)
    scaled = [(s * c[0] + ox, s * c[1] + oy, s * c[2]) for c in centres]
    snap_circles(desc, scaled, grid)
    snapped = [(round(c[0] / grid) * grid, round(c[1] / grid) * grid)
               for c in scaled]
    names = pin_names(key, snapped)

    mid_x = sum(p[0] for p in snapped) / len(snapped)
    for (gx, gy), name in sorted(zip(snapped, names),
                                 key=lambda z: int(z[1])):
        attrs = {"x": num(gx), "y": num(gy), "name": name,
                 "orientation": "w" if gx < mid_x else "e",
                 "type": "Generic",
                 "uuid": uid(key, "realistic", "term", name)}
        if name in potentials:
            attrs["potential"] = potentials[name]
        ET.SubElement(desc, "terminal", attrs)
    for name, sx, sy in sockets:
        ET.SubElement(desc, "terminal", {
            "x": num(round((s * sx + ox) / grid) * grid),
            "y": num(round((s * sy + oy) / grid) * grid),
            "name": name, "orientation": "w", "type": "Generic",
            "uuid": uid(key, "realistic", "term", name)})

    has_label = False
    for d in desc.iter("dynamic_text"):
        d.set("uuid", uid(key, "realistic", "dyn", d.get("x"), d.get("y")))
        has_label |= (d.findtext("info_name") == "label")
    if not has_label:
        left, top, right, bottom = bounds(desc)
        dyn = ET.SubElement(desc, "dynamic_text", {
            "x": num(left), "y": num(bottom + 4), "z": "25",
            "text_width": "-1", "Halignment": "AlignLeft",
            "Valignment": "AlignTop", "frame": "false", "rotation": "0",
            "keep_visual_rotation": "true", "text_from": "ElementInfo",
            "uuid": uid(key, "realistic", "dyn", "label"),
            "font": "Liberation Sans,9,-1,5,50,0,0,0,0,0,Regular"})
        ET.SubElement(dyn, "text")
        ET.SubElement(dyn, "info_name").text = "label"

    left, top, right, bottom = bounds(desc)
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
    for sym_name in symbol.find("names"):
        suffix = (", realistic view" if sym_name.get("lang") == "en"
                  else ", vue réaliste")
        ET.SubElement(names_el, "name", {"lang": sym_name.get("lang")}).text = \
            sym_name.text + suffix
    for tag in ("elementInformations", "kindInformations"):
        node = symbol.find(tag)
        if node is not None:
            out.append(node)
    ET.SubElement(out, "informations").text = (
        "Author: QElectroTech contributors\n"
        "Realistic view made by misc/beckhoff/realistic_from_model.py from "
        "a drawing supplied by the user: scale %.2f, grid %d px, residual "
        "%.2f px. License: see "
        "http://qelectrotech.org/wiki/doc/elements_license"
        % (s, grid, fitted["residual"]))
    # a channel whose pin does not exist on the drawing keeps no pin
    present = {t.get("name") for t in desc.iter("terminal")}
    for io in out.iter("plcIO"):
        pins = io.findall("terminal")
        if any(p.text not in present for p in pins):
            for p in pins:
                io.remove(p)
            io.attrib.pop("terminalCount", None)
    out.append(desc)
    indent(out)
    path = os.path.join(out_dir, key + "_realistic.elmt")
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(ET.tostring(out, encoding="unicode") + "\n")
    worst = max(max(abs(p[0] - c[0]), abs(p[1] - c[1]))
                for p, c in zip(snapped, scaled))
    print("%-7s scale %.2f  offset (%.1f, %.1f)  grid %d px  fit residual "
          "%.2f px  circles %d  sockets %d  size %dx%d  hotspot (%d, %d)"
          % (key, s, ox, oy, grid, max(fitted["residual"], worst),
             len(centres), len(sockets), width, height, -left, -top))


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    drawings, elements = sys.argv[1:]
    for key in CARDS:
        build(key, drawings, elements)
    return 0


if __name__ == "__main__":
    sys.exit(main())
