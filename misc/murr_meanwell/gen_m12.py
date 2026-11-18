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
"""Generate the Murrelektronik M12 library (18_murrelektronik_m12).

Moulded cables, Y distributors, free M12 connectors and a T coupler, each
product being ONE appearance (a simple element, no pin table) with
variants by order number and length. An M12 connector is drawn as a circle
with its numbered pins (male: solid pins, female: hollow sockets, seen from
the mating face) and has ONE terminal, the way the M12 sockets of the
Beckhoff EPP boxes do. The wires inside a cable are not drawn.
All the drawings are original vector drawings; only factual data (order
number, poles, coding, length, cable type) comes from public datasheets.

Usage: python misc/murr_meanwell/gen_m12.py
"""
import json
import math
import os
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from qetlib import (Canvas, Terminal, write_element, write_directory,
                    GREEN, BLACK)
from gen_elements import EXTRA, AUTHOR

M12_DIR = "18_murrelektronik_m12"
MANUFACTURER = "Murrelektronik"
LIGHT = "lightgray"

ROOT_NAMES = ("Murrelektronik M12 cables, connectors and distributors",
              "C\u00e2bles, connecteurs et r\u00e9partiteurs M12 "
              "Murrelektronik",
              "Murrelektronik M12 Leitungen, Steckverbinder und Verteiler")
FAMILIES = {
    "m12_cable": ("M12 moulded cables (A-coded, PUR)",
                  "C\u00e2bles moul\u00e9s M12 (cod\u00e9 A, PUR)",
                  "M12 Anschlussleitungen (A-codiert, PUR)"),
    "m12_y_distributor": ("M12 Y distributors for valves",
                          "R\u00e9partiteurs Y M12 pour \u00e9lectrovannes",
                          "M12 Y-Verteiler f\u00fcr Ventile"),
    "m12_connector": ("M12 field-wireable connectors (IDC)",
                      "Connecteurs M12 \u00e0 c\u00e2bler (bornes "
                      "guillotines)",
                      "M12 Steckverbinder konfektionierbar (Schneidklemm)"),
    "m12_t_coupler": ("M12 T couplers",
                      "Coupleurs T M12", "M12 T-Koppler"),
    "m12_valve_coil": ("Solenoid valve coils",
                       "Bobines d'\u00e9lectrovanne",
                       "Magnetventilspulen"),
    "m12_valve_plug": ("Valve plugs form BI with LED and suppression",
                       "Connecteurs d'\u00e9lectrovanne forme BI avec LED "
                       "et protection",
                       "Ventilstecker Bauform BI mit LED und "
                       "Schutzbeschaltung"),
}

# Pin positions seen from the mating face of a male A-coded connector;
# a female connector seen from its face is the mirror image.
PIN_XY = {1: (-8, -8), 2: (8, -8), 3: (8, 8), 4: (-8, 8), 5: (0, 0)}
RADIUS = 23
# Contact colours of the A-coded M12 (1 brown, 2 white, 3 blue, 4 black,
# 5 grey); the white contact is outlined in black to stay visible.
PIN_COLOURS = {1: "brun", 2: "white", 3: "blue", 4: "black", 5: "gray"}


def pin_xy(n, male):
    x, y = PIN_XY[n]
    if not male:
        x = {1: 8, 2: -8, 3: -8, 4: 8, 5: 0}[n]
    return x, y


def knurl(c, cx, cy, ticks=24):
    """The knurled coupling nut: short radial ribs on its rim."""
    for k in range(ticks):
        a = 2 * math.pi * k / ticks
        c.line(cx + 20.5 * math.cos(a), cy + 20.5 * math.sin(a),
               cx + 23 * math.cos(a), cy + 23 * math.sin(a), "gray")


