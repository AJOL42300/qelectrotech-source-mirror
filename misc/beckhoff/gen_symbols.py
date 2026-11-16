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
"""Write the generic Beckhoff elements (symbols and I/O tables).

Usage: python misc/beckhoff/gen_symbols.py <extra-elements>
The uuids are derived from fixed names, so the output is reproducible.
"""
import os, sys, uuid

NS = uuid.UUID("6f1c2a5e-8b0f-4d57-9a7e-5b1d0c8e2f10")
FONT = "Liberation Sans,%d,-1,5,%d,0,0,0,0,0,%s"
STYLE = "line-style:normal;line-weight:%s;filling:%s;color:black"


ALL = {}
# channel n (0-based) -> Beckhoff terminal, 8-channel EL1008/EL2008 as drawn
# in the user's model: 1 and 5 on the first row, 2 and 6 on the second...
EL8_PINS = [1, 5, 2, 6, 3, 7, 4, 8]
# cards that also come as an I/O table appearance (<key>_table.elmt)
TABLES = ("el1008", "el2008", "el2652", "el5152", "ek1914", "epp2338")


def uid(*parts):
    return "{%s}" % uuid.uuid5(NS, "/".join(parts))


def esc(s):
    return s.replace("&", "&amp;").replace('"', "&quot;").replace("<", "&lt;")


