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
"""Make the 16 terminal version of the generic EL1xxx..EL4xxx terminals.

The generic terminals have 8 contacts (4 rows of 2).  The 16 terminal
version has 8 rows of 2 contacts in the same housing, the numbers are
written inside the circles.  It is made from the 8 terminal card: its
realistic view, its table and a skeleton of its symbol; the schematic and the
description views are then made by gen_schematic.py and gen_description.py.
The signal of each contact is not known: the contacts are numbered 1 to 16,
1 to 8 on the left and 9 to 16 on the right.

Usage: python gen_generic16.py <extra-elements>/10_electric/20_manufacturers_articles/beckhoff
"""
import copy
import glob
import os
import sys
import uuid
import xml.etree.ElementTree as ET

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from realistic_fixes import num, text_width, write  # noqa: E402

NS = uuid.UUID("6f1c2a5e-8b0f-4d57-9a7e-5b1d0c8e2f11")
FAMILIES = ("el1xxx", "el2xxx", "el3xxx", "el4xxx")
COLUMN_X = (-5.0, 10.0)
ROW_Y = [-35.0 + 22.0 * k for k in range(8)]
OUTER, INNER = 16.2, 12.6
NOTE = ("Generic card, 16 terminals: made by misc/beckhoff/gen_generic16.py "
        "from the 8 terminal card; the signal of each contact depends on the "
        "exact type. License: see "
        "http://qelectrotech.org/wiki/doc/elements_license")


def uid(*parts):
    return "{%s}" % uuid.uuid5(NS, "/".join(parts))


def retitle(root, stem):
    root.find("uuid").set("uuid", uid(stem, "element"))
    for n in root.find("names"):
        if n.get("lang") == "fr":
            n.text = n.text.replace("(générique)", "(générique, 16 bornes)")
            n.text = n.text.replace("tableau d'E/S", "tableau d'E/S")
        else:
            n.text = n.text.replace("(generic)", "(generic, 16 terminals)")
    for e in root.iter("elementInformation"):
        if e.get("name") == "description":
            e.text = e.text.replace("contacts 1 to 8", "contacts 1 to 16") \
                .replace("the 8 contacts", "the 16 contacts")
    info = root.find("informations")
    if info is not None:
        info.text = "Author: QElectroTech contributors\n" + NOTE


def ios16(root, first):
    ios = root.find("kindInformations").find("plcMasterData").find("plcIOs")
    kind = first.get("type")
    for io in list(ios):
        ios.remove(io)
    for k in range(1, 17):
        io = ET.SubElement(ios, "plcIO", {
            "type": kind, "address": str(k), "functionText": "",
            "comment": "", "crossRef": "", "terminalCount": "1"})
        ET.SubElement(io, "terminal").text = str(k)
    return ios


def pin_position(k):
    col, row = divmod(k - 1, 8)
    return COLUMN_X[col], ROW_Y[row], "w" if col == 0 else "e"


def realistic(src, stem, dst):
    root = ET.parse(src).getroot()
    retitle(root, stem + "_realistic")
    desc = root.find("description")
    for e in list(desc):
        if e.tag == "terminal" or e.tag == "circle":
            desc.remove(e)
        elif e.tag == "text" and ",75," not in e.get("font"):
            desc.remove(e)
    for e in desc.findall("text"):
        centre = float(e.get("x")) + text_width(e.get("text")) / 2
        e.set("text", "EL%sxxx" % stem[2])
        e.set("x", num(centre - text_width(e.get("text")) / 2))
    dyn = desc.find("dynamic_text")
    index = list(desc).index(dyn) if dyn is not None else len(desc)
    thin = "line-style:normal;line-weight:thin;filling:none;color:black"
    new = []
    for k in range(1, 17):
        x, y, orient = pin_position(k)
        for d in (OUTER, INNER):
            new.append(ET.Element("circle", {
                "x": num(x - d / 2), "y": num(y - d / 2), "style": thin,
                "antialias": "false", "diameter": num(d)}))
        width = text_width(str(k)) * 6 / 7.0
        new.append(ET.Element("text", {
            "x": num(x - width / 2), "y": num(y + 2.8), "rotation": "0",
            "color": "#000000",
            "font": "Roboto Condensed,6,-1,5,50,0,0,0,0,0,Regular",
            "text": str(k)}))
        new.append(ET.Element("terminal", {
            "x": num(x), "y": num(y), "name": str(k), "orientation": orient,
            "type": "Generic", "uuid": uid(stem, "terminal", str(k))}))
    for e in new:
        desc.insert(index, e)
        index += 1
    if dyn is not None:
        dyn.set("uuid", uid(stem, "dynamic", "realistic"))
    write(root, dst)
    return root


def table(src, stem, dst, ios_from):
    root = ET.parse(src).getroot()
    retitle(root, stem + "_table")
    mine = root.find("kindInformations").find("plcMasterData")
    mine.find("plcIOs").clear()
    master = root.find("kindInformations").find("plcMasterData")
    master[list(master).index(master.find("plcIOs"))] = copy.deepcopy(
        ios_from.find("kindInformations").find("plcMasterData")
        .find("plcIOs"))
    root.set("height", str(int(root.get("height")) + 8 * 12))
    for k, d in enumerate(root.find("description").findall("dynamic_text")):
        d.set("uuid", uid(stem, "table", "text%d" % k))
    write(root, dst)


def symbol(src, stem, dst):
    root = ET.parse(src).getroot()
    retitle(root, stem)
    first = next(root.iter("plcIO"))
    ios16(root, first)
    write(root, dst)
    return root


def main(base):
    for fam in FAMILIES:
        hits = glob.glob(os.path.join(base, "*", fam + "_generic",
                                      fam + "_generic.elmt"))
        folder8 = os.path.dirname(hits[0])
        stem = fam + "_generic_16"
        folder = os.path.join(os.path.dirname(folder8), stem)
        os.makedirs(folder, exist_ok=True)
        sym = symbol(hits[0], stem, os.path.join(folder, stem + ".elmt"))
        realistic(os.path.join(folder8, fam + "_generic_realistic.elmt"),
                  stem, os.path.join(folder, stem + "_realistic.elmt"))
        table(os.path.join(folder8, fam + "_generic_table.elmt"), stem,
              os.path.join(folder, stem + "_table.elmt"), sym)
        directory = ET.parse(os.path.join(folder8, "qet_directory")).getroot()
        for n in directory.iter("name"):
            n.text = n.text.replace("(generic)", "(generic, 16 terminals)") \
                .replace("(générique)", "(générique, 16 bornes)")
        ET.ElementTree(directory).write(
            os.path.join(folder, "qet_directory"), encoding="utf-8",
            xml_declaration=False)
        print("generic 16 terminals", stem)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
