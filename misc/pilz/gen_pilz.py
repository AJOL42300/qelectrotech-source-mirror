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
"""Generate the Pilz library (19_pilz): the PSEN safety switches, the
actuator and the connection cables of the order numbers 540003, 570480,
630311, C1000073, C1000074, 6M000007, 6M000008 and 6M000009.

Every product is ONE appearance (a simple element, no pin table). A device
has one terminal per connector pin that carries a signal, named with the
designation of the manufacturer (A1, A2, 12, 22, S31 ...). A cable has one
terminal per connector pin (named with the pin number) and one terminal per
wire end (named with the IEC 60757 colour code of the wire, for instance BN
or GYBN), so a wiring list shows the colour of every wire.
All the drawings are original vector drawings; only factual data (order
number, pin assignment, wire colours, terminal designations, ratings) comes
from public Pilz documents, listed in docs/pilz.md.

Usage: python misc/pilz/gen_pilz.py
"""
import os
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from qetlib import (Canvas, Terminal, write_element, write_directory,
                    text_size, BLACK)

REPO = os.path.dirname(os.path.dirname(os.path.dirname(
    os.path.abspath(__file__))))
EXTRA = os.path.join(REPO, "extra-elements")
PILZ_DIR = "19_pilz"
MANUFACTURER = "Pilz"
LIGHT = "lightgray"
AUTHOR = ("Author: QElectroTech contributors\nOriginal vector drawing, "
          "not copied from a manufacturer drawing. Order numbers, pin "
          "assignments, terminal designations and wire colours come from "
          "public Pilz documents (see docs/pilz.md); the status of each "
          "datum is given in the description.")

ROOT_NAMES = ("Pilz safety switches and cables",
              "Interrupteurs de s\u00e9curit\u00e9 et c\u00e2bles Pilz",
              "Pilz Sicherheitsschalter und Leitungen")
FAMILIES = {
    "psen_mgate": ("PSENmgate safety gate systems (M23, 19-pin)",
                   "Syst\u00e8mes de porte de s\u00e9curit\u00e9 PSENmgate "
                   "(M23, 19 broches)",
                   "Sicherheitszuhaltungssysteme PSENmgate (M23, 19-polig)"),
    "psen_cs": ("PSEN cs coded safety switches",
                "Interrupteurs de s\u00e9curit\u00e9 cod\u00e9s PSEN cs",
                "Codierte Sicherheitsschalter PSEN cs"),
    "psen_ml": ("PSEN ml actuators",
                "Actionneurs PSEN ml", "PSEN ml Betaetiger"),
    "cables": ("Connection cables with wire colours",
               "C\u00e2bles de raccordement avec couleurs des fils",
               "Anschlussleitungen mit Aderfarben"),
}

# IEC 60757 colour codes: name in English and French, QElectroTech fill.
CODES = {
    "BK": ("black", "noir", "black"),
    "BN": ("brown", "marron", "brun"),
    "RD": ("red", "rouge", "red"),
    "YE": ("yellow", "jaune", "yellow"),
    "GN": ("green", "vert", "green"),
    "BU": ("blue", "bleu", "blue"),
    "VT": ("violet", "violet", "purple"),
    "GY": ("grey", "gris", "gray"),
    "WH": ("white", "blanc", "white"),
    "PK": ("pink", "rose", "HTMLPinkPink"),
}


def split_code(code):
    return [code[i:i + 2] for i in range(0, len(code), 2)]


def colour_name(code, lang=0):
    return "-".join(CODES[c][lang] for c in split_code(code))


def swatch(c, x, y, w, code, h=4):
    """A piece of wire seen from the side, in the colour(s) of the code; a
    two-colour wire is striped. y is the centre line."""
    colours = [CODES[k][2] for k in split_code(code)]
    seg = 6 if len(colours) > 1 else w
    k, pos = 0, 0.0
    while pos < w:
        c.rect(x + pos, y - h / 2.0, min(seg, w - pos), h,
               colours[k % len(colours)])
        pos += seg
        k += 1


def term(name, x, y, orient):
    return Terminal(name, x, y, orient, "", "", "")


