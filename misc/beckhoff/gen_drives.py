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
"""Write the Beckhoff AF1000 and AX5000 servo drive elements.

Usage: python misc/beckhoff/gen_drives.py <extra-elements> [--report]

Every reference gets three appearances: <ref>.elmt (symbol),
<ref>_realistic.elmt (vector redraw of the front of the drive) and
<ref>_table.elmt (I/O table). The drawings are redrawn by hand from the
user's project pictures, which are NOT in the repository: only plain lines,
rectangles, circles and the names of the connectors. Positions were
measured on the pictures, then snapped to the 10 px grid of QElectroTech
(--report prints the residual of every connector).

The number of axes is the second digit of the reference (AX5103 and
AF1103: 1 axis; AX5203 and AF1215: 2 axes).
"""
import os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_symbols as gs
from gen_symbols import Elmt, NOTE

AF_SCALE = 0.95     # picture px -> element px (X03 row pitch 10.5 -> 10)
AX_SCALE = 0.313    # display px -> element px of the AX5000 pictures

# reference, rating as written in the user's project nomenclature
AF_REFS = [("AF1103", "0.37 kW, 1.6 A"), ("AF1107", "0.75 kW, 2 A"),
           ("AF1115", "1.5 kW, 3.4 A"), ("AF1130", "3 kW, 6.7 A"),
           ("AF1155", "5.5 kW, 18 A (as written in the project, to be checked)"),
           ("AF1215", "1.5 kW, 3.4 A"), ("AF1222", "2 kW, 5.2 A")]
AX_REFS = [("AX5101", "1.5 A"), ("AX5103", "3 A"), ("AX5106", "6 A"),
           ("AX5112", "12 A"), ("AX5118", "18 A"), ("AX5125", "25 A"),
           ("AX5201", "2 x 1.5 A"), ("AX5203", "2 x 3 A"),
           ("AX5206", "2 x 6 A"), ("AX5212", "2 x 12 A"),
           ("AX5218", "2 x 18 A"), ("AX5225", "2 x 25 A")]


def axes_of(ref):
    """The second digit of the numeric part gives the number of axes."""
    return int(ref[2:][1])


class Pin:
    def __init__(self, name, label, x, y, orient, target=None):
        self.name, self.label, self.x, self.y = name, label, x, y
        self.orient, self.target = orient, target


class Block:
    """A connector of the front of the drive."""

    def __init__(self, title, sym_side, shape, pins, io=None):
        self.title, self.sym_side, self.shape = title, sym_side, shape
        self.pins, self.io = pins, io


def af_blocks(axes):
    s = AF_SCALE
    blocks = [Block("X04 X05", "n", "rj45", [
        Pin("EtherCAT IN", "X04 IN", 40, 100, "n", (45.5 * s, 109 * s)),
        Pin("EtherCAT OUT", "X05 OUT", 80, 100, "n", (88.5 * s, 109 * s))])]
    if axes == 2:
        left = ["IN1", "IN3", "GND1", "IN5", "TH1", "TH2", "OUT", "FGND1"]
        right = ["IN2", "IN4", "FGND2", "IN6", "24V_A", "24V_B", "24V_C", "FGND3"]
        first, pitch = 141 + 5.45, 10.9
    else:
        left = ["IN1", "GND1", "IN5", "TH1", "OUT", "FGND1"]
        right = ["IN2", "FGND2", "IN6", "24V_A", "24V_B", "FGND3"]
        first, pitch = 139 + 5.2, 10.3
    pins = []
    for column, names, x, tx, orient in ((0, left, 50, 51, "w"),
                                         (1, right, 80, 84.5, "e")):
        for i, n in enumerate(names):
            pins.append(Pin(n, n, x, 140 + 10 * i, orient,
                            (tx * s, (first + pitch * i) * s)))
    io = {}
    for n in left + right:
        if n.startswith("IN"):
            io[n] = "entree_digitale"
        elif n == "OUT":
            io[n] = "sortie_digitale"
        elif n.startswith("TH"):
            io[n] = "entree_analogique"
    blocks.append(Block("X03", "w", "strip", pins, io))
    for axis, x, orient, tx, title in ((1, 40, "w", 41, "X13"),
                                       (2, 100, "e", 100, "X23")):
        if axis > axes:
            continue
        blocks.append(Block(title, "e", "strip", [
            Pin("M%d-%s" % (axis, l), l, x, 340 + 20 * i, orient,
                (tx * s, (360 + 22 * i) * s))
            for i, l in enumerate(("U", "V", "W", "PE"))]))
    blocks.append(Block("X01", "w", "strip", [
        Pin(l, l, 50, 470 + 20 * i, "w", (55 * s, (498 + 22 * i) * s))
        for i, l in enumerate(("L1", "L2", "L3", "PE", "RB+", "RB-"))]))
    return blocks, (170, 630)