class Elmt:
    def __init__(self, key, name_en, name_fr, infos, author_note):
        self.key, self.name_en, self.name_fr = key, name_en, name_fr
        self.infos, self.note = infos, author_note
        self.parts, self.xs, self.ys = [], [], []
        self.ios = []
        self.mode = "symbol"
        ALL[key] = self

    def column_visibility(self):
        # QET draws the I/O table of every PLC master at (0, 0) unless the
        # element has a plc_table part: hide it on drawings, show it on the
        # table appearance (with the Beckhoff terminal as 6th column).
        if self.mode == "table":
            return (["\t\t\t<columnVisibility>",
                     '\t\t\t\t<column index="5" visible="true"/>',
                     "\t\t\t</columnVisibility>"])
        return (["\t\t\t<columnVisibility>"]
                + ['\t\t\t\t<column index="%d" visible="false"/>' % i
                   for i in range(5)]
                + ["\t\t\t</columnVisibility>"])

    def io(self, kind, address, function="", comment="", term=True, pins=None):
        """A channel of the I/O table.

        address is the signal (I1, O3, SI2...); pins are the physical
        terminals of the card that carry it, in the Beckhoff order, which
        is not always the channel number. By default the terminal is
        named like the address.
        """
        if pins is None:
            pins = [address] if term else []
        self.ios.append((kind, address, function, comment, list(pins)))

    def _b(self, *xy):
        for i in range(0, len(xy), 2):
            self.xs.append(xy[i])
            self.ys.append(xy[i + 1])

    def rect(self, x, y, w, h, weight="normal", fill="none"):
        self._b(x, y, x + w, y + h)
        self.parts.append(
            '<rect x="%g" y="%g" width="%g" height="%g" rx="0" ry="0" '
            'style="%s" antialias="false"/>' % (x, y, w, h, STYLE % (weight, fill)))

    def circle(self, cx, cy, d, fill="white"):
        """Circle of diameter d centred on (cx, cy)."""
        self._b(cx - d / 2, cy - d / 2, cx + d / 2, cy + d / 2)
        self.parts.append(
            '<circle x="%g" y="%g" diameter="%g" style="%s" antialias="false"/>'
            % (cx - d / 2, cy - d / 2, d, STYLE % ("normal", fill)))

    def line(self, x1, y1, x2, y2, weight="thin"):
        self._b(x1, y1, x2, y2)
        self.parts.append(
            '<line x1="%g" y1="%g" x2="%g" y2="%g" end1="none" end2="none" '
            'length1="1.5" length2="1.5" style="%s" antialias="false"/>'
            % (x1, y1, x2, y2, STYLE % (weight, "none")))

    def text(self, x, y, s, size=7, bold=False):
        self._b(x, y)
        self.parts.append(
            '<text x="%g" y="%g" font="%s" rotation="0" color="#000000" text="%s"/>'
            % (x, y, FONT % (size, 75 if bold else 50, "Bold" if bold else "Regular"), esc(s)))

    def dyn(self, x, y, info, size=9, bold=False):
        self._b(x, y)
        self.parts.append(
            '<dynamic_text x="%g" y="%g" z="25" text_width="-1" Halignment="AlignLeft" '
            'Valignment="AlignTop" frame="false" rotation="0" keep_visual_rotation="true" '
            'text_from="ElementInfo" uuid="%s" font="%s">'
            "<text></text><info_name>%s</info_name></dynamic_text>"
            % (x, y, uid(self.key, "dyn", info), FONT % (size, 75 if bold else 50, "Bold" if bold else "Regular"), info))

    def terminal(self, x, y, name, orient, potential=""):
        self._b(x, y)
        self.parts.append(
            '<terminal x="%g" y="%g" name="%s" orientation="%s" type="Generic" uuid="%s"%s/>'
            % (x, y, esc(name), orient, uid(self.key, "term", name),
               (' potential="%s"' % esc(potential)) if potential else ""))

    def xml(self):
        w = int(max(self.xs) - min(self.xs)) + 10
        h = int(max(self.ys) - min(self.ys)) + 10
        hx, hy = int(-min(self.xs)) + 5, int(-min(self.ys)) + 5
        out = ['<definition version="0.100.0" type="element" link_type="%s" '
               'width="%d" height="%d" hotspot_x="%d" hotspot_y="%d">'
               % ("master" if self.ios else "simple", w, h, hx, hy),
               '\t<uuid uuid="%s"/>' % uid(self.key, "element"),
               "\t<names>",
               '\t\t<name lang="en">%s</name>' % esc(self.name_en),
               '\t\t<name lang="fr">%s</name>' % esc(self.name_fr),
               "\t</names>",
               "\t<elementInformations>"]
        for k, v in self.infos:
            out.append('\t\t<elementInformation name="%s" show="1">%s</elementInformation>' % (k, esc(v)))
        out.append("\t</elementInformations>")
        if self.ios:
            out += ["\t<kindInformations>",
                    '\t\t<kindInformation name="type">plc</kindInformation>',
                    '\t\t<plcMasterData rowHeight="8.00">']
            out += self.column_visibility()
            out += ["\t\t\t<plcIOs>"]
            for kind, address, function, comment, pins in self.ios:
                out += ['\t\t\t\t<plcIO type="%s" address="%s" functionText="%s" '
                        'comment="%s" crossRef="" terminalCount="%d">'
                        % (kind, esc(address), esc(function), esc(comment), len(pins))]
                for pin in pins:
                    out.append("\t\t\t\t\t<terminal>%s</terminal>" % esc(pin))
                out.append("\t\t\t\t</plcIO>")
            out += ["\t\t\t</plcIOs>", "\t\t</plcMasterData>",
                    "\t</kindInformations>"]
        out += ["\t<informations>%s</informations>" % esc(self.note),
                "\t<description>"]
        out += ["\t\t" + p for p in self.parts]
        out += ["\t</description>", "</definition>", ""]
        return "\n".join(out)


NOTE = ("Author: QElectroTech contributors\n"
        "Generic layout, not copied from a manufacturer drawing. "
        "License: see http://qelectrotech.org/wiki/doc/elements_license")


def card(key, designation, kind_en, kind_fr, short, io_kind, ref="", title="",
         channels=8, signals=None, pin_map=None):
    """Generic card with the contacts 1..channels, half left, half right.

    signals names the channel of each contact (default: the contact number);
    pin_map[i] is the Beckhoff terminal that carries channel i (default i).
    """
    infos = [("manufacturer", "Beckhoff")]
    if ref:
        infos.append(("manufacturer_reference", ref))
    infos += [("designation", title or designation),
              ("description", "EtherCAT Terminal, %s. Generic layout: contacts 1 to %d; "
               "the signal on each contact depends on the exact type, see its datasheet. "
               "The I/O table lists the %d contacts: edit it in the element properties."
               % (kind_en, channels, channels))]
    e = Elmt(key, "%s %s (generic)" % (designation, kind_en),
             "%s %s (générique)" % (designation, kind_fr), infos, NOTE)
    for i in range(channels):
        signal = signals[i] if signals else str(i + 1)
        pin = pin_map[i] if pin_map else i + 1
        e.io(io_kind, signal, pins=[str(pin)])
    half = 4 if channels > 4 else channels
    e.rect(-40, -90, 80, 180)
    e.text(-34, -86, "BECKHOFF", 5, True)
    e.text(-34, -72, designation, 9, True)
    e.text(-34, -56, short, 7)
    for i in range(half):
        y = -30 + 20 * i
        e.terminal(-40, y, str(i + 1), "w")
        e.line(-40, y, -30, y)
        e.text(-28, y - 8, str(i + 1), 6)
        if channels > 4:
            e.terminal(40, y, str(i + 5), "e")
            e.line(30, y, 40, y)
            e.text(22, y - 8, str(i + 5), 6)
    e.text(-34, 56, "see datasheet", 5)
    e.dyn(-40, 94, "label")
    return e