class Product:
    family = ""
    kind_en = kind_fr = ""
    ly = -40

    def __init__(self, order, file, short_en, short_fr, designation,
                 description):
        self.order, self.file = order, file
        self.short_en, self.short_fr = short_en, short_fr
        self.designation, self.description = designation, description
        self.terms = ()

    @property
    def key(self):
        return MANUFACTURER + "/" + self.order

    def path(self):
        return os.path.join(EXTRA, PILZ_DIR, self.family,
                            self.file + ".elmt")

    def infos(self):
        return {"manufacturer": MANUFACTURER,
                "manufacturer_reference": self.order,
                "designation": self.designation,
                "description": self.description}

    def names(self):
        return {"en": f"{MANUFACTURER} {self.order} {self.kind_en} "
                f"{self.short_en}",
                "fr": f"{self.kind_fr} {MANUFACTURER} {self.order} "
                f"{self.short_fr}"}

    def write(self):
        canvas = self.draw()
        write_element(self.path(), self.key, self.names(), self.infos(),
                      AUTHOR, canvas, self.terms, label_at=(0, self.ly),
                      master=False)


# ------------------------------------------------------------ M12 drawing
# Pin positions seen from the mating face of a male A-coded connector.
PIN_XY = {1: (-8, -8), 2: (8, -8), 3: (8, 8), 4: (-8, 8), 5: (0, 0)}
PIN_FILL = {1: "brun", 2: "white", 3: "blue", 4: "black", 5: "gray"}


def m12_connector(c, cx, cy, male):
    """An M12 connector seen from its mating face, the contacts coloured
    like the wires of the Pilz cable (1 BN, 2 WH, 3 BU, 4 BK, 5 GY)."""
    c.disc(cx, cy, 46, LIGHT)
    c.disc(cx, cy, 40, "white")
    c.disc(cx, cy, 38, LIGHT)
    c.rect(cx - 2.5, cy - 19, 5, 4, "gray" if male else "white")
    for n in range(1, 6):
        x, y = PIN_XY[n]
        if not male:
            x = {1: 8, 2: -8, 3: -8, 4: 8, 5: 0}[n]
        px, py = cx + x, cy + y
        c.disc(px, py, 7 if male else 8, PIN_FILL[n], BLACK, "normal")
        if not male:
            c.disc(px, py, 3, "white" if n == 4 else "black", BLACK)
        ty = py - 1.2 if y < 0 else py - 5.2 if y > 0 else py - 3.1
        if x < 0:
            c.text_r(px - 4.5, ty, str(n), 4)
        else:
            c.text(px + 4.5, ty, str(n), 4)


# --------------------------------------------------------------- devices
class Device(Product):
    """A device drawn as a box. rows: (terminal name, side, row, pin,
    colour code, function); the pin is None for an unused contact."""

    def terminal_row(self, c, name, side, y, pin, code, function, width):
        """Designation, wire swatch and the pin, colour and function of one
        terminal, on the left (w) or the right (e) side of the box."""
        tail = f"pin {pin} {code}  {function}"
        if side == "w":
            c.text(14, y - 4.5, name, 6, True)
            swatch(c, 14 + text_size(name, 6, True)[0] + 3, y, 12, code)
            c.text(14 + text_size(name, 6, True)[0] + 18, y - 3, tail, 4)
        else:
            x = width - 4
            c.text_r(x, y - 4.5, name, 6, True)
            nx = x - text_size(name, 6, True)[0]
            swatch(c, nx - 15, y, 12, code)
            c.text_r(nx - 18, y - 3, tail, 4)


