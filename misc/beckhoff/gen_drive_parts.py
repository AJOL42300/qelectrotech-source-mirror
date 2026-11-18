#!/usr/bin/env python3
# Copyright 2026 QElectroTech contributors
# License: GNU GPL v2 or later, see the licence block of the repository.
"""Add the mating connectors of the Beckhoff drives as extra articles.

QElectroTech keeps up to four extra articles ("Parts" tab of an element, the
keys auxiliary1..4, manufacturer_auxiliaryN, manufacturer_reference_auxiliaryN,
quantity_auxiliaryN...). Every drive (AF1000 and AX5000) gets one article per
connector drawn on its terminals.

Only references found on the public Beckhoff product pages are written:

  AX5000 (1.5 to 25 A), connector set ZS4500-2010, with its single plugs
    ZS4500-2001  X01 mains input, 4-pin socket, max. 6 mm2
    ZS4500-2002  X02 DC link, 2-pin socket, max. 6 mm2
    ZS4500-2003  X03 24 V DC supply (Up, Us, GND), 3-pin socket
    ZS4500-2006  X06 BLFZ connector, single row, max. 1.5 mm2
  AF1000
    ZS4100-3113  X13/X23 motor plug, 4-pin, pitch 7.62 mm, three-phase
                 supply (ZS4100-1113: single-phase supply, pitch 5.08 mm)
    ZK1090-9191-0xxx  EtherCAT patch cable, RJ45 to RJ45 (xxx = length code)

The mains (X01) and I/O (X03) plugs of the AF1000 are NOT identified on a
public page: they are listed with a factual description and an empty
reference, to be completed by the user. QElectroTech has only four extra
articles, so the AX5000 list does not carry the EtherCAT patch cables, the
feedback D-sub plugs X11/X12/X21/X22 and the motor plugs, which are not
identified either.

The script is idempotent: it removes the keys it writes before writing them.

Usage: python gen_drive_parts.py <extra-elements>/10_electric/20_manufacturers_articles/beckhoff
"""
import os
import re
import sys
from xml.sax.saxutils import escape

FIELDS = ("auxiliary", "description", "manufacturer",
          "manufacturer_reference", "designation", "quantity", "unity")


def key(field, n):
    return "auxiliary%d" % n if field == "auxiliary" \
        else "%s_auxiliary%d" % (field, n)


def part(block, label, description, reference, quantity, designation=""):
    return {"auxiliary": label, "description": description,
            "manufacturer": "Beckhoff" if reference else "",
            "manufacturer_reference": reference,
            "designation": designation, "quantity": str(quantity),
            "unity": "1", "block": block}


def af_parts(axes):
    motor = ("X13" if axes == 1 else "X13 and X23")
    return [
        part(1, "EtherCAT cable X04/X05",
             "EtherCAT patch cable, RJ45 to RJ45, green PUR, Cat.5. The last "
             "digits of the reference give the length (for example 0010 = "
             "1 m). One cable counted for the IN port; the cable of the OUT "
             "port belongs to the next device.",
             "ZK1090-9191-0xxx", 1, "EtherCAT patch cable"),
        part(2, "Motor plug %s" % motor,
             "4-pin plug U V W PE, pitch 7.62 mm, push-in, for the slot "
             "X13/X23, three-phase supply version, sold by 10 (PU 10); one "
             "plug per axis. The single-phase supply version is ZS4100-1113 "
             "(pitch 5.08 mm).",
             "ZS4100-3113", axes, "Motor connector AF1000"),
        part(3, "I/O plug X03",
             "Plug of the X03 terminal strip (IN1..IN6, GND, TH, OUT, "
             "FGND, 24V). Reference NOT identified on the public Beckhoff "
             "pages: to be completed from the AF1000 documentation.",
             "", 1, "X03 connector"),
        part(4, "Mains plug X01",
             "Plug of the X01 terminal strip (L1 L2 L3 PE RB+ RB-). "
             "Reference NOT identified on the public Beckhoff pages: to be "
             "completed from the AF1000 documentation.",
             "", 1, "X01 connector"),
    ]


def ax_parts(axes):
    return [
        part(1, "Mains plug X01",
             "4-pin socket, pitch 7.62 mm, max. 6 mm2, for 1.5 to 25 A. "
             "Included in the scope of supply of the drive, also in the "
             "connector set ZS4500-2010.",
             "ZS4500-2001", 1, "X01 mains input connector"),
        part(2, "DC link plug X02",
             "2-pin socket, max. 6 mm2, for 1.5 to 25 A. Included in the "
             "scope of supply of the drive, also in the connector set "
             "ZS4500-2010.",
             "ZS4500-2002", 1, "X02 DC link connector"),
        part(3, "24 V supply plug X03",
             "3-pin socket (Up, Us, GND), max. 6 mm2. Included in the "
             "scope of supply of the drive, also in the connector set "
             "ZS4500-2010.",
             "ZS4500-2003", 1, "X03 24 V DC supply connector"),
        part(4, "I/O plug X06",
             "BLFZ connector, single row without LED, max. 1.5 mm2, for "
             "the terminals 24, 0..7, 0V. Included in the scope of supply "
             "of the drive, also in the connector set ZS4500-2010.",
             "ZS4500-2006", 1, "X06 BLFZ connector"),
    ]


def parts_for(name):
    m = re.match(r"(af|ax)(\d)(\d)", name)
    if not m:
        return None
    axes = int(m.group(3))
    return af_parts(axes) if m.group(1) == "af" else ax_parts(axes)


def rewrite(text, parts):
    for n in range(1, 5):
        for f in FIELDS:
            text = re.sub(r'[ \t]*<elementInformation name="%s"[^>]*?(/>|>[^<]*'
                          r'</elementInformation>)\r?\n' % key(f, n), "", text)
    lines = []
    for p in parts:
        for f in FIELDS:
            if f == "manufacturer_reference" and not p[f]:
                value = ""
            else:
                value = p[f]
            if value == "" and f in ("manufacturer", "manufacturer_reference"):
                continue
            lines.append('\t\t<elementInformation name="%s" show="1">%s'
                         '</elementInformation>\n'
                         % (key(f, p["block"]), escape(value)))
    new = "".join(lines)
    eol = "\r\n" if "\r\n" in text else "\n"
    if eol == "\r\n":
        new = new.replace("\n", "\r\n")
    return re.sub(r"([ \t]*</elementInformations>)", lambda m: new + m.group(1),
                  text, count=1)


def main(folder):
    count = 0
    for dirpath, _, files in os.walk(folder):
        for f in files:
            if not f.endswith(".elmt"):
                continue
            parts = parts_for(f)
            if not parts:
                continue
            path = os.path.join(dirpath, f)
            text = open(path, encoding="utf-8", newline="").read()
            if "</elementInformations>" not in text:
                continue
            new = rewrite(text, parts)
            if new != text:
                open(path, "w", encoding="utf-8", newline="").write(new)
                count += 1
    print("%d drive files completed with their connector articles" % count)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