def ax_blocks(axes):
    s = AX_SCALE
    blocks = [Block("X04 X05", "n", "rj45", [
        Pin("EtherCAT IN", "X04 IN", 60, 250, "n", (210 * s, 810 * s)),
        Pin("EtherCAT OUT", "X05 OUT", 100, 250, "n", (320 * s, 810 * s))])]
    dsubs = [("X11", 30, 103), ("X12", 60, 200), ("X21", 100, 327),
             ("X22", 130, 425)][:2 * axes]
    blocks.append(Block("X11 X12 X21 X22", "e", "dsub", [
        Pin(n, n, x, 110, "n", (tx * s, 345 * s)) for n, x, tx in dsubs]))
    x06 = ["24", "0", "1", "2", "3", "4", "5", "6", "7", "0V"]
    io = {"X06-%s" % l: "entree_universelle" for l in x06[1:9]}
    blocks.append(Block("X06", "w", "row", [
        Pin("X06-" + l, l, 40 + 10 * i, 170, "s",
            ((168 + 21.8 * i) * s, 548 * s)) for i, l in enumerate(x06)], io))
    blocks.append(Block("X02", "w", "strip", [
        Pin("DC+", "DC+", 30, 360, "w", (115 * s, 1170 * s)),
        Pin("DC-", "DC-", 30, 380, "w", (115 * s, 1220 * s))]))
    blocks.append(Block("X01", "w", "strip", [
        Pin(l, l, 30, 480 + 20 * i, "w", (100 * s, y * s))
        for i, (l, y) in enumerate((("L1", 1537), ("L2", 1586),
                                    ("L3N", 1631), ("PE", 1672)))]))
    blocks.append(Block("Up Us GND", "w", "strip", [
        Pin(l, l, 130, 320 + 20 * i, "e", (400 * s, y * s))
        for i, (l, y) in enumerate((("Up", 1028), ("Us", 1070),
                                    ("GND", 1113)))]))
    # the motor connectors are not visible on the pictures: generic block
    # below the body, 20 px between the contacts
    for axis in range(1, axes + 1):
        blocks.append(Block("Motor %d" % axis, "e", "motor", [
            Pin("M%d-%s" % (axis, l), l, 20 + 80 * (axis - 1) + 20 * i, 640,
                "s")
            for i, l in enumerate(("U", "V", "W", "PE"))]))
    return blocks, (170, 610)


def infos_of(ref, axes, family, rating, extra):
    word = "axis" if axes == 1 else "axes"
    if family == "AF":
        designation = "%s servo drive, %d %s" % (ref, axes, word)
    else:
        designation = "%s digital compact servo drive, %d %s" % (ref, axes, word)
    return [("manufacturer", "Beckhoff"), ("manufacturer_reference", ref),
            ("designation", designation),
            ("description", "%s %d-%s drive (the second digit of the reference "
             "is the number of axes). Rating from the project nomenclature: %s. "
             "Redrawn in vector from a picture of the project, terminals "
             "snapped to the 10 px grid. %s" % (family, axes, word, rating,
                                                extra))]


AF_NOTE = ("Mains terminals drawn as 3-phase L1 L2 L3 PE RB+ RB-; the "
           "1-phase version (L N PE RB+ RB-) is not drawn. 24V_A..C are the "
           "'25 Vdc' of a blurry picture, read as 24 V. Check against the "
           "datasheet.")
AX_NOTE = ("The motor and brake connectors are not on the pictures: the Motor "
           "terminals are generic, under the body. X06 function names are "
           "generic (0..7).")


def add_io(e, blocks):
    for b in blocks:
        for p in b.pins:
            kind = (b.io or {}).get(p.name)
            if kind:
                e.io(kind, p.name, pins=[p.name])


def symbol(ref, axes, family, rating, blocks):
    note = AF_NOTE if family == "AF" else AX_NOTE
    e = Elmt(ref.lower(), "%s servo drive, %d axis" % (ref, axes),
             "%s variateur, %d axe(s)" % (ref, axes),
             infos_of(ref, axes, family, rating, note), NOTE)
    add_io(e, blocks)
    west, east, north = [], [], []
    for b in blocks:
        {"w": west, "e": east, "n": north}[b.sym_side].append(b)
    h = 60
    for side in (west, east):
        h = max(h, 60 + sum(20 + 20 * len(b.pins) for b in side))
    h += 20
    e.rect(-80, 0, 160, h)
    e.text(-74, 22, ref, 12, True)
    e.text(-74, 38, "BECKHOFF", 5)
    for b in north:
        for i, p in enumerate(b.pins):
            x = -30 + 60 * i
            e.terminal(x, 0, p.name, "n")
            e.line(x, 0, x, 10)
            e.text(x - 12, 12, p.label, 5)
    for side, sign in ((west, -1), (east, 1)):
        y = 60
        for b in side:
            e.text(-74 if sign < 0 else 40, y - 8, b.title, 5, True)
            for p in b.pins:
                y += 20
                e.terminal(80 * sign, y, p.name, "w" if sign < 0 else "e")
                e.line(80 * sign, y, 70 * sign, y)
                e.text(-66 if sign < 0 else 66 - 4.2 * len(p.label), y - 6,
                       p.label, 6)
            y += 20
    e.dyn(-74, h + 4, "label")
    e.dyn(-74, h + 16, "designation", 6)
    return e


