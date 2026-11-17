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
TABLES = ("el1004", "el1008", "el2004", "el2008", "el2522", "el2574",
          "el2652", "el2911", "el5152", "ek1914", "epp2338")
# I/O table appearance: Roboto Condensed (QET falls back to Arial Narrow),
# rows 12 high, columns Type, Address, Function, Comment, Cross-reference,
# Terminal
TABLE_FONT = "Roboto Condensed"
TABLE_SIZE = 7
TABLE_ROW = 12
TABLE_COLUMNS = (60, 36, 110, 90, 64, 40)


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
            out = ["\t\t\t<columnWidths>"]
            out += ['\t\t\t\t<column index="%d" width="%g"/>' % (i, w)
                    for i, w in enumerate(TABLE_COLUMNS)]
            out += ["\t\t\t</columnWidths>", "\t\t\t<columnVisibility>",
                    '\t\t\t\t<column index="5" visible="true"/>',
                    "\t\t\t</columnVisibility>",
                    '\t\t\t<headerFont family="%s" size="%d" bold="true"/>'
                    % (TABLE_FONT, TABLE_SIZE),
                    '\t\t\t<cellFont family="%s" size="%d" bold="false"/>'
                    % (TABLE_FONT, TABLE_SIZE)]
            return out
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

    def rect(self, x, y, w, h, weight="normal", fill="none", dash="normal"):
        self._b(x, y, x + w, y + h)
        self.parts.append(
            '<rect x="%g" y="%g" width="%g" height="%g" rx="0" ry="0" '
            'style="%s" antialias="false"/>'
            % (x, y, w, h,
               (STYLE % (weight, fill)).replace("style:normal", "style:" + dash)))

    def circle(self, cx, cy, d, fill="white"):
        """Circle of diameter d centred on (cx, cy)."""
        self._b(cx - d / 2, cy - d / 2, cx + d / 2, cy + d / 2)
        self.parts.append(
            '<circle x="%g" y="%g" diameter="%g" style="%s" antialias="false"/>'
            % (cx - d / 2, cy - d / 2, d, STYLE % ("normal", fill)))

    def line(self, x1, y1, x2, y2, weight="thin", dash="normal"):
        self._b(x1, y1, x2, y2)
        self.parts.append(
            '<line x1="%g" y1="%g" x2="%g" y2="%g" end1="none" end2="none" '
            'length1="1.5" length2="1.5" style="%s" antialias="false"/>'
            % (x1, y1, x2, y2,
               (STYLE % (weight, "none")).replace("style:normal", "style:" + dash)))

    def text(self, x, y, s, size=7, bold=False, color="#000000",
             family="Liberation Sans"):
        self._b(x, y)
        self.parts.append(
            '<text x="%g" y="%g" font="%s" rotation="0" color="%s" text="%s"/>'
            % (x, y, (FONT % (size, 75 if bold else 50, "Bold" if bold else "Regular"))
               .replace("Liberation Sans", family, 1), color, esc(s)))

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
                    '\t\t<plcMasterData rowHeight="%.2f">'
                    % (TABLE_ROW if self.mode == "table" else 8)]
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


def potential_of(label):
    if label.startswith("0V") or label.startswith("GND"):
        return "0V"
    if label.startswith("24V"):
        return "24V"
    return ""


# Cards whose pin-out is read on the user's own drawings. rows are the
# printed labels, row by row, as (left, right); the Beckhoff terminal number
# follows the position: the left column is 1 to 4 (top to bottom), the right
# column 5 to 8 (so row 1 is the pins 1 and 5). A second group of four rows
# continues with 9 to 16. The channel (I1, O3...) is not the pin number.
#   key: (reference, kind, English, French, short text, designation, rows,
#         channel names {label: signal}, I/O type)
MODEL_CARDS = {
    "el1004": ("EL1004", "digital input terminal", "borne d'entrée numérique",
               "4 x DI 24 V DC",
               "EtherCAT Terminal, 4-channel digital input, 24 V DC, 3 ms",
               [("I1", "I2"), ("0V1", "0V2"), ("I3", "I4"), ("0V3", "0V4")],
               "entree_digitale"),
    "el2004": ("EL2004", "digital output terminal", "borne de sortie numérique",
               "4 x DO 24 V DC",
               "EtherCAT Terminal, 4-channel digital output, 24 V DC, 0.5 A",
               [("Q1", "Q2"), ("0V1", "0V2"), ("Q3", "Q4"), ("0V3", "0V4")],
               "sortie_digitale"),
    "el2522": ("EL2522", "pulse train output terminal",
               "borne de sortie train d'impulsions", "2 x pulse train",
               "EtherCAT Terminal, 2-channel pulse train output",
               [("CH1A", "CH1B"), ("CH1C", "CH1D"), ("CH2A", "CH2B"),
                ("CH2C", "CH2D")], "sortie_digitale"),
    "el2574": ("EL2574", "LED output terminal", "borne de sortie LED",
               "4 x P+ / P-",
               "EtherCAT Terminal, 4-channel output, labels as drawn",
               [("P1+", "P1-"), ("P2+", "P2-"), ("P3+", "P3-"),
                ("P4+", "P4-")], "sortie_digitale"),
    "el5152": ("EL5152", "encoder input terminal", "borne d'entrée codeur",
               "Encoder A/B", "EtherCAT Terminal, 2-channel incremental "
               "encoder input", [("A1", "A2"), ("B1", "B2"),
                                 ("24V1", "0V1"), ("24V2", "0V2")],
               "entree_digitale"),
    "el2911": ("EL2911", "safety terminal", "borne de sécurité",
               "IN1-4 / OUT",
               "EtherCAT Terminal, labels as drawn (drawing doubled)",
               [("IN1", "IN2"), ("OUT", "24V_UP"), ("GND_UP", "24V"),
                ("IN3", "IN4"), ("IN1", "IN2"), ("OUT", "24V_UP"),
                ("GND_UP", "24V"), ("IN3", "IN4")], "entree_digitale"),
    "el9100": ("EL9100", "potential feed terminal",
               "borne d'alimentation de potentiel", "END / NC",
               "EtherCAT Terminal, labels as drawn",
               [("END1", "END2"), ("NC1", "NC2"), ("NC3", "NC4"),
                ("NC5", "NC6")], None),
}
# the channel is the label itself unless named here
CHANNELS_ONLY = {"el2911": ("IN1", "IN2", "IN3", "IN4", "OUT")}
FUNCTIONS = {"el5152": {"A1": "Encoder 1, A", "A2": "Encoder 2, A",
                        "B1": "Encoder 1, B", "B2": "Encoder 2, B"}}