class MGate(Device):
    family = "psen_mgate"
    kind_en = "PSENmgate safety gate system"
    kind_fr = "Syst\u00e8me de porte de s\u00e9curit\u00e9 PSENmgate"
    ly = -52
    WIDTH = 300
    # (terminal, side, pin, wire colour, function)
    ROWS = (
        ("A1", "w", 19, "BN", "+24 V DC supply UB"),
        ("A2", "w", 6, "BU", "0 V supply UB"),
        ("Y1", "w", 1, "RD", "Diagnostic input"),
        ("S31", "w", 18, "PK", "Solenoid operation, channel 1"),
        ("S41", "w", 7, "WH", "Solenoid operation, channel 2"),
        ("B1", "w", 12, "GNYE", "Pushbutton 1"),
        ("B2", "w", 10, "BNGN", "Pushbutton 2"),
        ("LED1", "w", 15, "WHGN", "LED of pushbutton 1"),
        ("LED2", "w", 13, "BK", "LED of pushbutton 2"),
        ("12", "e", 5, "GN", "Safety output, channel 1"),
        ("22", "e", 4, "YE", "Safety output, channel 2"),
        ("Y32", "e", 2, "GY", "Signal / diagnostic output"),
        ("ES_SO", "e", 3, "VT", "E-STOP signal output"),
        ("ES1.1", "e", 9, "GYBN", "E-STOP, channel 1"),
        ("ES1.2", "e", 11, "YEBN", "E-STOP, channel 1"),
        ("ES2.1", "e", 14, "WHYE", "E-STOP, channel 2"),
        ("ES2.2", "e", 16, "WHGY", "E-STOP, channel 2"),
    )
    RESERVED = ((8, "RDBU"), (17, "GYPK"))

    def __init__(self, order, file, entry_en, entry_fr, entry_short):
        super().__init__(
            order, file, f"{entry_short}, 2 LED pushbuttons + E-STOP",
            f"{entry_short}, 2 boutons lumineux + arr\u00eat d'urgence",
            f"Syst\u00e8me de porte de s\u00e9curit\u00e9 PSENmgate, 2 "
            f"boutons lumineux (1 NO) et 1 arr\u00eat d'urgence "
            f"(2 NC + 1 NO), cod\u00e9, rearmement automatique, "
            f"{entry_fr}, connecteur M23 m\u00e2le 19 broches, sortie "
            "de c\u00e2ble vers le bas, connexion simple",
            "PSENmgate safety gate system (PSEN mg1-A-LLFXX, single "
            "connection), 2 illuminated pushbuttons (1 NO each) and 1 "
            f"E-STOP (2 NC + 1 NO), coded, automatic reset, {entry_en}, "
            "19-pin M23 male connector, cable outlet downwards, 24 V DC, "
            "2 safety outputs (12, 22) and 1 signal output, actuator PSEN "
            "ml 1.1 (570480). VERIFIED (Pilz e-shop page of the order "
            "number and operating manual 1006672, pin assignment table): "
            "pin 1 Y1 red, 2 Y32 grey, 3 ES_SO violet, 4 terminal 22 "
            "yellow, 5 terminal 12 green, 6 A2 blue, 7 S41 white, 8 "
            "reserved red-blue, 9 ES1.1 grey-brown, 10 B2 brown-green, 11 "
            "ES1.2 yellow-brown, 12 B1 green-yellow, 13 LED2 black, 14 "
            "ES2.1 white-yellow, 15 LED1 white-green, 16 ES2.2 white-grey, "
            "17 reserved grey-pink, 18 S31 pink, 19 A1 brown. The colours "
            "are those of the Pilz accessory cable only. ASSUMED: the "
            "three order numbers 6M000007, 6M000008 and 6M000009 share the "
            "same wiring (the manual gives one table for the series); the "
            "reserved pins 8 and 17 have no terminal.")
        self.entry_en = entry_en
        self.terms = tuple(
            term(name, 0 if side == "w" else self.WIDTH + 20,
                 10 * [r for r in self.ROWS if r[1] == side].index(
                     (name, side, pin, code, fn)),
                 side)
            for name, side, pin, code, fn in self.ROWS)

    def draw(self):
        c = Canvas()
        w = self.WIDTH
        left = [r for r in self.ROWS if r[1] == "w"]
        right = [r for r in self.ROWS if r[1] == "e"]
        bottom = 10 * (len(left) - 1) + 28
        c.rect(10, -42, w, bottom + 42, "white", BLACK, "normal")
        c.line(10, -10, 10 + w, -10)
        c.line(10 + w / 2, -10, 10 + w / 2, bottom)
        c.text_c(10 + w / 2, -39, "PSEN mg1-A-LLFXX", 7, True)
        c.text_c(10 + w / 2, -28, f"{self.order}  -  {self.entry_en}", 5)
        c.text_c(10 + w / 2, -19, "M23 male 19-pin, 24 V DC", 4)
        for side, group in (("w", left), ("e", right)):
            for i, (name, s, pin, code, fn) in enumerate(group):
                self.terminal_row(c, name, side, 10 * i, pin, code, fn,
                                  10 + w if side == "e" else 0)
                end = (10, 0) if side == "w" else (10 + w, w + 20)
                c.line(end[0], 10 * i, end[1], 10 * i)
        res = ", ".join(f"pin {p} {k}" for p, k in self.RESERVED)
        c.text(14, bottom - 18, f"Reserved, no terminal: {res}", 4)
        c.text(14, bottom - 9, "Wire colours: Pilz accessory cable", 4)
        c.text_r(w + 6, bottom - 9, "VERIFIED pin-out", 4)
        return c