def el5152():
    """Encoder terminal: pin names and labels as drawn in the user's model."""
    e = Elmt("el5152", "EL5152 encoder input terminal (generic)",
             "EL5152 borne d'entrée codeur (générique)",
             [("manufacturer", "Beckhoff"), ("manufacturer_reference", "EL5152"),
              ("designation", "EL5152 EtherCAT Terminal"),
              ("description", "EtherCAT Terminal, two incremental encoder inputs A/B. "
               "Terminals numbered in the Beckhoff order, as in the user's drawing: "
               "1 A1, 2 +, 3 -, 4 A2 on the left; 5 B1, 6 +, 7 -, 8 B2 on the right. "
               "Check against the datasheet of your terminal.")], NOTE)
    # pin -> (label, potential, side)
    pins = {1: ("A1", "", "w"), 2: ("+", "24V", "w"), 3: ("-", "0V", "w"),
            4: ("A2", "", "w"), 5: ("B1", "", "e"), 6: ("+", "24V", "e"),
            7: ("-", "0V", "e"), 8: ("B2", "", "e")}
    for ch, pin, fn in (("A1", 1, "Encoder 1, A"), ("B1", 5, "Encoder 1, B"),
                        ("A2", 4, "Encoder 2, A"), ("B2", 8, "Encoder 2, B")):
        e.io("entree_digitale", ch, fn, pins=[str(pin)])
    e.rect(-40, -90, 80, 180)
    e.text(-34, -86, "BECKHOFF", 5, True)
    e.text(-34, -72, "EL5152", 9, True)
    e.text(-34, -56, "Encoder A/B", 7)
    for pin, (label, potential, side) in pins.items():
        y = -30 + 20 * ((pin - 1) % 4)
        x = -40 if side == "w" else 40
        e.terminal(x, y, str(pin), side, potential)
        e.line(x, y, x + (10 if side == "w" else -10), y)
        e.text(-28 if side == "w" else 22, y - 8, "%d %s" % (pin, label), 6)
    e.dyn(-40, 94, "label")
    return e


def table_of(src):
    """I/O table appearance of a card: same data, no drawing, no terminals."""
    e = Elmt(src.key + "_table", src.name_en + " - I/O table",
             src.name_fr + " - tableau d'E/S", list(src.infos), src.note)
    e.mode = "table"
    e.ios = list(src.ios)
    e.dyn(0, -24, "label", 9, True)
    e.dyn(60, -24, "designation", 7)
    e._b(0, -24, 380, 12 * (len(e.ios) + 2))
    e.parts.append('<plc_table x="0" y="0" style="%s" antialias="false"/>'
                   % (STYLE % ("thin", "none")))
    return e


