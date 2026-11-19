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
"""Refresh the element definitions embedded in project files.

Every element of the embedded collection whose uuid is found in
extra-elements gets the current definition of that file (for example after
misc/scale_elements.py). The placed elements keep their position and their
terminal order. Run qelectrotech --resave on the result.

Usage: python misc/update_embedded_elements.py project.qet [...]
"""
import os
import sys
import xml.etree.ElementTree as ET

ROOT = os.path.normpath(os.path.join(os.path.dirname(
    os.path.abspath(__file__)), ".."))


def library():
    found = {}
    for dp, _, fn in os.walk(os.path.join(ROOT, "extra-elements")):
        for f in fn:
            if f.endswith(".elmt"):
                node = ET.parse(os.path.join(dp, f)).getroot()
                u = node.find("uuid")
                if u is not None:
                    found[u.get("uuid").lower()] = node
    return found


def refresh(path, lib):
    tree = ET.parse(path)
    count = 0
    for parent in tree.getroot().iter():
        for i, child in enumerate(list(parent)):
            if child.tag != "element" or child.find("definition") is None:
                continue
            old = child.find("definition")
            u = old.find("uuid")
            new = lib.get(u.get("uuid").lower()) if u is not None else None
            if new is None:
                continue
            child.remove(old)
            child.append(new)
            count += 1
    tree.write(path, encoding="utf-8", xml_declaration=True)
    return count


def main():
    lib = library()
    for path in sys.argv[1:]:
        print(path, refresh(path, lib), "definitions refreshed")


if __name__ == "__main__":
    main()