def connector(c, cx, cy, male, poles):
    """An M12 connector seen from its mating face: knurled coupling nut,
    thread, insulator with the coding key of the A coding at the top, and
    the numbered contacts (male: solid pins, female: hollow sockets)."""
    c.disc(cx, cy, 2 * RADIUS, LIGHT)
    knurl(c, cx, cy)
    c.disc(cx, cy, 40, "white")
    c.disc(cx, cy, 38, LIGHT)
    if male:
        c.rect(cx - 2.5, cy - 19, 5, 4, "gray")
    else:
        c.rect(cx - 2.5, cy - 19, 5, 4, "white")
    for n in range(1, poles + 1):
        x, y = pin_xy(n, male)
        px, py = cx + x, cy + y
        colour = PIN_COLOURS[n]
        if male:
            c.disc(px, py, 7, colour, BLACK, "normal")
        else:
            # a socket: a ring filled with the colour of the contact, with
            # a dark (light for the black contact) hole in the middle
            c.disc(px, py, 8, colour, BLACK, "normal")
            c.disc(px, py, 3, "white" if colour == "black" else "black",
                   BLACK)
        ty = py - 1.2 if y < 0 else py - 5.2 if y > 0 else py - 3.1
        if x < 0:
            c.text_r(px - 4.5, ty, str(n), 4)
        else:
            c.text(px + 4.5, ty, str(n), 4)


EV_COLOURS = {"1": "red", "2": "blue", "PE": "orange"}


def blade(c, x, y, w, h, colour, male, hole=False):
    """A flat blade contact (flat tab with rounded corners): solid colour
    for a pin of the coil side, white with a coloured outline for a socket
    slot; the PE blade has a round hole."""
    r = 1.2
    pts = [(x + r, y), (x + w - r, y), (x + w, y + r), (x + w, y + h - r),
           (x + w - r, y + h), (x + r, y + h), (x, y + h - r), (x, y + r)]
    c.poly(pts, True, colour if male else "white", colour)
    if hole:
        c.disc(x + w / 2, y + h / 2, 2.4, "white", colour)


def valve_connector(c, cx, cy, male=False, led=False):
    """A valve connector form BI: two flat blades side by side for contacts
    1 and 2 and a lower, wider flat blade with a hole for PE (assumed).
    Contact 1 is red, 2 blue and PE orange, as the colours of the library.
    With led=True the yellow status LED of the plug is drawn between the
    contacts 1 and 2."""
    c.rect(cx - 22, cy - 15, 44, 30, LIGHT)
    if led:
        c.disc(cx, cy - 6, 5, "yellow")
    blade(c, cx - 12, cy - 12, 4, 12, EV_COLOURS["1"], male)
    blade(c, cx + 8, cy - 12, 4, 12, EV_COLOURS["2"], male)
    blade(c, cx - 8, cy + 3, 16, 7, EV_COLOURS["PE"], male, True)
    c.text(cx - 19, cy - 8, "1", 4)
    c.text(cx + 15, cy - 8, "2", 4)
    c.text(cx + 11, cy + 4.5, "PE", 4)


def cable_line(c, x1, y1, x2, y2):
    c.line(x1, y1, x2, y2, BLACK, "hight")


def boot(c, x, y, w):
    c.rect(x, y - 8, w, 16, LIGHT)


def overmould(c, x, y, direction):
    """The moulded strain relief at the end of a cable, tapering from the
    connector (x) to the cable; direction is 1 towards +x, -1 towards -x."""
    pts = [(x, y - 11), (x + direction * 20, y - 11),
           (x + direction * 28, y - 3), (x + direction * 28, y + 3),
           (x + direction * 20, y + 11), (x, y + 11)]
    c.poly(pts, True, LIGHT)
    for k in (6, 12):
        c.line(x + direction * k, y - 11, x + direction * k, y + 11, "gray")


class Product:
    kind_en = kind_fr = ""

    def __init__(self, family, order, file, short, designation,
                 description, poles=4):
        self.family, self.order, self.file = family, order, file
        self.short, self.designation = short, designation
        self.description, self.poles = description, poles
        self.terms = ()

    @property
    def key(self):
        return MANUFACTURER + "/" + self.order

    def path(self):
        return os.path.join(EXTRA, M12_DIR, self.family, self.file + ".elmt")

    def infos(self):
        return {"manufacturer": MANUFACTURER,
                "manufacturer_reference": self.order,
                "designation": self.designation,
                "description": self.description}

    def names(self):
        return {"en": f"{MANUFACTURER} {self.order} {self.kind_en} "
                f"{self.short}",
                "fr": f"{self.kind_fr} {MANUFACTURER} {self.order} "
                f"{self.short_fr}"}

    def kinds(self):
        """Kind informations the function following reads: the routes
        of a symbol that passes the channels through (ROUTES), the
        channels of a symbol that keeps a function (ENDS)."""
        out = []
        if self.family in ROUTES:
            out.append(("function_routes", ";".join(
                f"{a}:{ca}>{b}:{cb}"
                for a, ca, b, cb in ROUTES[self.family])))
        if self.family in ENDS:
            out.append(("function_ends", ";".join(
                f"{t}:{c}" for t, c in ENDS[self.family])))
        return out

    def write(self):
        canvas = self.draw()
        write_element(self.path(), self.key, self.names(), self.infos(),
                      AUTHOR, canvas, self.terms, label_at=(0, self.ly),
                      master=False,
                      extra_texts=getattr(self, "extra_texts", ()),
                      kind_informations=self.kinds())