def epp2338():
    e = Elmt("epp2338", "EPP2338 EtherCAT P box, 8 channels",
             "EPP2338 boîtier EtherCAT P, 8 voies",
             [("manufacturer", "Beckhoff"), ("manufacturer_reference", "EPP2338-0001"),
              ("designation", "EtherCAT P Box, 8-channel digital combi, 24 V DC, 10 us, 0.5 A, M8"),
              ("description", "EtherCAT P box with four M8 sockets X01 to X04, two channels "
               "each, EtherCAT P IN X50 and OUT X51. Generic layout, not the manufacturer "
               "drawing: put the picture under it. Fill in the function of each channel "
               "in the I/O table; the label of the box is the card id (A1.10).")], NOTE)
    e.rect(-60, -50, 120, 110)
    e.text(-54, -46, "EPP2338", 9, True)
    e.text(-54, -34, "EtherCAT P box", 5)
    e.terminal(-60, -20, "IN X50", "w")
    e.terminal(60, -20, "OUT X51", "e")
    e.line(-60, -20, -50, -20)
    e.line(50, -20, 60, -20)
    e.text(-54, -16, "IN X50", 5)
    e.text(30, -16, "OUT X51", 5)
    for i in range(4):
        y = 0 + 15 * i
        port = "X%02d" % (i + 1)
        e.terminal(60, y, port, "e")
        e.line(50, y, 60, y)
        e.text(30, y - 4, port, 5)
        e.io("entree_universelle", "%s(1)" % port, term=False)
        e.io("entree_universelle", "%s(2)" % port, term=False)
    e.dyn(-54, -4, "label", 10, True)
    e.dyn(-60, 64, "designation", 6)
    return e


def end_cap():
    e = Elmt("el9011", "EL9011 bus end cap", "EL9011 capot de fin de bus",
             [("manufacturer", "Beckhoff"), ("designation", "EL9011"),
              ("description", "EtherCAT bus end cap, closes the E-bus after the last terminal.")], NOTE)
    e.rect(-20, -60, 40, 120)
    e.text(-16, -56, "BECKHOFF", 4, True)
    e.text(-16, -40, "EL9011", 7, True)
    e.text(-16, -28, "End cap", 5)
    e.dyn(-20, 64, "label")
    return e


def box():
    e = Elmt("ethercat_box", "EtherCAT box (generic)", "Boîtier EtherCAT (générique)",
             [("description", "EtherCAT box, generic: one EtherCAT IN and one EtherCAT OUT "
               "for the daisy chain. Fill in manufacturer and reference.")], NOTE)
    e.rect(-60, -30, 120, 60)
    e.text(-54, -26, "EtherCAT", 6)
    e.text(-54, 14, "IN", 6)
    e.text(40, 14, "OUT", 6)
    e.terminal(-60, 0, "EtherCAT IN", "w")
    e.terminal(60, 0, "EtherCAT OUT", "e")
    e.line(-60, 0, -50, 0)
    e.line(50, 0, 60, 0)
    e.dyn(-54, -14, "label", 10, True)
    e.dyn(-60, 34, "designation", 7)
    return e


