#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Colour the supply labels of the Beckhoff views by their function.

The code of the EL9410 view is generalised to all the cards, couplers, boxes
and drives: +24 V, Us, Up and DC+ in red, 0 V in blue, PE in orange. On the
realistic view (and on the housing labels of the description view) a small
coloured stroke is drawn above the label. In the schematic view and in the
legends of the description view only the colour of the text changes (the rails
of the schematic are already red, blue and orange). Signals stay black.

Run it after gen_schematic.py, gen_description.py and gen_hubs.py: it only
changes the colour of the texts and adds strokes, and can be run again.
The table views cannot be coloured cell by cell: see gen_table_titles.py.

Usage: python gen_colors.py <extra-elements>/10_electric/20_manufacturers_articles/beckhoff
"""
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from realistic_fixes import text_width  # noqa: E402

RED, BLUE, ORANGE = "#ff0000", "#0000ff", "#b87800"
STROKE = {RED: "red", BLUE: "blue", ORANGE: "orange"}
POSITIVE = re.compile(r"^(\+?24( ?V)?(_[A-C])?( DC)?|Us|Up|DC\+|\+)( |$)")
NEGATIVE = re.compile(r"^(0 ?V[1-4]?|\u2212|-)( |$)")
EARTH = re.compile(r"^PE( |$)")
TEXT = re.compile(r"<text [^>]*?/>")


def function_colour(text):
    """Colour of a supply label, None for the signals and the mixed texts."""
    if "/" in text or "GND" in text:
        return None
    if POSITIVE.match(text):
        return RED
    if NEGATIVE.match(text):
        return BLUE
    if EARTH.match(text):
        return ORANGE
    return None


def attribute(tag, name):
    found = re.search(r'\b%s="([^"]*)"' % name, tag)
    return found.group(1) if found else None


def stroke_for(tag, colour):
    x = float(attribute(tag, "x"))
    y = float(attribute(tag, "y"))
    size = float(attribute(tag, "font").split(",")[1])
    k = size / 7.0
    width = max(4.0, text_width(attribute(tag, "text")) * k + 1.5)
    return ('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" rx="0" ry="0" '
            'style="line-style:normal;line-weight:thin;filling:%s;color:%s" '
            'antialias="false" />'
            % (x + 1.4 * k, y + 1.4 * k, width, 1.6 * k,
               STROKE[colour], STROKE[colour]))


def recolour(path):
    name = os.path.basename(path)
    realistic = name.endswith("_realistic.elmt")
    description = name.endswith("_description.elmt")
    with open(path, encoding="utf-8", newline="") as f:
        text = f.read()
    newline = "\r\n" if "\r\n" in text else "\n"
    changed = [0]

    def replace(match):
        tag = match.group(0)
        colour = function_colour(attribute(tag, "text") or "")
        if colour is None:
            return tag
        current = attribute(tag, "color")
        housing = current == "#ffffff" or realistic
        if current == colour:
            return tag
        changed[0] += 1
        tag = re.sub(r'\bcolor="[^"]*"', 'color="%s"' % colour, tag)
        stroke = stroke_for(tag, colour)
        if (realistic or description) and housing and stroke not in text:
            # the stroke is drawn just before its label, so above the housing
            return stroke + newline + "\t\t" + tag
        return tag

    new = TEXT.sub(replace, text)
    if changed[0]:
        with open(path, "w", encoding="utf-8", newline="") as f:
            f.write(new)
    return changed[0]


def main(base):
    paths = sorted(p for p in glob.glob(os.path.join(base, "**", "*.elmt"),
                                        recursive=True)
                   if not p.endswith("_table.elmt"))
    total = sum(recolour(p) for p in paths)
    print("%d supply labels coloured in %d files" % (total, len(paths)))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