def term(name, x, y, orient):
    return Terminal(name, x, y, orient, "", "", "")


def metres(length):
    return f"{length:g}".replace(".", ",")


class Cable(Product):
    kind_en, kind_fr = "M12 moulded cable", "C\u00e2ble moul\u00e9 M12"
    ly = -40

    def __init__(self, order, length):
        self.length = length
        self.short = f"{metres(length)} m"
        self.short_fr = self.short
        super().__init__(
            "m12_cable", order,
            "m12_cable_" + metres(length).replace(",", "_") + "m",
            self.short,
            "C\u00e2ble moul\u00e9 M12 m\u00e2le 0\u00b0 / M12 femelle 0\u00b0 "
            f"cod\u00e9 A, PUR 4x0,34, {metres(length)} m",
            "M12 male 0 deg / M12 female 0 deg, A-coded, PUR 4 x 0.34 mm2, "
            f"{metres(length)} m. The female connector has 5 contacts (the "
            "contact 5 is present but not wired) and the cable has 4 "
            "conductors: 1 brown, 2 white, 3 blue, 4 black on the pins 1 to "
            "4 of both ends (the free contact 5 is the information of the "
            "user, not read in the datasheet). Function routing: X1 and X2 "
            "are the same channels (1 = pin 4, 2 = pin 2).", 4)
        self.terms = (term("X1", 0, 0, "w"), term("X2", 200, 0, "e"))

    def draw(self):
        c = Canvas()
        c.line(0, 0, 10, 0)
        c.line(190, 0, 200, 0)
        connector(c, 33, 0, True, 4)
        connector(c, 167, 0, False, 5)
        overmould(c, 56, 0, 1)
        overmould(c, 144, 0, -1)
        c.rect(84, -3, 32, 6, "gray")
        c.text_c(100, -20, f"{metres(self.length)} m", 6, True)
        c.text_c(100, 14, self.order, 5)
        c.text_c(100, 23, "PUR 4x0,34", 4)
        c.text_c(33, 26, "male", 4)
        c.text_c(167, 26, "female", 4)
        return c


class YDistributor(Product):
    kind_en = "M12 Y distributor"
    kind_fr = "R\u00e9partiteur Y M12"
    ly = -60

    def __init__(self, order, length):
        self.short = f"{metres(length)} m"
        self.short_fr = self.short
        super().__init__(
            "m12_y_distributor", order,
            "m12_y_" + metres(length).replace(",", "_") + "m", self.short,
            "R\u00e9partiteur Y M12 m\u00e2le / 2 connecteurs pour "
            f"\u00e9lectrovanne forme BI 18 mm, {metres(length)} m",
            "Y distributor, M12 male A-coded to two valve connectors "
            f"form BI, {metres(length)} m. Wiring from the diagram of the "
            "datasheet supplied by the user: the M12 pins 2, 3, 4 and 5 are "
            "wired (pin 1 free), pin 4 to the contact 1 of valve 1 (X2), "
            "pin 2 to the contact 1 of valve 2 (X3), pin 3 to the contact 2 "
            "and pin 5 to PE of both valves; each valve connector has a "
            "yellow LED with a resistor and two suppression diodes in "
            "anti-series between the contacts 1 and 2. The datasheet of the "
            "42421 itself was not found (the connector form is the one of "
            "the order designation). Function routing: channel 1 (pin 4) "
            "to X2, channel 2 (pin 2) to X3.", 5)
        self.length = length
        self.terms = (term("X1", 0, 0, "w"), term("X2", 200, -30, "e"),
                      term("X3", 200, 30, "e"))

    def draw(self):
        c = Canvas()
        c.line(0, 0, 10, 0)
        c.line(190, -30, 200, -30)
        c.line(190, 30, 200, 30)
        connector(c, 33, 0, True, 5)
        overmould(c, 56, 0, 1)
        cable_line(c, 84, 0, 98, 0)
        c.poly([(96, -6), (106, -6), (120, -14), (120, 14), (106, 6),
                (96, 6)], True, LIGHT)
        for sign in (-1, 1):
            cable_line(c, 120, 14 * sign, 134, 30 * sign)
            cable_line(c, 134, 30 * sign, 146, 30 * sign)
            valve_connector(c, 168, 30 * sign, False, True)
        c.text_c(33, 26, "male", 4)
        c.text_c(100, 14, f"{metres(self.length)} m", 6, True)
        c.text_c(90, 26, self.order, 5)
        return c


