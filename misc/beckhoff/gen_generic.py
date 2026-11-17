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
"""Give the generic terminals a realistic view and a table.

The generic EL1xxx / EL2xxx / EL3xxx / EL4xxx terminals and the EL2652 only
had a plain rectangle.  This script writes their <card>_realistic.elmt and
<card>_table.elmt from the realistic view and the table of a donor card
(same housing, same terminals 1 to 8), with the data (names, I/O table,
information) of the generic symbol.  The pins carry their number as label:
which signal is on which contact depends on the exact type.  The schematic
and the description views are then made by gen_schematic.py and
gen_description.py, like for the other cards.

Usage: python gen_generic.py <extra-elements>/15_beckhoff_ethercat
"""
import copy
import glob
import os
import sys
import uuid
import xml.etree.ElementTree as ET

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from realistic_fixes import num, text_width, write  # noqa: E402

NS = uuid.UUID("6f1c2a5e-8b0f-4d57-9a7e-5b1d0c8e2f10")
# generic card -> donor card of the same housing
DONORS = {
    "el1xxx_generic": "el1008", "el2xxx_generic": "el2008",
    "el3xxx_generic": "el1008", "el4xxx_generic": "el2008",
    "el2652": "el2004",
}
NOTE = ("Generic card: the realistic view and the table are made by "
        "misc/beckhoff/gen_generic.py from a donor card of the same "
        "housing; the signal of each contact depends on the exact type. "
        "License: see http://qelectrotech.org/wiki/doc/elements_license")


def uid(*parts):
    return "{%s}" % uuid.uuid5(NS, "/".join(parts))


def find(base, stem):
    hits = glob.glob(os.path.join(base, "*", stem + ".elmt"))
    return hits[0] if hits else None


def clone(donor_path, sym, stem, kind, suffix_en, suffix_fr):
    root = ET.parse(donor_path).getroot()
    root.find("uuid").set("uuid", uid("generic", kind, stem))
    for n in root.find("names"):
        base = sym.find("names").find("name[@lang='%s']" % n.get("lang"))
        extra = suffix_en if n.get("lang") == "en" else suffix_fr
        n.text = base.text + extra
    for tag in ("elementInformations", "kindInformations"):
        old, new = root.find(tag), sym.find(tag)
        if tag == "kindInformations" and kind == "table":
            # keep the columns of the donor table, take the I/O of the card
            old_ios = old.find("plcMasterData").find("plcIOs")
            new_ios = new.find("plcMasterData").find("plcIOs")
            master = old.find("plcMasterData")
            master[list(master).index(old_ios)] = copy.deepcopy(new_ios)
            continue
        root[list(root).index(old)] = copy.deepcopy(new)
    info = root.find("informations")
    if info is not None:
        info.text = "Author: QElectroTech contributors\n" + NOTE
    return root


def relabel(root, stem):
    desc = root.find("description")
    terms = desc.findall("terminal")
    for t in terms:
        t.set("uuid", uid("generic", "realistic", stem, t.get("name")))
        for att in ("potential",):
            if att in t.attrib:
                del t.attrib[att]
    for t in desc.findall("text"):
        if ",75," in t.get("font"):
            continue
        x = float(t.get("x")) + text_width(t.get("text")) / 2
        y = float(t.get("y"))
        best, dist = None, 12.0
        for p in terms:
            if -34 <= y - float(p.get("y")) <= -8:
                d = abs(x - float(p.get("x")))
                if d < dist:
                    best, dist = p, d
        if best is None:
            continue
        name = best.get("name")
        t.set("text", name)
        t.set("x", num(float(best.get("x")) - text_width(name) / 2))
    dyn = desc.find("dynamic_text")
    if dyn is not None:
        dyn.set("uuid", uid("generic", "realistic", stem, "label"))


def reference(root, text):
    for t in root.find("description").findall("text"):
        if ",75," in t.get("font"):
            width = text_width(t.get("text"))
            centre = float(t.get("x")) + width / 2
            t.set("text", text)
            t.set("x", num(centre - text_width(text) / 2))


def main(base):
    for stem, donor in DONORS.items():
        sym_path = find(base, stem)
        sym = ET.parse(sym_path).getroot()
        folder = os.path.dirname(sym_path)
        ref = None
        for e in sym.iter("elementInformation"):
            if e.get("name") == "designation":
                ref = e.text.split()[0]
        d_real = find(base, donor + "_realistic")
        root = clone(d_real, sym, stem, "realistic",
                     ", realistic view", ", vue réaliste")
        relabel(root, stem)
        reference(root, ref)
        write(root, os.path.join(folder, stem + "_realistic.elmt"))
        d_table = find(base, donor + "_table")
        root = clone(d_table, sym, stem, "table", ", I/O table",
                     ", tableau d'E/S")
        dyn = root.find("description").find("dynamic_text")
        if dyn is not None:
            dyn.set("uuid", uid("generic", "table", stem, "label"))
        for k, d in enumerate(root.find("description").findall(
                "dynamic_text")):
            d.set("uuid", uid("generic", "table", stem, "text%d" % k))
        write(root, os.path.join(folder, stem + "_table.elmt"))
        print("generic views", stem, "from", donor)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