class CodedSwitch(Device):
    family = "psen_cs"
    kind_en = "PSEN cs1.1n coded safety switch"
    kind_fr = "Interrupteur de s\u00e9curit\u00e9 cod\u00e9 PSEN cs1.1n"
    WIDTH = 180
    ROWS = (
        ("A1", "w", 1, "BN", "+24 V DC supply UB"),
        ("A2", "w", 3, "BU", "0 V supply UB"),
        ("12", "e", 2, "WH", "Safety output 1"),
        ("22", "e", 4, "BK", "Safety output 2"),
    )

    def __init__(self):
        super().__init__(
            "540003", "psen_cs1_1n_540003",
            "with 5-pin M12 male connector",
            "avec connecteur M12 m\u00e2le 5 broches",
            "Interrupteur de s\u00e9curit\u00e9 cod\u00e9 RFID PSEN cs1.1n, "
            "2 sorties de s\u00e9curit\u00e9 \u00e0 semi-conducteurs, "
            "connecteur M12 m\u00e2le 5 broches, 24 V DC",
            "PSEN cs1.1n / PSEN cs1.1 coded RFID safety switch with "
            "actuator, 2 safe semiconductor outputs, 5-pin M12 male "
            "connector, 24 V DC, IP67. VERIFIED (Pilz e-shop page of 540003 "
            "and the Pilz operating manual of the PSEN cs1.1n): pin 1 A1 "
            "+24 V UB brown, pin 2 terminal 12 output channel 1 white, pin "
            "3 A2 0 V UB blue, pin 4 terminal 22 output channel 2 black, "
            "pin 5 grey do not connect (no terminal). The colours are those "
            "of the Pilz accessory cable.")
        self.terms = (term("A1", 0, 0, "w"), term("A2", 0, 30, "w"),
                      term("12", self.WIDTH + 20, 0, "e"),
                      term("22", self.WIDTH + 20, 30, "e"))

    def draw(self):
        c = Canvas()
        w = self.WIDTH
        c.rect(10, -28, w, 148, "white", BLACK, "normal")
        c.text_c(10 + w / 2, -25, "PSEN cs1.1n", 7, True)
        for name, side, pin, code, fn in self.ROWS:
            y = 0 if name in ("A1", "12") else 30
            self.terminal_row_two(c, name, side, y, pin, code, fn, w + 6)
            if side == "w":
                c.line(10, y, 0, y)
            else:
                c.line(10 + w, y, w + 20, y)
        m12_connector(c, 10 + w / 2, 86, True)
        c.text_c(10 + w / 2, 112, "M12 male 5-pin", 4)
        c.text(14, 82, "pin 5 GY: do not connect", 4)
        return c

    def terminal_row_two(self, c, name, side, y, pin, code, fn, right):
        tail = f"pin {pin} {code}"
        if side == "w":
            c.text(14, y - 4.5, name, 6, True)
            swatch(c, 14 + text_size(name, 6, True)[0] + 3, y, 12, code)
            c.text(14, y + 6, tail, 4)
            c.text(14, y + 14, fn, 4)
        else:
            c.text_r(right, y - 4.5, name, 6, True)
            nx = right - text_size(name, 6, True)[0]
            swatch(c, nx - 15, y, 12, code)
            c.text_r(right, y + 6, tail, 4)
            c.text_r(right, y + 14, fn, 4)