def realistic(ref, axes, family, rating, blocks, size):
    note = AF_NOTE if family == "AF" else AX_NOTE
    e = Elmt(ref.lower() + "_realistic",
             "%s servo drive, %d axis - realistic" % (ref, axes),
             "%s variateur, %d axe(s) - réaliste" % (ref, axes),
             infos_of(ref, axes, family, rating, note), NOTE)
    add_io(e, blocks)
    w, h = size
    e.rect(0, 0, w, h)
    e.text(8, 8, "BECKHOFF", 5, True)
    e.text(8, 20, ref, 10, True)
    e.dyn(8, 36, "label", 10, True)
    for b in blocks:
        xs = [p.x for p in b.pins]
        ys = [p.y for p in b.pins]
        if b.shape == "rj45":
            for p in b.pins:
                e.rect(p.x - 14, p.y - 17, 28, 34)
                e.rect(p.x - 8, p.y - 9, 16, 14, "thin")
                e.text(p.x - 12, p.y + 8, p.label, 4)
        elif b.shape == "dsub":
            for p in b.pins:
                e.rect(p.x - 10, p.y - 30, 20, 60)
                e.text(p.x - 8, p.y - 40, p.label, 5)
        elif b.shape == "motor":
            e.rect(min(xs) - 12, 620, max(xs) - min(xs) + 24, 40)
            e.text(min(xs) - 12, 662, b.title, 5)
        else:
            if b.shape == "row":
                rx, ry = min(xs) - 10, min(ys) - 16
                rw, rh = max(xs) - min(xs) + 20, 26
                e.text(rx, ry - 8, b.title, 5)
            else:
                rx, ry = min(xs) - 12, min(ys) - 8
                rw = max(xs) - min(xs) + (36 if b.title != "X03" else 36 + 4)
                rh = max(ys) - min(ys) + 16
                e.text(rx, ry - 8, b.title, 5)
            e.rect(rx, ry, rw, rh)
        for p in b.pins:
            if b.shape in ("strip", "row"):
                e.circle(p.x, p.y, 6)
                if b.shape == "row":
                    e.text(p.x - 3, p.y - 14, p.label, 4)
                else:
                    e.text(p.x + 6, p.y - 5, p.label, 5)
            e.terminal(p.x, p.y, p.name, p.orient)
    e.dyn(8, h + 6, "designation", 6)
    return e


def report(ref, blocks):
    worst = 0
    for b in blocks:
        for p in b.pins:
            if p.target:
                r = max(abs(p.x - p.target[0]), abs(p.y - p.target[1]))
                worst = max(worst, r)
    return worst


def write_family(root, folder, title_en, title_fr, family, refs, build,
                 do_report):
    out = os.path.join(root, "15_beckhoff_ethercat", folder)
    os.makedirs(out, exist_ok=True)
    files = {"qet_directory": gs.directory([("en", title_en), ("fr", title_fr)])}
    for ref, rating in refs:
        axes = axes_of(ref)
        blocks, size = build(axes)
        sym = symbol(ref, axes, family, rating, blocks)
        files[ref.lower() + ".elmt"] = sym.xml()
        files[ref.lower() + "_realistic.elmt"] = realistic(
            ref, axes, family, rating, blocks, size).xml()
        files[ref.lower() + "_table.elmt"] = gs.table_of(sym).xml()
        if do_report:
            print("%s  axes %d  pins %d  max residual %.1f px"
                  % (ref, axes, sum(len(b.pins) for b in blocks),
                     report(ref, blocks)))
    for n, c in files.items():
        with open(os.path.join(out, n), "w", encoding="utf-8", newline="\n") as f:
            f.write(c)


def main(root, do_report=False):
    write_family(root, "drives_af1000", "AF1000 servo drives",
                 "Variateurs AF1000", "AF", AF_REFS, af_blocks, do_report)
    write_family(root, "drives_ax5000", "AX5000 servo drives",
                 "Variateurs AX5000", "AX", AX_REFS, ax_blocks, do_report)


if __name__ == "__main__":
    main(sys.argv[1], "--report" in sys.argv)
