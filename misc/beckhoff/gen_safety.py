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
"""Write the Beckhoff TwinSAFE ("yellow") symbols and I/O tables.

Usage: python misc/beckhoff/gen_safety.py <extra-elements>

None of these references is in the user's project: the layouts are generic
and the terminal numbering is NOT verified against the datasheets (see
docs/beckhoff-ethercat.md). Terminals are named with the physical terminal
number, the I/O table maps it to the safe channel (SI1, SO2...).
"""
import os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_symbols as gs
from gen_symbols import Elmt, NOTE

YELLOW = "yellow"
GENERIC = ("Terminal numbering follows the user's own drawing and is NOT "
           "verified against the datasheet: check it before use.")

# EL1918 terminal order from the public Beckhoff documentation (factual list
# only): input n -> (clock output terminal, safe input terminal)
EL1918_ORDER = (1, 3, 5, 7, 2, 4, 6, 8)
EL1918_TERMINALS = {n: (2 * k + 1, 2 * k + 2)
                    for k, n in enumerate(EL1918_ORDER)}
EL1918_NOTE = ("16 terminals (user request, public Beckhoff list). The "
               "terminal order is documented, the physical arrangement "
               "(rows of two) is an assumption: check it before use.")


class Safety:
    def __init__(self, key, ref, title, desc, pins, ios, rest=None,
                 tabled=True, note=GENERIC):
        self.key, self.ref, self.title, self.desc = key, ref, title, desc
        self.pins = pins      # terminal numbers, in physical order
        self.ios = ios        # (kind, signal, [terminals])
        self.rest = rest or {}  # terminal -> label of the terminals without channel
        self.tabled = tabled
        self.note = note

    def infos(self):
        return [("manufacturer", "Beckhoff"), ("manufacturer_reference", self.ref),
                ("designation", self.title),
                ("description", "%s %s" % (self.desc, self.note))]

    def label_of(self, pin):
        if pin in self.rest:
            return self.rest[pin]
        for _, signal, pins in self.ios:
            if pin in pins:
                return signal if pins[0] == pin else "%s (%d)" % (
                    signal, pins.index(pin) + 1)
        return ""

    def add_io(self, e):
        for kind, signal, pins in self.ios:
            e.io(kind, signal, pins=list(pins))

    def split(self):
        half = (len(self.pins) + 1) // 2
        return self.pins[:half], self.pins[half:]

    def symbol(self):
        e = Elmt(self.key, "%s %s" % (self.ref, self.title),
                 "%s %s" % (self.ref, self.title), self.infos(), NOTE)
        self.add_io(e)
        left, right = self.split()
        rows = max(len(left), len(right), 1)
        h = 40 + 20 * rows + 20
        e.rect(-50, 0, 100, h, fill=YELLOW)
        e.text(-44, 6, "BECKHOFF", 5, True)
        e.text(-44, 18, self.ref, 9, True)
        for column, x, sign in ((left, -50, -1), (right, 50, 1)):
            for i, pin in enumerate(column):
                y = 40 + 20 * i
                e.terminal(x, y, pin, "w" if sign < 0 else "e",
                           gs.potential_of(self.label_of(pin)))
                e.line(x, y, x - 10 * sign, y)
                text = "%s %s" % (pin, self.label_of(pin)) if sign < 0 else \
                    "%s %s" % (self.label_of(pin), pin)
                e.text(-44 if sign < 0 else 44 - 4.2 * len(text), y - 6,
                       text.strip(), 6)
        e.dyn(-50, h + 4, "label")
        e.dyn(-50, h + 16, "designation", 6)
        return e