class FreeConnector(Product):
    kind_en = "M12 field-wireable connector"
    kind_fr = "Connecteur M12 \u00e0 c\u00e2bler"
    ly = -40

    def __init__(self, order, male):
        self.male = male
        self.short = "male 0\u00b0 A-coded IDC" if male \
            else "female 0\u00b0 A-coded IDC"
        self.short_fr = "m\u00e2le 0\u00b0 cod\u00e9 A, bornes guillotines" \
            if male else "femelle 0\u00b0 cod\u00e9 A, bornes guillotines"
        gender_fr = "m\u00e2le" if male else "femelle"
        super().__init__(
            "m12_connector", order,
            "m12_connector_" + ("male" if male else "female"), self.short,
            f"Connecteur M12 {gender_fr} 0\u00b0 cod\u00e9 A, 4 p\u00f4les, "
            "bornes guillotines (IDC)",
            f"M12 {'male' if male else 'female'} 0 deg, A-coded, 4 poles, "
            "IDC terminals, field-wireable.", 4)
        self.terms = (term("X1", 0, 0, "w"),)

    def draw(self):
        c = Canvas()
        c.line(0, 0, 10, 0)
        connector(c, 33, 0, self.male, 4)
        # side view: coupling nut with knurl, body, screw-down cap
        c.rect(70, -12, 14, 24, LIGHT)
        for k in range(1, 4):
            c.line(70 + 3.5 * k, -12, 70 + 3.5 * k, 12, "gray")
        c.rect(84, -9, 34, 18, LIGHT)
        c.rect(118, -7, 12, 14, "gray")
        c.text_c(100, 12, "IDC", 5)
        c.text_c(33, 26, "male" if self.male else "female", 4)
        c.text_c(80, 30, self.order, 5)
        return c