def model_pins(rows):
    """(pin, label, side, row) of every terminal, in the Beckhoff order."""
    out = []
    for r, (left, right) in enumerate(rows):
        base = 8 * (r // 4) + r % 4 + 1
        out.append((base, left, "w", r))
        out.append((base + 4, right, "e", r))
    return out


def model_card(key, uncertain=True):
    ref, en, fr, short, title, rows, kind = MODEL_CARDS[key]
    pins = model_pins(rows)
    note = ("Pin names are the Beckhoff terminal points in the order of the "
            "user's own drawing (left column 1 to 4, right column 5 to 8): "
            "%s. The channel is the signal in the I/O table, not the pin "
            "number. Check this against the datasheet of your terminal."
            % ", ".join("%d %s" % (p, l) for p, l, _, _ in sorted(pins)))
    infos = [("manufacturer", "Beckhoff"), ("manufacturer_reference", ref),
             ("designation", title),
             ("description", "EtherCAT Terminal, %s. %s" % (en, note))]
    e = Elmt(key, "%s %s" % (ref, en), "%s %s" % (ref, fr), infos, NOTE)
    only = CHANNELS_ONLY.get(key)
    for pin, label, _, _ in sorted(pins):
        if kind and not potential_of(label) and not label.startswith(
                ("END", "NC")) and (only is None or (label in only
                                                      and pin <= 8)):
            e.io(kind, label, FUNCTIONS.get(key, {}).get(label, ""),
                 pins=[str(pin)])
    n = len(rows)
    h = 180 + 20 * max(0, n - 4)
    e.rect(-50, -90, 100, h)
    e.text(-44, -86, "BECKHOFF", 5, True)
    e.text(-44, -72, ref, 9, True)
    e.text(-44, -56, short, 7)
    for pin, label, side, r in pins:
        y = -30 + 20 * r
        x = -50 if side == "w" else 50
        e.terminal(x, y, str(pin), side, potential_of(label))
        e.line(x, y, x + (10 if side == "w" else -10), y)
        text = "%d %s" % (pin, label) if side == "w" else "%s %d" % (label, pin)
        e.text(-38 if side == "w" else 38 - 4.2 * len(text), y - 8, text, 6)
    e.dyn(-50, -90 + h + 4, "label")
    return e


def table_of(src):
    """I/O table appearance of a card: same data, no drawing, no terminals."""
    e = Elmt(src.key + "_table", src.name_en + " - I/O table",
             src.name_fr + " - tableau d'E/S", list(src.infos), src.note)
    e.mode = "table"
    e.ios = list(src.ios)
    e.dyn(0, -24, "label", 9, True)
    e.dyn(60, -24, "designation", 7)
    e._b(0, -24, sum(TABLE_COLUMNS), TABLE_ROW * (len(e.ios) + 2))
    e.parts.append('<plc_table x="0" y="0" style="%s" antialias="false"/>'
                   % (STYLE % ("thin", "none")))
    return e


def epp2338():
    e = Elmt("epp2338", "EPP2338 EtherCAT P box, 8 channels",
             "EPP2338 boîtier EtherCAT P, 8 voies",
             [("manufacturer", "Beckhoff"), ("manufacturer_reference", "EPP2338-0001"),
              ("designation", "EtherCAT P Box, 8-channel digital combi, 24 V DC, 10 us, 0.5 A, M8"),
              ("description", "EtherCAT P box with four M8 sockets X01 to X04, two channels "
               "each, EtherCAT P IN X50 and OUT X51. Schematic view: one block per "
               "socket, pins named after the socket (X01.1 to X04.4, the pin order "
               "assumed for a 4-pin M8 socket: 1 +24 V, 2 channel 2, 3 0 V, "
               "4 channel 1; check it in the datasheet). The channel (X01(1)...) is "
               "the signal in the I/O table, not the pin. The label of the box is "
               "the card id (A1.10).")], NOTE)
    for i in range(4):
        port = "X%02d" % (i + 1)
        e.io("entree_universelle", "%s(1)" % port, pins=["%s.4" % port])
        e.io("entree_universelle", "%s(2)" % port, pins=["%s.2" % port])
    # dotted outline of the module, with the clips of the rail
    e.rect(-70, -120, 140, 220, dash="dotted")
    e.rect(-12, -128, 24, 8, "thin")
    e.rect(-12, 100, 24, 8, "thin")
    # EtherCAT P: IN above OUT, passing through the module
    for y, name, text in ((-100, "IN X50", "IN X50"), (-50, "OUT X51", "OUT X51")):
        e.terminal(-70, y, name, "w")
        e.circle(-70, y, 6)
        e.line(-67, y, -66, y)
        e.rect(-66, y - 7, 30, 14, "thin")
        e.text(-63, y - 5, text, 5)
    e.line(-51, -93, -51, -57, dash="dotted")
    # supply rails: 24 V and 0 V, shared by the four sockets
    e.line(-10, -100, -10, 80)
    e.line(-20, -100, -20, 80)
    e.text(-14, -112, "+24 V", 5)
    e.text(-34, -112, "0 V", 5)
    for i in range(4):
        y0 = -100 + 50 * i
        port = "X%02d" % (i + 1)
        e.text(2, y0 - 11, port, 6, True)
        for pin in range(1, 5):
            y = y0 + 10 * (pin - 1)
            e.terminal(70, y, "%s.%d" % (port, pin), "e")
            e.circle(70, y, 6)
            e.text(60, y - 9, str(pin), 5)
            if pin == 1:  # +24 V
                e.line(-10, y, 67, y)
                e.circle(-10, y, 2, "black")
            elif pin == 3:  # 0 V
                e.line(-20, y, 67, y)
                e.circle(-20, y, 2, "black")
            else:  # channel: LED and phototransistor of the stage
                e.rect(0, y - 6, 36, 12, "thin")
                e.circle(7, y, 5)
                e.circle(29, y, 5)
                e.line(11, y, 25, y, dash="dotted")
                e.line(36, y, 67, y)
    e.text(-66, -30, "EPP2338", 8, True)
    e.dyn(-66, -18, "label", 9, True)
    e.dyn(-66, 104, "designation", 6)
    return e


def epp2338_realistic():
    """Realistic view of the EPP2338: scale of the EK1914, grid of 5 px.

    The layout of the box (two columns of M8 sockets, EtherCAT P at the
    bottom) and the position of the pins in a socket are assumed; the
    drawing is original.
    """
    src = ALL["epp2338"]
    e = Elmt("epp2338_realistic", src.name_en + ", realistic view",
             src.name_fr + ", vue réaliste", list(src.infos),
             "Author: QElectroTech contributors\n"
             "Realistic view drawn by misc/beckhoff/gen_symbols.py, layout "
             "assumed: scale 1.00, grid 5 px. License: see "
             "http://qelectrotech.org/wiki/doc/elements_license")
    e.ios = list(src.ios)
    f = "Roboto Condensed"
    e.rect(-60, -105, 120, 210, "thin")
    e.rect(-55, -100, 110, 200, "thin")
    for x in (-50, 50):
        for y in (-92, 92):
            e.circle(x, y, 6)
    sockets = ((-30, -50, "X01"), (30, -50, "X02"), (-30, 10, "X03"),
               (30, 10, "X04"), (-30, 60, "IN X50"), (30, 60, "OUT X51"))
    for cx, cy, name in sockets:
        e.circle(cx, cy, 30)
        e.circle(cx, cy, 22)
        e.text(cx - len(name) * 2, cy + 16, name, 6, family=f)
        for dx in (-5, 5):  # state LEDs of the two channels
            e.circle(cx + dx * 2, cy - 22, 4)
        if name.startswith("X0"):
            for pin, (dx, dy) in enumerate(((-5, -5), (5, -5), (5, 5), (-5, 5)), 1):
                e.circle(cx + dx, cy + dy, 7)
                e.terminal(cx + dx, cy + dy, "%s.%d" % (name, pin), "e")
        else:
            e.circle(cx, cy, 8)
            e.terminal(cx, cy, name, "e")
    e.text(-40, 88, "EPP2338", 8, True, family=f)
    e.dyn(-40, -100, "label", 9, True)
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
    for key in MODEL_CARDS:
        out[key + ".elmt"] = model_card(key).xml()
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
        "epp2338_realistic.elmt": epp2338_realistic().xml(),
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