class Actuator(Product):
    family = "psen_ml"
    kind_en = "PSEN ml 1.1 actuator"
    kind_fr = "Actionneur PSEN ml 1.1"
    ly = -40

    def __init__(self):
        super().__init__(
            "570480", "psen_ml_1_1_570480", "for PSENmlock", 
            "pour PSENmlock",
            "Actionneur cod\u00e9 PSEN ml 1.1 pour interrupteur de "
            "s\u00e9curit\u00e9 \u00e0 verrouillage PSENmlock",
            "PSEN ml 1.1 coded actuator for the PSENmlock safety guard "
            "locking devices and the PSENmgate systems (the Type 1 "
            "actuator of 6M000007 to 6M000009). VERIFIED (Pilz e-shop "
            "page of 570480): passive actuator, no electrical "
            "connection. ASSUMED: the single terminal MECH is only a "
            "marker for the mechanical coupling to the switch, it carries "
            "no signal.")
        self.terms = (term("MECH", 0, 0, "w"),)

    def draw(self):
        c = Canvas()
        c.line(0, 0, 20, 0)
        c.rect(20, -6, 24, 12, "gray")
        c.rect(44, -20, 56, 40, LIGHT, BLACK, "normal")
        c.disc(55, -12, 5, "white")
        c.disc(55, 12, 5, "white")
        c.disc(89, -12, 5, "white")
        c.disc(89, 12, 5, "white")
        c.text_c(72, -10, "PSEN ml", 5, True)
        c.text_c(72, -1, "1.1", 5, True)
        c.text_c(72, 28, "actuator, mechanical", 4)
        c.text_c(72, 35, "no electrical contact", 4)
        c.text_c(72, 42, "570480", 5)
        return c


# ---------------------------------------------------------------- cables
class Cable(Product):
    """A cable with a connector (X1, pins) at one end and open wires at the
    other: one terminal per pin on the left, one per wire end (named with
    its colour code) on the right."""
    family = "cables"
    kind_en = "connection cable"
    kind_fr = "C\u00e2ble de raccordement"
    WIRE_X0, WIRE_X1 = 84, 130

    def __init__(self, order, file, connector, length, wires, short,
                 designation, description):
        super().__init__(order, file, short, short, designation,
                         description)
        self.connector, self.length, self.wires = connector, length, wires
        self.ly = -52
        self.terms = tuple(
            [term(str(pin), 0, 10 * i, "w")
             for i, (pin, code) in enumerate(wires)]
            + [term(code, 200, 10 * i, "e")
               for i, (pin, code) in enumerate(wires)])

    def draw(self):
        c = Canvas()
        rows = len(self.wires)
        bottom = 10 * (rows - 1)
        c.rect(10, -5, 36, bottom + 10, LIGHT, BLACK, "normal")
        c.rect(46, -5, 38, bottom + 10, "yellow", BLACK, "normal")
        c.text_c(28, -38, self.connector, 4)
        c.text_c(65, -30, self.length, 5, True)
        c.text(110, -30, "open end", 4)
        for i, (pin, code) in enumerate(self.wires):
            y = 10 * i
            c.line(0, y, 10, y)
            c.text_c(28, y - 3.5, str(pin), 5)
            swatch(c, self.WIRE_X0, y, self.WIRE_X1 - self.WIRE_X0, code)
            c.text(self.WIRE_X1 + 4, y - 4, code, 5, True)
            c.text(self.WIRE_X1 + 24, y - 3, colour_name(code), 4)
            c.line(190, y, 200, y)
        c.text(10, bottom + 12, self.order, 5)
        return c


def cable_infos(wires):
    return ", ".join(f"pin {p} {k}" for p, k in wires)