def coupler(key, ref, title, description, ports, ios=()):
    """EtherCAT coupler or junction: bus ports on the sides, optional I/O."""
    e = Elmt(key, "%s %s" % (ref, title), "%s %s" % (ref, title),
             [("manufacturer", "Beckhoff"), ("manufacturer_reference", ref),
              ("designation", title), ("description", description)], NOTE)
    for kind, address in ios:
        e.io(kind, address)
    west = [p for p in ports if p[1] == "w"]
    east = [p for p in ports if p[1] == "e"] + [(a, "e") for _, a in ios]
    h = 60 + 20 * max(len(west), len(east), 2)
    e.rect(-50, -h // 2, 100, h)
    e.text(-44, -h // 2 + 4, "BECKHOFF", 5, True)
    e.text(-44, -h // 2 + 16, ref, 9, True)
    for column in (west, east):
        for i, (name, side) in enumerate(column):
            y = -h // 2 + 40 + 20 * i
            x = -50 if side == "w" else 50
            e.terminal(x, y, name, side)
            e.line(x, y, x + (10 if side == "w" else -10), y)
            e.text(-44 if side == "w" else 6, y - 8, name, 5)
    e.dyn(-50, h // 2 + 4, "label")
    return e


def dir_entries():
    out = {}
    out["el1008.elmt"] = card(
        "el1008", "EL1008", "digital input terminal",
        "borne d'entrée numérique", "8 x DI 24 V DC", "entree_digitale",
        ref="EL1008",
        title="EtherCAT Terminal, 8-channel digital input, 24 V DC, 3 ms",
        signals=["I%d" % i for i in range(1, 9)], pin_map=EL8_PINS).xml()
    out["el2008.elmt"] = card(
        "el2008", "EL2008", "digital output terminal",
        "borne de sortie numérique", "8 x DO 24 V DC", "sortie_digitale",
        ref="EL2008",
        title="EtherCAT Terminal, 8-channel digital output, 24 V DC, 0.5 A",
        signals=["O%d" % i for i in range(1, 9)], pin_map=EL8_PINS).xml()
    out["el5152.elmt"] = el5152().xml()
    out["el2652.elmt"] = card(
        "el2652", "EL2652", "relay output terminal",
        "borne de sortie relais", "Relay output", "sortie_digitale",
        ref="EL2652", title="EL2652 EtherCAT Terminal, relay output",
        channels=4).xml()
    out["ek1100.elmt"] = coupler(
        "ek1100", "EK1100", "EtherCAT Coupler, 2 x RJ45",
        "EtherCAT coupler: the upper RJ45 socket goes to the network, the lower "
        "one to further EtherCAT devices. The terminals are the two bus sockets; "
        "the supply contacts are not drawn (generic layout). The label is the "
        "island id.",
        [("EtherCAT IN", "w"), ("EtherCAT OUT", "e")]).xml()
    out["ek1322.elmt"] = coupler(
        "ek1322", "EK1322", "2-port EtherCAT P junction with feed-in",
        "EtherCAT P junction for the EtherCAT Terminal segment. Generic layout: "
        "one EtherCAT P input and two EtherCAT P outputs named after the "
        "usual X50, X51 and X52 of the EtherCAT P boxes; check the exact ports "
        "of your device in its datasheet.",
        [("IN X50", "w"), ("OUT X51", "e"), ("OUT X52", "e")]).xml()
    out["ek1914.elmt"] = coupler(
        "ek1914", "EK1914", "EtherCAT Coupler, 4 standard inputs, 4 standard outputs, "
        "2 safe inputs, 2 safe outputs",
        "TwinSAFE EtherCAT coupler. Generic layout: the bus sockets, and the "
        "channels in the I/O table (names I1.., O1.., SI1.., SO1.. are generic, "
        "not the terminal marks of the datasheet).",
        [("EtherCAT IN", "w"), ("EtherCAT OUT", "e")],
        [("entree_digitale", "I%d" % i) for i in range(1, 5)]
        + [("entree_digitale", "SI%d" % i) for i in range(1, 3)]
        + [("sortie_digitale", "O%d" % i) for i in range(1, 5)]
        + [("sortie_digitale", "SO%d" % i) for i in range(1, 3)]).xml()
    return out


def directory(names):
    out = ["<qet-directory>", "\t<names>"]
    for lang, n in names:
        out.append('\t\t<name lang="%s">%s</name>' % (lang, esc(n)))
    out += ["\t</names>", "</qet-directory>", ""]
    return "\n".join(out)


def main(root):
    cat = os.path.join(root, "15_beckhoff_ethercat")
    os.makedirs(cat, exist_ok=True)
    files = {
        "qet_directory": directory([("en", "Beckhoff EtherCAT (generic)"),
                                    ("fr", "Beckhoff EtherCAT (générique)")]),
        "el1xxx_generic.elmt": card("el1xxx", "EL1xxx", "digital input terminal",
                                    "borne d'entrée numérique", "Digital input",
                                    "entree_digitale").xml(),
        "el2xxx_generic.elmt": card("el2xxx", "EL2xxx", "digital output terminal",
                                    "borne de sortie numérique", "Digital output",
                                    "sortie_digitale").xml(),
        "el3xxx_generic.elmt": card("el3xxx", "EL3xxx", "analog input terminal",
                                    "borne d'entrée analogique", "Analog input",
                                    "entree_analogique").xml(),
        "el4xxx_generic.elmt": card("el4xxx", "EL4xxx", "analog output terminal",
                                    "borne de sortie analogique", "Analog output",
                                    "sortie_analogique").xml(),
        "epp2338.elmt": epp2338().xml(),
        "el9011.elmt": end_cap().xml(),
        "ethercat_box_generic.elmt": box().xml(),
    }
    files.update(dir_entries())
    for key in TABLES:
        files[key + "_table.elmt"] = table_of(ALL[key]).xml()
    for n, c in files.items():
        with open(os.path.join(cat, n), "w", encoding="utf-8", newline="\n") as f:
            f.write(c)
    print(sorted(files))


if __name__ == "__main__":
    main(sys.argv[1])