class TCoupler(Product):
    kind_en = "M12 T coupler"
    kind_fr = "Coupleur T M12"
    ly = -70

    def __init__(self):
        self.short = "male / 2 x female A-coded, LED"
        self.short_fr = "m\u00e2le / 2 x femelle cod\u00e9 A, LED"
        super().__init__(
            "m12_t_coupler", "7000-41135-0000000", "m12_t_coupler",
            self.short,
            "Coupleur T M12 m\u00e2le (5 p\u00f4les) / 2 x M12 femelle "
            "(4 contacts c\u00e2bl\u00e9s) cod\u00e9 A, 3 LED",
            "T coupler, M12 male base 5 poles A-coded to two M12 female "
            "sockets A-coded, 3 LEDs with resistors. Housing about 32 x 45 "
            "mm, M12x1, sockets 16.4 mm apart (dimensions read on the "
            "drawing supplied by the user). Internal wiring from the "
            "diagram supplied by the user (the order number itself is "
            "still to be confirmed on the Murrelektronik site): base pin 1 "
            "to the contacts 1 of both sockets, pin 2 to the contact 4 of "
            "socket 2 (X3) only, pin 4 to the contact 4 of socket 1 (X2) "
            "only, pin 3 to the contacts 3 and pin 5 to the contacts 5 of "
            "both; the contact 2 of the sockets is not wired. LEDs, all "
            "cathodes on pin 3: GN between pins 1 and 3, WH between pins "
            "2 and 3, YE between pins 4 and 3. Function routing: channel 1 "
            "(pin 4) to X2, channel 2 (pin 2) to X3.", 5)
        self.terms = (term("X1", 0, 0, "w"), term("X2", 200, -30, "e"),
                      term("X3", 200, 30, "e"))

    def led_branch(self, c, x, y, fill, name):
        """Resistor and LED in series from the line y down to the line of
        the pin 3 (y 24), anode on top."""
        c.line(x, y, x, 3)
        c.rect(x - 3, 3, 6, 8)
        c.line(x, 11, x, 14)
        c.poly([(x - 4, 14), (x + 4, 14), (x, 21)], True, fill)
        c.line(x - 4, 21, x + 4, 21)
        c.line(x, 21, x, 24)
        c.disc(x, y, 2.5, "black")
        c.disc(x, 24, 2.5, "black")
        c.text(x + 3.5, 12, name, 4)

    def draw(self):
        c = Canvas()
        c.line(0, 0, 10, 0)
        c.line(190, -30, 200, -30)
        c.line(190, 30, 200, 30)
        connector(c, 33, 0, True, 5)
        connector(c, 167, -30, False, 5)
        connector(c, 167, 30, False, 5)
        # T shaped housing: the stem of the male base (holds the pin lines
        # and the LEDs) joins the bar that carries the two sockets
        c.poly([(58, -30), (100, -30), (100, -57), (142, -57), (142, 58),
                (100, 58), (100, 30), (58, 30)], True, "white", BLACK,
               "normal")
        c.rect(55, -14, 4, 28, "lightgray")
        # the lines of the base pins, from top to bottom 4, 1, 2, 5, 3
        lines = ((4, -24, "black"), (1, -16, "brun"), (2, -8, "black"),
                 (5, 0, "gray"), (3, 24, "blue"))
        for pin, y, colour in lines:
            c.text(60.5, y - 2, str(pin), 4)
        c.line(66, -24, 108, -24, "black")
        c.line(108, -24, 108, -38, "black")
        c.line(108, -38, 134, -38, "black")
        c.line(66, -16, 112, -16, "brun")
        c.line(66, -8, 108, -8, "black")
        c.line(108, -8, 108, 45, "black")
        c.line(108, 45, 134, 45, "black")
        c.line(66, 0, 128, 0, "gray")
        c.line(66, 24, 120, 24, "blue")
        # buses of the contacts 1, 3 and 5, common to both sockets
        c.line(112, -52, 112, 31, "brun")
        c.line(120, -45, 120, 38, "blue")
        c.line(128, -31, 128, 52, "gray")
        for x, y in ((112, -16), (128, 0), (120, 24)):
            c.disc(x, y, 3, "black")
        for y, x0 in ((-52, 112), (-45, 120), (-31, 128), (31, 112),
                      (38, 120), (52, 128)):
            colour = {112: "brun", 120: "blue", 128: "gray"}[x0]
            c.line(x0, y, 134, y, colour)
        for y, name in ((-52, "1"), (-45, "3"), (-38, "4"), (-31, "5"),
                        (31, "1"), (38, "3"), (45, "4"), (52, "5")):
            c.text(136, y - 2, name, 4)
        # the three LEDs, in this order GN (pin 1), WH (pin 2), YE (pin 4)
        self.led_branch(c, 70, -8, "white", "WH")
        self.led_branch(c, 83, -16, "green", "GN")
        self.led_branch(c, 96, -24, "yellow", "YE")
        c.text_c(33, 30, "male", 4)
        c.text_c(167, 56, "female", 4)
        c.text_c(100, 62, self.order, 5)
        return c


