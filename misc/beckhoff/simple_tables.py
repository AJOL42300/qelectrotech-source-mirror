#!/usr/bin/env python3
# Copyright 2006-2026 The QElectroTech Team
# This file is part of QElectroTech (GNU GPL v2 or later).
"""Give every *_table.elmt of the Beckhoff library the simple table layout.

The table shows three columns only: pin number | label | TwinCAT ID.
They are columns of the existing PLC table, chosen by the element file:

    index 5  Terminal  -> "NÂ° pin"      (the pin of the card, e.g. 5)
    index 2  Function  -> "Label"       (the name of the channel, filled in
                                         by the user or by the TwinCAT sync)
    index 1  Address   -> "ID TwinCAT"  (channel key: direction letter and
                                         channel number, e.g. I3, Q2, SI1)

The other columns (type, comment, cross-reference) are hidden, not removed:
the I/O data of the card is unchanged, and the tooltips, the TwinCAT sync
and the export still read it. Nothing else is touched.

Usage: simple_tables.py [collection_dir]   (default: the Beckhoff folder)
"""
import os
import re
import sys

DEFAULT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..",
                       "extra-elements", "10_electric",
                       "20_manufacturers_articles", "beckhoff")

W_PIN, W_LABEL, W_ID = 44, 130, 66
ROW_H = 12
HOTSPOT_X, HOTSPOT_Y = 5, 29

LAYOUT = (
    "\t\t\t<columnWidths>\n"
    '\t\t\t\t<column index="0" width="60"/>\n'
    '\t\t\t\t<column index="1" width="%d"/>\n'
    '\t\t\t\t<column index="2" width="%d"/>\n'
    '\t\t\t\t<column index="3" width="90"/>\n'
    '\t\t\t\t<column index="4" width="64"/>\n'
    '\t\t\t\t<column index="5" width="%d"/>\n'
    "\t\t\t</columnWidths>\n"
    "\t\t\t<columnVisibility>\n"
    '\t\t\t\t<column index="0" visible="false"/>\n'
    '\t\t\t\t<column index="1" visible="true"/>\n'
    '\t\t\t\t<column index="2" visible="true"/>\n'
    '\t\t\t\t<column index="3" visible="false"/>\n'
    '\t\t\t\t<column index="4" visible="false"/>\n'
    '\t\t\t\t<column index="5" visible="true"/>\n'
    "\t\t\t</columnVisibility>\n"
    "\t\t\t<columnNames>\n"
    '\t\t\t\t<column index="0"></column>\n'
    '\t\t\t\t<column index="1">ID TwinCAT</column>\n'
    '\t\t\t\t<column index="2">Label</column>\n'
    '\t\t\t\t<column index="3"></column>\n'
    '\t\t\t\t<column index="4"></column>\n'
    '\t\t\t\t<column index="5">N\u00b0 pin</column>\n'
    "\t\t\t</columnNames>\n"
    "\t\t\t<columnOrder>5,2,1</columnOrder>\n"
) % (W_ID, W_LABEL, W_PIN)

BLOCK = re.compile(
    r"\t\t\t<columnWidths>.*?</columnVisibility>\n"
    r"(?:\t\t\t<columnNames>.*?</columnNames>\n)?"
    r"(?:\t\t\t<columnOrder>.*?</columnOrder>\n)?", re.S)


def convert(text):
    rows = len(re.findall(r"<plcIO ", text))
    out, n = BLOCK.subn(lambda m: LAYOUT, text, count=1)
    if n != 1:
        raise ValueError("no column block")
    width = HOTSPOT_X + W_PIN + W_LABEL + W_ID + 5
    height = HOTSPOT_Y + (ROW_H + 2) + ROW_H * rows + 15
    out = re.sub(r'(<definition [^>]*?)width="\d+" height="\d+"',
                 r'\g<1>width="%d" height="%d"' % (width, height), out, 1)
    return out


def main(argv):
    root = os.path.abspath(argv[1] if len(argv) > 1 else DEFAULT)
    count = 0
    for base, _dirs, files in os.walk(root):
        for name in files:
            if not name.endswith("_table.elmt"):
                continue
            path = os.path.join(base, name)
            with open(path, "r", encoding="utf-8") as f:
                text = f.read()
            new = convert(text)
            if new != text:
                with open(path, "w", encoding="utf-8", newline="\n") as f:
                    f.write(new)
            count += 1
    print("%d table files laid out as pin | label | TwinCAT ID" % count)


if __name__ == "__main__":
    main(sys.argv)
