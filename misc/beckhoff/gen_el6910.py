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
"""Schematic of the EL6910 (TwinSAFE Logic) in the housing of the other
cards: same yellow body and clips, a "Logic" block, no field terminal.

The public datasheet gives "current consumption power contacts: -" and no
power contact connection, so no power contact is drawn (not verified, see
docs/beckhoff-ethercat.md).  The old drawing had a wide body, the text
"BECKHOFF" and two generic rails under it: they are replaced.

Usage: python gen_el6910.py <beckhoff elements dir>
"""
import glob
import math
import os
import sys
import xml.etree.ElementTree as ET

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gen_schematic import (Sheet, DOTTED, THIN, YELLOW_FILL, CLIP_W,  # noqa
                           CLIP_H, EL60_BODY, NOTE, uid)
from realistic_fixes import num, write  # noqa: E402


def main(base):
    hits = glob.glob(os.path.join(base, "twinsafe", "el6910", "el6910.elmt"))
    if not hits:
        return
    path = hits[0]
    root = ET.parse(path).getroot()
    old = root.find("description")
    dyn = old.find("dynamic_text")
    s = Sheet()
    x0, y0, x1, y1 = EL60_BODY
    s.rect(x0, y0, x1 - x0, y1 - y0, YELLOW_FILL)
    s.rect(x0, y0, x1 - x0, y1 - y0, DOTTED)
    cx = (x0 + x1) / 2 - 2.0
    for top in (True, False):
        y = y0 - CLIP_H if top else y1
        s.rect(cx - CLIP_W / 2, y, CLIP_W, CLIP_H, THIN)
        inner = y + (2.0 if top else CLIP_H - 2.0)
        s.line(cx - CLIP_W / 2 + 3, inner, cx + CLIP_W / 2 - 3, inner)
    mx = (x0 + x1) / 2
    s.rect(mx - 11, -22, 22, 44, THIN)
    s.text_centered(mx, 0, "Logic", 5)
    s.text(mx - 15, y1 - 14, "EL6910", 9, True)
    desc = ET.Element("description")
    for e in s.parts:
        desc.append(e)
    if dyn is not None:
        dyn.set("x", num(x0 - 6))
        dyn.set("y", num(y1 + CLIP_H + 6))
        desc.append(dyn)
    xs = [x for x, _ in s.bits]
    ys = [y for _, y in s.bits]
    if dyn is not None:
        ys.append(float(dyn.get("y")) + 12)
        xs.append(float(dyn.get("x")) + 40)
    ax, ay = min(xs) - 5, min(ys) - 5
    root.remove(old)
    root.append(desc)
    root.set("width", str(int(math.ceil(max(xs) + 5 - ax))))
    root.set("height", str(int(math.ceil(max(ys) + 5 - ay))))
    root.set("hotspot_x", str(int(math.ceil(-ax))))
    root.set("hotspot_y", str(int(math.ceil(-ay))))
    info = root.find("informations")
    for ei in root.iter("elementInformation"):
        if ei.get("name") == "description":
            ei.text = ("TwinSAFE logic terminal, no field terminals and no "
                       "terminal drawn. Power contacts not drawn: the "
                       "datasheet gives no power contact connection "
                       "(current consumption power contacts: -).")
    info.text = (
        "Author: QElectroTech contributors\n"
        "Generic layout, not copied from a manufacturer drawing. License: "
        "see http://qelectrotech.org/wiki/doc/elements_license\n"
        + NOTE.replace("gen_schematic", "gen_el6910").replace(
            "the realistic view: the stages are generic, check them against"
            " the datasheet.", "the datasheet.") + "\n"
        "No power contact drawn: the datasheet gives \"current consumption "
        "power contacts: -\" (not verified), source "
        "https://www.beckhoff.com/en-en/products/i-o/ethercat-terminals/"
        "el-ed6xxx-communication/el6910.html")
    write(root, path)
    print("el6910")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