class ValveCoil(Product):
    """Generic coil, no manufacturer and no invented order number."""
    kind_en = "Solenoid valve coil"
    kind_fr = "Bobine d'\u00e9lectrovanne"
    ly = -30

    def __init__(self):
        self.short = "form BI 18 mm, 24 V DC"
        self.short_fr = "forme BI 18 mm, 24 V DC"
        super().__init__(
            "m12_valve_coil", "", "valve_coil_bi_18", self.short,
            "Bobine d'\u00e9lectrovanne forme BI 18 mm",
            "Solenoid valve coil, valve connector form BI 18 mm, 24 V DC "
            "(contacts 1, 2 and PE assumed).", 3)
        self.terms = (term("X1", 0, 0, "w"),)

    @property
    def key(self):
        return "generic/valve_coil_bi_18"

    def infos(self):
        return {"designation": self.designation,
                "description": self.description, "function": ""}

    extra_texts = (("function", 78, 33, 6),)

    def names(self):
        return {"en": "Solenoid valve coil form BI 18 mm, 24 V DC",
                "fr": "Bobine d'\u00e9lectrovanne forme BI 18 mm, 24 V DC"}

    def draw(self):
        c = Canvas()
        c.line(0, 0, 10, 0)
        valve_connector(c, 32, 0, True)
        c.line(54, 0, 60, 0, BLACK, "hight")
        # square body with rounded corners, seen from the front
        x0, y0, x1, y1, r = 60, -28, 120, 28, 8
        pts = []
        for cx, cy, a0 in ((x1 - r, y0 + r, -90), (x1 - r, y1 - r, 0),
                           (x0 + r, y1 - r, 90), (x0 + r, y0 + r, 180)):
            for k in range(5):
                a = math.radians(a0 + 22.5 * k)
                pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
        c.poly(pts, True, LIGHT)
        c.text_c(90, -25, "BI 18", 4)
        # centre bore for the valve core
        c.disc(90, -3, 22, "white")
        c.disc(90, -3, 12, "gray")
        # rating plate
        c.rect(66, 13, 48, 11, "white")
        c.text_c(90, 15.5, "24 V DC", 5)
        # flat blade terminals on the side, markings outside the drawing
        for y, name in ((-17, "A1"), (6, "A2")):
            c.rect(120, y, 9, 5, "gray")
            c.text(132, y - 0.5, name, 4)
        return c


class ValvePlug(Product):
    """Valve plug form BI with LED and suppression; X1 is the cable end,
    X2 the side of the valve coil. The internal circuit is drawn between
    the contacts 1 and 2: a LED in series with a resistor, in parallel
    with two suppression diodes in anti-series; PE goes straight through."""
    kind_en = "Valve plug form BI with LED and suppression"
    kind_fr = "Connecteur d'\u00e9lectrovanne forme BI avec LED et protection"
    ly = -50

    def __init__(self, order, file, short, cable, led, source_note):
        self.short = short
        self.short_fr = short
        super().__init__(
            "m12_valve_plug", order, file, short,
            "Connecteur d'\u00e9lectrovanne forme BI, 24 V AC/DC, "
            f"LED et protection, {cable}",
            "MSUD valve plug form BI (11 mm in the datasheet), "
            "24 V AC +-20 % / DC +-25 %, LED " + led + " and suppression "
            "(diode, Zener diode), 4 A per contact max., cut-off peak "
            f"voltage 55 V, {cable}. " + source_note +
            " The internal circuit drawn (LED and resistor in series, in "
            "parallel with two anti-series suppression diodes, between the "
            "contacts 1 and 2; PE straight through) follows the diagram of "
            "the Y distributor datasheet supplied by the user and is "
            "assumed for the plug. Function routing: X1 and X2 are the "
            "same channel.", 3)
        self.terms = (term("X1", 0, 0, "w"), term("X2", 160, 0, "e"))

    def draw(self):
        c = Canvas()
        cable_line(c, 0, 0, 30, 0)
        c.rect(30, -34, 110, 68, "white")
        # the cable wires fan out to the three contacts
        rails = ((-18, "red"), (18, "blue"), (29, "orange"))
        for y, colour in rails:
            c.line(30, 0, 38, y, colour)
            c.line(38, y, 128, y, colour)
        # branch 1: LED in series with a resistor
        c.line(65, -18, 65, -14)
        c.rect(61, -14, 8, 12)
        c.line(65, -2, 65, 3)
        c.poly([(60, 3), (70, 3), (65, 12)], True, "yellow")
        c.line(60, 12, 70, 12)
        c.line(65, 12, 65, 18)
        c.line(72, 6, 79, 1)
        c.line(74, 10, 81, 5)
        c.text(84, 0, "LED", 4)
        # branch 2: two suppression diodes in anti-series
        c.line(105, -18, 105, -14)
        c.poly([(100, -14), (110, -14), (105, -4)], True)
        c.line(100, -4, 110, -4)
        c.line(105, -4, 105, 4)
        c.line(100, 4, 110, 4)
        c.poly([(100, 14), (110, 14), (105, 4)], True)
        c.line(105, 14, 105, 18)
        c.text(113, -2, "Z", 4)
        for x in (65, 105):
            c.disc(x, -18, 3, "black")
            c.disc(x, 18, 3, "black")
        # female contacts on the valve side, markings outside the housing
        blade(c, 128, -22, 8, 8, EV_COLOURS["1"], False)
        blade(c, 128, 14, 8, 8, EV_COLOURS["2"], False)
        blade(c, 128, 25, 8, 8, EV_COLOURS["PE"], False, True)
        c.text(143, -21, "1", 4)
        c.text(143, 15, "2", 4)
        c.text(143, 26, "PE", 4)
        c.line(140, 0, 150, 0)
        c.line(150, 0, 160, 0, BLACK, "hight")
        c.text_c(85, 38, self.order, 5)
        c.text_c(85, 46, "24 V AC/DC", 4)
        return c


