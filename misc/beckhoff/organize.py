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
"""Sort the Beckhoff library into one folder per family.

Usage: python misc/beckhoff/organize.py <scratch root> <extra-elements>

The generators (gen_symbols.py, gen_safety.py, gen_drives.py and
realistic_from_model.py) write into a scratch root, in the layout
<scratch root>/15_beckhoff_ethercat[/twinsafe|/drives_*]. This script
replaces <extra-elements>/15_beckhoff_ethercat by the same elements, one
folder per family. All the appearances of a card (symbol, _realistic,
_table) stay in the same folder: the island commands look for them there.
"""
import os
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_symbols as gs

CATEGORY = "15_beckhoff_ethercat"

# folder -> (English name, French name, card keys)
FAMILIES = {
    "ek_couplers": ("EK couplers", "Coupleurs EK",
                    ("ek1100", "ek1322", "ek1914")),
    "el1xxx_digital_in": ("EL1xxx digital inputs", "EL1xxx entrées TOR",
                          ("el1004", "el1008", "el1xxx_generic")),
    "el2xxx_digital_out": ("EL2xxx digital outputs", "EL2xxx sorties TOR",
                           ("el2004", "el2008", "el2522", "el2574", "el2652",
                            "el2xxx_generic")),
    "el3xxx_analog_in": ("EL3xxx analog inputs", "EL3xxx entrées analogiques",
                         ("el3xxx_generic",)),
    "el4xxx_analog_out": ("EL4xxx analog outputs", "EL4xxx sorties analogiques",
                          ("el4xxx_generic",)),
    "el5xxx_encoder": ("EL5xxx position and encoder", "EL5xxx codeurs",
                       ("el5152",)),
    "el9xxx_system": ("EL9xxx system terminals", "EL9xxx bornes système",
                      ("el9011", "el9100")),
    "epp_box": ("EtherCAT P boxes", "Boîtiers EtherCAT P",
                ("epp2338", "ethercat_box_generic")),
    "twinsafe": ("TwinSAFE (yellow)", "TwinSAFE (jaunes)",
                 ("ek1960", "el1904", "el1918", "el2904", "el2911", "el6910")),
}
# folders written as they are by gen_drives.py
KEPT = ("drives_af1000", "drives_ax5000")


def family_of(key):
    for folder, (_, _, keys) in FAMILIES.items():
        if key in keys:
            return folder
    return None


def card_key(filename):
    """ek1914_realistic.elmt -> ek1914; el1xxx_generic.elmt is a card."""
    stem = filename[:-len(".elmt")]
    for suffix in ("_realistic", "_description", "_table"):
        if stem.endswith(suffix):
            return stem[:-len(suffix)]
    return stem


def main(scratch, elements):
    src = os.path.join(scratch, CATEGORY)
    dst = os.path.join(elements, CATEGORY)
    files = {}
    for folder, _, names in os.walk(src):
        for n in names:
            if n.endswith(".elmt"):
                files[n] = os.path.join(folder, n)
    out = {}
    for n, path in files.items():
        key = card_key(n)
        folder = family_of(key)
        if folder is None:
            rel = os.path.relpath(path, src)
            if rel.split(os.sep)[0] not in KEPT:
                raise SystemExit("no family for " + n)
            out[rel] = path
        else:
            out[os.path.join(folder, n)] = path
    if os.path.isdir(dst):
        shutil.rmtree(dst)
    os.makedirs(dst)
    for rel, path in sorted(out.items()):
        target = os.path.join(dst, rel)
        os.makedirs(os.path.dirname(target), exist_ok=True)
        shutil.copyfile(path, target)
    for folder in KEPT:
        shutil.copyfile(os.path.join(src, folder, "qet_directory"),
                        os.path.join(dst, folder, "qet_directory"))
    for folder, (en, fr, _) in FAMILIES.items():
        with open(os.path.join(dst, folder, "qet_directory"), "w",
                  encoding="utf-8", newline="\n") as f:
            f.write(gs.directory([("en", en), ("fr", fr)]))
    with open(os.path.join(dst, "qet_directory"), "w", encoding="utf-8",
              newline="\n") as f:
        f.write(gs.directory([("en", "Beckhoff EtherCAT"),
                              ("fr", "Beckhoff EtherCAT")]))
    print("%d elements in %d folders" % (len(out), len(FAMILIES) + len(KEPT)))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(2)
    main(*sys.argv[1:])
