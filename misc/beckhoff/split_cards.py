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
"""Move every card of the Beckhoff library into a folder of its own.

Usage: python misc/beckhoff/split_cards.py <library folder> [--git]

<family>/el1004.elmt, el1004_realistic.elmt, ... become
<family>/el1004/el1004.elmt, el1004/el1004_realistic.elmt, ... and the folder
gets a qet_directory named after the reference of the card ("EL1004").
Folders that are already split are left alone, so a second run does nothing.
With --git the files are moved with "git mv" to keep their history.
"""
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_symbols as gs
import organize


def display_name(key):
    """Name of the folder of a card: its reference, or the generic family."""
    if key.endswith("_generic"):
        if key.startswith("ethercat_box"):
            return ("EtherCAT box (generic)", "Boîtier EtherCAT (générique)")
        family = key[:2].upper() + key[2:-len("_generic")]
        return ("%s (generic)" % family, "%s (générique)" % family)
    return (key.upper(), key.upper())


def split(root, use_git=False):
    moved = 0
    for family in sorted(os.listdir(root)):
        folder = os.path.join(root, family)
        if not os.path.isdir(folder):
            continue
        for name in sorted(os.listdir(folder)):
            path = os.path.join(folder, name)
            if not (os.path.isfile(path) and name.endswith(".elmt")):
                continue
            key = organize.card_key(name)
            card = os.path.join(folder, key)
            os.makedirs(card, exist_ok=True)
            target = os.path.join(card, name)
            if use_git:
                subprocess.check_call(["git", "mv", path, target])
            else:
                os.replace(path, target)
            moved += 1
        for name in sorted(os.listdir(folder)):
            card = os.path.join(folder, name)
            directory = os.path.join(card, "qet_directory")
            if os.path.isdir(card) and not os.path.exists(directory):
                en, fr = display_name(name)
                with open(directory, "w", encoding="utf-8",
                          newline="\n") as f:
                    f.write(gs.directory([("en", en), ("fr", fr)]))
    return moved


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if a != "--git"]
    if len(args) != 1:
        print(__doc__)
        sys.exit(2)
    print("%d elements moved" % split(args[0], "--git" in sys.argv))