def plain(key, ref, title, desc, ports):
    """Safety module without field terminals (logic): only bus sockets."""
    e = Elmt(key, "%s %s" % (ref, title), "%s %s" % (ref, title),
             [("manufacturer", "Beckhoff"), ("manufacturer_reference", ref),
              ("designation", title), ("description", "%s %s" % (desc, GENERIC))],
             NOTE)
    h = 80 + 20 * len(ports)
    e.rect(-50, 0, 100, h, fill=YELLOW)
    e.text(-44, 6, "BECKHOFF", 5, True)
    e.text(-44, 18, ref, 9, True)
    for i, (name, side) in enumerate(ports):
        y = 40 + 20 * i
        x = -50 if side == "w" else 50
        e.terminal(x, y, name, side)
        e.line(x, y, x + (10 if side == "w" else -10), y)
        e.text(-44 if side == "w" else 44 - 4.2 * len(name), y - 6, name, 6)
    e.dyn(-50, h + 4, "label")
    return e


def cards():
    # pin-out read on the user's own drawings: the safe channels sit on the
    # first terminal of each pair of the rows, the other terminals are 0 V
    rows = {1: "1", 2: "5", 3: "3", 4: "7"}
    return [
        Safety("el1904", "EL1904", "EtherCAT Terminal, 4 fail-safe digital inputs, 24 V DC",
               "TwinSAFE input terminal: SI1 to SI4 on the terminals 1, 5, 3 and 7; "
               "the terminals 2, 6, 4 and 8 are 0 V.",
               [str(i) for i in range(1, 9)],
               [("entree_digitale", "SI%d" % i, [rows[i]]) for i in range(1, 5)],
               {"2": "0V1", "6": "0V2", "4": "0V3", "8": "0V4"}),
        Safety("el1918", "EL1918", "EtherCAT Terminal, 8 fail-safe digital inputs, 24 V DC",
               "TwinSAFE input terminal with 16 terminals: for each input n, "
               "a clock output Sn+ and a safe input SIn (terminal Sn-). The "
               "order is 1, 3, 5, 7, 2, 4, 6, 8 (terminals 1 to 16).",
               [str(i) for i in range(1, 17)],
               [("entree_digitale", "SI%d" % n, [str(EL1918_TERMINALS[n][1])])
                for n in range(1, 9)],
               {str(EL1918_TERMINALS[n][0]): "S%d+" % n for n in range(1, 9)},
               note=EL1918_NOTE),
        Safety("el2904", "EL2904", "EtherCAT Terminal, 4 fail-safe digital outputs, 24 V DC",
               "TwinSAFE output terminal: SO1 to SO4 on the terminals 1, 5, 3 and 7; "
               "the terminals 2, 6, 4 and 8 are 0 V.",
               [str(i) for i in range(1, 9)],
               [("sortie_digitale", "SO%d" % i, [rows[i]]) for i in range(1, 5)],
               {"2": "0V1", "6": "0V2", "4": "0V3", "8": "0V4"}),
    ]


def main(root):
    out = os.path.join(root, "15_beckhoff_ethercat", "twinsafe")
    os.makedirs(out, exist_ok=True)
    files = {"qet_directory": gs.directory([
        ("en", "Beckhoff TwinSAFE (generic)"),
        ("fr", "Beckhoff TwinSAFE (générique)")])}
    for c in cards():
        sym = c.symbol()
        files[c.key + ".elmt"] = sym.xml()
        files[c.key + "_table.elmt"] = gs.table_of(sym).xml()
    for key, ref, title, desc, ports in (
            ("el6910", "EL6910", "TwinSAFE Logic Terminal",
             "TwinSAFE logic terminal, no field terminals.", []),
            ("ek1960", "EK1960", "TwinSAFE Logic EtherCAT coupler",
             "TwinSAFE logic coupler with two EtherCAT sockets.",
             [("EtherCAT IN", "w"), ("EtherCAT OUT", "e")])):
        files[key + ".elmt"] = plain(key, ref, title, desc, ports).xml()
    for n, c in files.items():
        with open(os.path.join(out, n), "w", encoding="utf-8", newline="\n") as f:
            f.write(c)


if __name__ == "__main__":
    main(sys.argv[1])
