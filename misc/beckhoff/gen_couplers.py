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
"""Schematic view of the couplers and system terminals.

EK1100, EL6910, EL9011 and the generic EtherCAT box have no
realistic view: their symbol was a plain rectangle.  This script draws them
as the other schematic views: dotted outline with the clips of the rail, and
inside the outline the EtherCAT bus (the E-bus block that links the bus
connectors) or, for the EL6910, the logic block.  The terminals, names and
data are kept.

Usage: python gen_couplers.py <extra-elements>/10_electric/20_manufacturers_articles/beckhoff
"""
import glob
import math
import os
import sys
import xml.etree.ElementTree as ET

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gen_schematic import (Sheet, DOTTED, THIN, CLIP_W, CLIP_H,  # noqa: E402
                           NOTE)
from realistic_fixes import num, write  # noqa: E402

CARDS = {
    "ek1100": "E-bus",
    "ethercat_box_generic": "E-bus", "el9011": "",
}


def outline_of(desc):
    rect = desc.find("rect")
    return (float(rect.get("x")), float(rect.get("y")),
            float(rect.get("width")), float(rect.get("height")), rect)


def patch(path, label):
    root = ET.parse(path).getroot()
    desc = root.find("description")
    x, y, w, h, rect = outline_of(desc)
    if "dotted" in rect.get("style"):
        return False
    rect.set("style", rect.get("style").replace("line-style:normal",
                                                "line-style:dotted"))
    gap = 6
    if "ethercat_box_generic" in path:
        # room under the label of the card
        gap = 16
        h += 14
        rect.set("height", num(h))
        for d in desc.findall("dynamic_text")[1:]:
            d.set("y", num(float(d.get("y")) + 14))
        root.set("height", str(int(root.get("height")) + 14))
    s = Sheet()
    cx = x + w / 2 - 2.0
    for top in (True, False):
        yy = y - CLIP_H if top else y + h
        s.rect(cx - CLIP_W / 2, yy, CLIP_W, CLIP_H, THIN)
        inner = yy + (2.0 if top else CLIP_H - 2.0)
        s.line(cx - CLIP_W / 2 + 3, inner, cx + CLIP_W / 2 - 3, inner)
    pins = [e for e in desc.findall("terminal")]
    left = sorted((float(p.get("y")) for p in pins
                   if float(p.get("x")) < x + 1))
    right = sorted((float(p.get("y")) for p in pins
                    if float(p.get("x")) > x + w - 1))
    rows = max(len(left), len(right), 1)
    bx0, bx1 = cx - 24, cx + 24
    if pins:
        by0 = max(left + right) + gap
        bh = 8 * rows + 12
        s.rect(bx0, by0, bx1 - bx0, bh)
        for ys, side in ((left, -1), (right, 1)):
            for i, py in enumerate(ys):
                ex = x + w - 20 + 5 * i if side > 0 else x + 20 - 5 * i
                ey = by0 + 6 + 8 * i
                bx = bx1 if side > 0 else bx0
                s.poly([(ex, py), (ex, ey), (bx, ey)], closed=False)
        s.text(cx - 9, by0 + 8 * rows, label, 5)
    elif label:
        by0 = y + h / 2 - 12
        s.rect(bx0, by0, bx1 - bx0, 24)
        s.text(cx - 9, by0 + 8, label, 5)
    else:
        s.line(cx - 10, y + h / 2, cx + 10, y + h / 2)
    for e in s.parts:
        desc.insert(1, e)
    dyn = desc.find("dynamic_text")
    if dyn is not None:
        dyn.set("y", num(float(dyn.get("y")) + CLIP_H))
    root.set("height", str(int(root.get("height")) + 2 * int(CLIP_H)))
    root.set("hotspot_y", str(int(root.get("hotspot_y")) + int(CLIP_H)))
    info = root.find("informations")
    if info is not None and "gen_couplers" not in (info.text or ""):
        info.text = (info.text or "").rstrip("\n") + "\n" + \
            NOTE.replace("gen_schematic", "gen_couplers")
    write(root, path)
    return True


def add_power(path):
    """+24 V and 0 V of the power contacts under the drawing: the housing
    is 32 px taller, the rails run from edge to edge."""
    root = ET.parse(path).getroot()
    desc = root.find("description")
    x, y, w, h, rect = outline_of(desc)
    more = 32
    done = any(t.get("text") == "+24 V" for t in desc.findall("text"))
    if done:
        # drawn by an earlier version: redraw the rails in colour
        band = y + h - more + 5, y + h - more + 27
        old = []
        for e in desc:
            if e is rect or e.tag not in ("line", "rect", "circle", "text"):
                continue
            ys = [float(e.get(k)) for k in ("y", "y1", "y2") if e.get(k)]
            if ys and all(band[0] <= v <= band[1] for v in ys):
                old.append(e)
        if not old or any(e.get("style", "").count("color:red") for e in old):
            return False
        for e in old:
            desc.remove(e)
        bottom = y + h - more
    else:
        bottom = y + h
        for e in desc:
            if e.tag == "rect" and float(e.get("y")) >= bottom - 0.5:
                e.set("y", num(float(e.get("y")) + more))
            elif e.tag == "line" and float(e.get("y1")) >= bottom - 0.5:
                e.set("y1", num(float(e.get("y1")) + more))
                e.set("y2", num(float(e.get("y2")) + more))
            elif e.tag == "dynamic_text" and float(e.get("y")) >= bottom - 0.5:
                e.set("y", num(float(e.get("y")) + more))
        rect.set("height", num(h + more))
        root.set("height", str(int(root.get("height")) + more))
    s = Sheet()
    s.power_rails(x, x + w, bottom + 10, bottom + 22)
    for e in s.parts:
        desc.insert(1, e)
    info = root.find("informations")
    if info is not None and "power contacts" not in (info.text or ""):
        info.text = (info.text or "").rstrip("\n") + \
            "\nThe +24 V and 0 V rails stand for the power contacts " \
            "(generic)."
    write(root, path)
    return True


def main(base):
    for stem, label in CARDS.items():
        hits = glob.glob(os.path.join(base, "*", "*", stem + ".elmt"))
        if hits and patch(hits[0], label):
            print("coupler schematic", stem)
        if hits and add_power(hits[0]):
            print("power contacts", stem)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