CABLE_LENGTHS = ((0.5, "6340050"), (1, "6340100"), (2, "6340200"),
                 (3, "6340300"), (5, "6340500"), (10, "6341000"))
Y_LENGTHS = ((0.6, "6260060"), (0.3, "6260030"))
MSUD = "https://shop.murrelektronik.com/en/Connection-Technology/Valve-Connectors/"
VALVE_PLUGS = (
    ("7000-11021-2160500", "msud_bi_11021_2160500", "with cable",
     "cable 5 m (assumed from the article number; the cable type was not "
     "read)", "(see datasheet)",
     "Source: " + MSUD + "With-open-ended-wires-oxid/"
     "MSUD-valve-plug-BI-11mm-with-cable-7000-11021-2160500.html."),
    ("7000-18141-6R52500", "msud_bi_18141_6r52500", "PUR 5x0.5, 25 m",
     "cable PUR 5 x 0.5 mm2 black, 25 m", "green/yellow",
     "Source: " + MSUD + "With-open-ended-wires-oxid/"
     "MSUD-valve-plug-A-18mm-with-cable-7000-18141-6R52500.html (the shop "
     "title says A-18mm, the text says form BI 11 mm: to check)."),
)

# How a function (signal) travels through the passive elements. A link is
# (terminal, channel, terminal, channel) and works both ways. An EPP M12
# socket carries two channels (1 = pin 4, 2 = pin 2); a branch of a T
# coupler or Y distributor carries one channel, seen as channel 1. The
# families without links (coil, free connector) end a chain.
ROUTES = {
    "m12_cable": [["X1", 1, "X2", 1], ["X1", 2, "X2", 2]],
    "m12_y_distributor": [["X1", 1, "X2", 1], ["X1", 2, "X3", 1]],
    "m12_t_coupler": [["X1", 1, "X2", 1], ["X1", 2, "X3", 1]],
    "m12_valve_plug": [["X1", 1, "X2", 1]],
}
# The channels of a symbol that keeps its function in its own "function"
# information: terminal, channel. They end a chain.
ENDS = {
    "m12_valve_coil": [["X1", 1]],
}
ROUTES_FILE = "function_routes.json"


def products():
    out = [Cable("7000-40021-" + code, length)
           for length, code in CABLE_LENGTHS]
    out += [YDistributor("7000-42421-" + code, length)
            for length, code in Y_LENGTHS]
    out += [FreeConnector("7000-12481-0000000", True),
            FreeConnector("7000-12601-0000000", False)]
    out.append(TCoupler())
    out.append(ValveCoil())
    out += [ValvePlug(*v) for v in VALVE_PLUGS]
    return out


def main():
    root = os.path.join(EXTRA, M12_DIR)
    shutil.rmtree(root, ignore_errors=True)
    write_directory(root, dict(zip(("en", "fr", "de"), ROOT_NAMES)))
    for fam, n in FAMILIES.items():
        write_directory(os.path.join(root, fam),
                        dict(zip(("en", "fr", "de"), n)))
    items = products()
    for p in items:
        p.write()
        for problem in p.draw().layout_problems():
            print(f"drawing problem in {p.file}: {problem}")
    with open(os.path.join(root, ROUTES_FILE), "w", encoding="utf-8",
              newline="\n") as f:
        json.dump(ROUTES, f, indent="\t")
        f.write("\n")
    print(f"{len(items)} M12 products written")


if __name__ == "__main__":
    main()
