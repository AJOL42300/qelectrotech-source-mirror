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
"""Give the title line of every *_table.elmt a filled background.

The line shows, on one horizontal axis (centred on the height of the band):
the label of the component (for example A1.10), the reference of the card
(for example EL2008, in bold) and its designation.  The table itself (header
and rows) is drawn by QElectroTech, which cannot colour a cell: the colour
code of the terminals is on the other views.

Run it last (after gen_hubs.py, gen_generic*.py, ...): it rewrites only the
<description> of the table views and can be run again.

Usage: python gen_table_titles.py <extra-elements>/10_electric/20_manufacturers_articles/beckhoff
"""
import glob
import os
import re
import sys
import uuid

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from realistic_fixes import text_width  # noqa: E402

NS = uuid.UUID("2b7a9c64-3d5e-4f10-8a6b-1c9d0e7f5a32")
BAND_Y, BAND_H = -22, 20
CENTRE = BAND_Y + BAND_H / 2.0
STYLE = "line-style:normal;line-weight:thin;filling:lightgray;color:black"
PLAIN = "line-style:normal;line-weight:thin;filling:none;color:black"
FONT = "Liberation Sans,%d,-1,5,%d,0,0,0,0,0,%s"


def dynamic(x, info, size, bold, key, stem, wrap=-1, lines=1):
    y = round(CENTRE - size * 1.8 * lines / 2.0, 1)
    return (
        '\t\t<dynamic_text x="%s" y="%s" z="25" text_width="%s" '
        'Halignment="AlignLeft" Valignment="AlignTop" frame="false" '
        'rotation="0" keep_visual_rotation="true" text_from="ElementInfo" '
        'uuid="{%s}" font="%s"><text></text><info_name>%s</info_name>'
        '</dynamic_text>\n'
        % (x, y, wrap, uuid.uuid5(NS, stem + "/" + key),
           FONT % (size, 75 if bold else 50, "Bold" if bold else "Regular"),
           info))


def visible_width(text):
    widths = [int(w) for w in re.findall(
        r'<column index="\d+" width="(\d+)"/>',
        re.search(r"<columnWidths>.*?</columnWidths>", text, re.S).group(0))]
    visible = re.findall(
        r'<column index="(\d+)" visible="(true|false)"/>', text)
    return sum(widths[int(i)] for i, v in visible if v == "true")


def retitle(path):
    with open(path, encoding="utf-8", newline="") as f:
        text = f.read()
    stem = os.path.basename(path)[:-len("_table.elmt")]
    found = re.search(
        r'name="manufacturer_reference"[^>]*>([^<]*)<', text)
    reference = found.group(1) if found else ""
    designation = re.search(
        r'name="designation"[^>]*>([^<]*)<', text).group(1)
    width = visible_width(text)
    ref_x = 38.0
    des_x = ref_x + (text_width(reference) * 8 / 7.0 + 8
                     if reference else 0)
    room = width - des_x - 3
    size = 6 if text_width(designation) * 6 / 7.0 <= room else 5
    lines = 1 if text_width(designation) * size / 7.0 <= room else 2
    block = (
        "\t<description>\n"
        '\t\t<rect x="0" y="%s" width="%s" height="%s" rx="0" ry="0" '
        'style="%s" antialias="false"/>\n' % (BAND_Y, width, BAND_H, STYLE)
        + dynamic(3, "label", 9, True, "label", stem)
        + (dynamic(ref_x, "manufacturer_reference", 8, True, "reference",
                   stem) if reference else "")
        + dynamic(round(des_x, 1), "designation", size, False, "title", stem,
                round(room), lines)
        + '\t\t<plc_table x="0" y="0" style="%s" antialias="false"/>\n'
        % PLAIN + "\t</description>\n")
    text, count = re.subn(r"\t<description>.*?\t</description>\r?\n",
                          lambda m: block, text, count=1, flags=re.S)
    if not count:
        sys.exit("no <description> block in " + path)
    # the band must stay inside the declared box
    m = re.search(r'hotspot_y="(\d+)"', text)
    if m and int(m.group(1)) < -BAND_Y + 2:
        top = -BAND_Y + 2
        grow = top - int(m.group(1))
        text = re.sub(r'hotspot_y="\d+"', 'hotspot_y="%d"' % top, text, 1)
        text = re.sub(r' height="(\d+)"',
                      lambda k: ' height="%d"' % (int(k.group(1)) + grow),
                      text, count=1)
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(text)


def main(base):
    paths = sorted(glob.glob(os.path.join(base, "**", "*_table.elmt"),
                             recursive=True))
    for p in paths:
        retitle(p)
    print("%d table titles written" % len(paths))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