M12_WIRES = ((1, "BN"), (2, "WH"), (3, "BU"), (4, "BK"), (5, "GY"))
M23_WIRES = ((1, "RD"), (2, "GY"), (3, "VT"), (4, "YE"), (5, "GN"),
             (6, "BU"), (7, "WH"), (8, "RDBU"), (9, "GYBN"), (10, "BNGN"),
             (11, "YEBN"), (12, "GNYE"), (13, "BK"), (14, "WHYE"),
             (15, "WHGN"), (16, "WHGY"), (17, "GYPK"), (18, "PK"),
             (19, "BN"))


def cables():
    out = [Cable(
        "630311", "psen_op_cable_m12_5p_5m", "M12 female 5-pin A-coded",
        "5 m", M12_WIRES, "PSEN op cable axial M12 5-pole, 5 m",
        "C\u00e2ble de raccordement PSEN op axial M12 5 p\u00f4les, 5 m",
        "PSEN op cable axial M12 5-pole, 5 m: PUR, yellow (similar to RAL "
        "1003), 5-pin M12 female connector straight (A-coded) on an open "
        "cable end, 5 x 0.25 mm2 (AWG 24), unshielded, 5.1 mm. VERIFIED "
        "(Pilz data sheet 1002482 of the PSEN op cable M12 5-pin and the "
        "data sheet of 630311): the wire colours are brown, white, blue, "
        "black, grey. ASSUMED: the colour of each pin, " +
        cable_infos(M12_WIRES) + ", is the order of the A-coded M12 "
        "standard (EN 61076-2-101), the same as the PSEN cs1.1n "
        "operating manual; it is not printed in the data sheet. The "
        "terminals 1 to 5 are the connector pins, BN WH BU BK GY the wire "
        "ends.")]
    for order, length in (("C1000073", 5), ("C1000074", 10)):
        out.append(Cable(
            order, f"cable_m23_19p_{length}m", "M23 female 19-pin",
            f"{length} m", M23_WIRES, f"M23 19-pin cable, {length} m",
            "C\u00e2ble de raccordement M23 19 broches, 19 x 0,25 mm2, "
            f"{length} m",
            f"Connection cable, 19-pin M23 female connector straight, "
            f"19 x 0.25 mm2 (AWG 24), unshielded, PUR, yellow (RAL "
            f"1003), 8.05 mm, 2 A, {length} m. VERIFIED (Pilz e-shop page "
            "and data sheet of the order number): connector, conductors, "
            "section and length. ASSUMED: the wire colours, " +
            cable_infos(M23_WIRES) + ", are the 'Cable colour (Pilz "
            "cable)' column of the PSEN mg1-A-LLFXX operating manual "
            "1006672 (verified there), applied here to this Pilz cable; "
            "the end opposite to the connector is taken as open wires "
            "(the data sheet lists one connector only). The terminals 1 to "
            "19 are the connector pins, the codes the wire ends."))
    return out


def devices():
    return [
        MGate("6M000007", "psen_mgate_6M000007", "entry from left",
              "entr\u00e9e par la gauche", "entry from left"),
        MGate("6M000008", "psen_mgate_6M000008", "entry from right",
              "entr\u00e9e par la droite", "entry from right"),
        MGate("6M000009", "psen_mgate_6M000009", "front entry",
              "entr\u00e9e frontale", "front entry"),
        CodedSwitch(),
        Actuator(),
    ]


def products():
    return devices() + cables()


def main():
    root = os.path.join(EXTRA, PILZ_DIR)
    shutil.rmtree(root, ignore_errors=True)
    write_directory(root, dict(zip(("en", "fr", "de"), ROOT_NAMES)))
    for fam, n in FAMILIES.items():
        write_directory(os.path.join(root, fam),
                        dict(zip(("en", "fr", "de"), n)))
    items = products()
    bad = 0
    for p in items:
        p.write()
        for problem in p.draw().layout_problems():
            print(f"drawing problem in {p.file}: {problem}")
            bad += 1
    print(f"{len(items)} Pilz products written, {bad} drawing problem(s)")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
