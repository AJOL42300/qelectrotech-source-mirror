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
}

# Pin positions seen from the mating face of a male A-coded connector;
# a female connector seen from its face is the mirror image.
PIN_XY = {1: (-8, -8), 2: (8, -8), 3: (8, 8), 4: (-8, 8), 5: (0, 0)}
RADIUS = 20


def pin_xy(n, male):
    x, y = PIN_XY[n]
    if not male:
        x = {1: 8, 2: -8, 3: -8, 4: 8, 5: 0}[n]
    return x, y


def connector(c, cx, cy, male, poles):
    """An M12 connector: coupling nut, insulator, numbered pins."""
    c.disc(cx, cy, 2 * RADIUS, LIGHT)
    c.disc(cx, cy, 2 * RADIUS - 10, "white")
    for n in range(1, poles + 1):
        x, y = pin_xy(n, male)
        px, py = cx + x, cy + y
        if male:
            c.disc(px, py, 3, "black")
        else:
            c.disc(px, py, 4, "white")
        if x < 0:
            c.text_r(px - 3, py - 3.2, str(n), 4)
        else:
            c.text(px + 3, py - 3.2, str(n), 4)


def valve_connector(c, cx, cy, male=False):
    """A valve connector form BI with two contacts and PE (assumed): socket
    contacts on the cable side, pins on the valve coil side."""
    c.rect(cx - 20, cy - 12, 40, 24, LIGHT)
    for k, name in enumerate(("1", "2", "PE")):
        x = cx - 12 + 12 * k
        c.disc(x, cy - 2, 5 if not male else 3, "black" if male else "white")
        c.text_c(x, cy + 3, name, 4)


def cable_line(c, x1, y1, x2, y2):
    c.line(x1, y1, x2, y2, BLACK, "hight")


def boot(c, x, y, w):
    c.rect(x, y - 8, w, 16, LIGHT)


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

    def write(self):
        canvas = self.draw()
        write_element(self.path(), self.key, self.names(), self.infos(),
                      AUTHOR, canvas, self.terms, label_at=(0, self.ly),
                      master=False,
                      extra_texts=getattr(self, "extra_texts", ()))


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
            "M12 male 0 deg / M12 female 0 deg, A-coded, 4 poles, "
            f"PUR 4 x 0.34 mm2, {metres(length)} m. "
            "Pins 1 brown, 2 white, 3 blue, 4 black. Function routing: "
            "X1 and X2 are the same channels (1 = pin 4, 2 = pin 2).", 4)
        self.terms = (term("X1", 0, 0, "w"), term("X2", 200, 0, "e"))

    def draw(self):
        c = Canvas()
        c.line(0, 0, 10, 0)
        c.line(190, 0, 200, 0)
        connector(c, 30, 0, True, 4)
        connector(c, 170, 0, False, 4)
        boot(c, 50, 0, 8)
        boot(c, 142, 0, 8)
        c.rect(58, -4, 84, 8, "white")
        c.text_c(100, -20, f"{metres(self.length)} m", 6, True)
        c.text_c(100, 8, self.order, 5)
        c.text_c(100, 16, "PUR 4x0,34", 4)
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
            "Y distributor, M12 male A-coded, 4 poles, to two valve "
            f"connectors form BI 18 mm, {metres(length)} m. The contacts "
            "of the valve connectors are assumed (1, 2, PE). Function "
            "routing: channel 1 (pin 4) to X2, channel 2 (pin 2) to X3.", 4)
        self.length = length
        self.terms = (term("X1", 0, 0, "w"), term("X2", 200, -30, "e"),
                      term("X3", 200, 30, "e"))

    def draw(self):
        c = Canvas()
        c.line(0, 0, 10, 0)
        c.line(190, -30, 200, -30)
        c.line(190, 30, 200, 30)
        connector(c, 30, 0, True, 4)
        boot(c, 50, 0, 8)
        cable_line(c, 58, 0, 90, 0)
        cable_line(c, 90, 0, 110, -30)
        cable_line(c, 110, -30, 150, -30)
        cable_line(c, 90, 0, 110, 30)
        cable_line(c, 110, 30, 150, 30)
        c.disc(90, 0, 3, "black")
        valve_connector(c, 170, -30)
        valve_connector(c, 170, 30)
        c.text_c(100, 48, f"{metres(self.length)} m", 6, True)
        c.text_c(100, 56, self.order, 5)
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
        connector(c, 30, 0, self.male, 4)
        boot(c, 50, 0, 20)
        c.text_c(60, 12, "IDC", 5)
        c.text_c(40, 26, self.order, 5)
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
            "(4 p\u00f4les) cod\u00e9 A, LED",
            "T coupler (Slim Line), M12 male 5 poles A-coded to two M12 "
            "female 4 poles A-coded, LEDs, distribution function. Function "
            "routing: channel 1 (pin 4) to X2, channel 2 (pin 2) to X3.", 5)
        self.terms = (term("X1", 0, 0, "w"), term("X2", 200, -30, "e"),
                      term("X3", 200, 30, "e"))

    def draw(self):
        c = Canvas()
        c.line(0, 0, 10, 0)
        c.line(190, -30, 200, -30)
        c.line(190, 30, 200, 30)
        connector(c, 30, 0, True, 5)
        connector(c, 170, -30, False, 4)
        connector(c, 170, 30, False, 4)
        c.rect(50, -45, 70, 90, LIGHT)
        cable_line(c, 120, -30, 150, -30)
        cable_line(c, 120, 30, 150, 30)
        for k in range(3):
            c.disc(70 + 15 * k, -25, 5, GREEN)
        c.text_c(85, -17, "LED", 4)
        c.text_c(85, 25, self.order, 5)
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

    extra_texts = (("function", 90, 18, 6),)

    def names(self):
        return {"en": "Solenoid valve coil form BI 18 mm, 24 V DC",
                "fr": "Bobine d'\u00e9lectrovanne forme BI 18 mm, 24 V DC"}

    def draw(self):
        c = Canvas()
        c.line(0, 0, 10, 0)
        valve_connector(c, 30, 0, True)
        c.line(50, 0, 90, 0)
        c.rect(90, -15, 60, 30, "white")
        c.text(93, -14, "A1", 4)
        c.text(93, 8, "A2", 4)
        c.text_c(125, -3, "24 V DC", 5)
        return c


CABLE_LENGTHS = ((0.5, "6340050"), (1, "6340100"), (2, "6340200"),
                 (3, "6340300"), (5, "6340500"), (10, "6341000"))
Y_LENGTHS = ((0.6, "6260060"), (0.3, "6260030"))

# How a function (signal) travels through the passive elements. A link is
# (terminal, channel, terminal, channel) and works both ways. An EPP M12
# socket carries two channels (1 = pin 4, 2 = pin 2); a branch of a T
# coupler or Y distributor carries one channel, seen as channel 1. The
# families without links (coil, free connector) end a chain.
ROUTES = {
    "m12_cable": [["X1", 1, "X2", 1], ["X1", 2, "X2", 2]],
    "m12_y_distributor": [["X1", 1, "X2", 1], ["X1", 2, "X3", 1]],
    "m12_t_coupler": [["X1", 1, "X2", 1], ["X1", 2, "X3", 1]],
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
    with open(os.path.join(root, ROUTES_FILE), "w", encoding="utf-8",
              newline="\n") as f:
        json.dump(ROUTES, f, indent="\t")
        f.write("\n")
    print(f"{len(items)} M12 products written")


if __name__ == "__main__":
    main()
