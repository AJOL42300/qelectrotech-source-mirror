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
"""Generate the Lanbao sensor library (19_lanbao_sensors).

Three sensors, each with a schematic, a realistic view, a description view
and a layout footprint (1 px = 2 mm):
  PSM-BC100DPB-E2   diffuse photoelectric sensor, M18, PNP NO/NC, M12
  LR18XCF12DPOY-E2  inductive sensor, M18, PNP NO, M12
  PDB-CC50TGI       laser distance sensor, 4...20 mA and switch output, cable
The drawings are original vector drawings; only factual data (references,
ratings, pinouts, dimensions) comes from the public Lanbao datasheets, see
docs/lanbao-sensors.md for the sources and for what is assumed.

Usage: python misc/lanbao/gen_sensors.py
"""
import json
import math
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "murr_meanwell"))
from qetlib import (Canvas, Terminal, write_element, write_directory,
                    RED, BLUE, BLACK)
from gen_elements import EXTRA, AUTHOR

LANBAO_DIR = "19_lanbao_sensors"
MANUFACTURER = "Lanbao"
HEX_RED, HEX_BLUE, HEX_BLACK = "#ff0000", "#0000ff", "#000000"

ROOT_NAMES = ("Lanbao sensors", "Capteurs Lanbao", "Lanbao Sensoren")
FAMILIES = {
    "photoelectric": ("Photoelectric sensors",
                      "Capteurs photo\u00e9lectriques",
                      "Optische Sensoren"),
    "inductive": ("Inductive sensors", "Capteurs inductifs",
                  "Induktive Sensoren"),
    "laser_distance": ("Laser distance sensors",
                       "Capteurs de distance laser",
                       "Laser-Distanzsensoren"),
}
VIEWS = [
    ("", "", ""),
    ("_realistic", ", realistic view", ", vue r\u00e9aliste"),
    ("_description", ", description view", ", vue descriptive"),
]


def term(name, x, y, orient, potential=""):
    return Terminal(name, x, y, orient, "", "", potential)


# ------------------------------------------------------------ shared parts
def arrow(c, x1, y, x2, size=2.5):
    """A horizontal arrow from x1 to x2."""
    c.line(x1, y, x2, y)
    sign = 1 if x2 > x1 else -1
    c.poly([(x2 - sign * size, y - size / 2.0), (x2, y),
            (x2 - sign * size, y + size / 2.0)], False)


def coil(c, x, y, n=3, r=2.0):
    """n small arches in a row, the symbol of an inductive sensor."""
    pts = []
    for k in range(n):
        for step in range(7):
            a = math.pi * step / 6.0
            pts.append((x + 2 * r * k + r - r * math.cos(a),
                        y - r * math.sin(a)))
    c.poly(pts, False)


def callout(c, dot, label, side, ylab, size=4):
    """A dot on the drawing, a leader line and a text at the side."""
    c.disc(dot[0], dot[1], 2, "black")
    xe = -6 if side < 0 else 86
    c.line(dot[0], dot[1], xe, ylab)
    if side < 0:
        c.text_r(xe - 2, ylab - 3.8, label, size)
    else:
        c.text(xe + 2, ylab - 3.8, label, size)


def pin_legend(c, rows, x, y, size=4):
    """The pinout under the drawing: one row per terminal."""
    for k, (s, color) in enumerate(rows):
        c.text(x, y + 8 * k, s, size, False, color)


# ----------------------------------------------------------------- sensors
class Sensor:
    ref = ""
    family = ""
    kind_en = kind_fr = ""
    short = short_fr = ""
    designation = description = ""
    length = height = depth = 0
    leds = ()
    pinout = ()
    marks = {}
    terms = ()
    mark_plus, mark_minus = "1 +", "3 -"

    @property
    def folder(self):
        return self.ref.lower().replace("-", "_")

    @property
    def key(self):
        return MANUFACTURER + "/" + self.ref

    def directory(self):
        return os.path.join(EXTRA, LANBAO_DIR, self.family, self.folder)

    def path(self, view):
        return os.path.join(self.directory(), self.folder + view + ".elmt")

    def infos(self):
        return {"manufacturer": MANUFACTURER,
                "manufacturer_reference": self.ref,
                "designation": self.designation,
                "description": self.description}

    def folder_names(self):
        return {"en": f"{MANUFACTURER} {self.ref} ({self.short})",
                "fr": f"{MANUFACTURER} {self.ref} ({self.short_fr})",
                "de": f"{MANUFACTURER} {self.ref} ({self.short})"}

    def dims_text(self):
        return (f"{self.length:g}x{self.height:g}x{self.depth:g} mm "
                "(width x height x depth)")

    def drawing(self, view):
        """(canvas, y of the label) of one appearance."""
        if view == "":
            return self.schematic(), -24
        if view == "_realistic":
            return self.realistic(), -24
        return self.description_view(), -24

    def write(self):
        write_directory(self.directory(), self.folder_names())
        for view, en, fr in VIEWS:
            names = {"en": f"{MANUFACTURER} {self.ref} {self.kind_en}" + en,
                     "fr": f"{self.kind_fr} {MANUFACTURER} {self.ref}" + fr}
            canvas, ly = self.drawing(view)
            write_element(self.path(view),
                          self.key + "/" + (view or "schematic"), names,
                          self.infos(), AUTHOR, canvas, self.terms,
                          label_at=(0, ly), master=False)
        names = {"en": f"{MANUFACTURER} {self.ref} {self.kind_en}, "
                       "layout (1 px = 2 mm)",
                 "fr": f"{self.kind_fr} {MANUFACTURER} {self.ref}, "
                       "implantation (1 px = 2 mm)"}
        infos = {"manufacturer": MANUFACTURER,
                 "designation": self.designation + " (layout)",
                 "description": f"{MANUFACTURER} {self.ref}, "
                                f"{self.dims_text()}"}
        write_element(self.path("_layout"), self.key + "/layout", names,
                      infos, AUTHOR, self.layout(), (), label_at=(0, -14),
                      master=False)

    def layout_entry(self):
        return {"manufacturer": MANUFACTURER, "reference": self.ref,
                "width": self.length, "height": self.height,
                "depth": self.depth}

    def symbol(self, c):
        raise NotImplementedError

    def schematic(self):
        """A box with the supply on top (red +24 V) and at the bottom
        (blue 0 V) and the outputs on the east side."""
        c = Canvas()
        c.rect(10, 10, 60, 80)
        c.line(40, 0, 40, 10, RED)
        c.line(40, 90, 40, 100, BLUE)
        c.text(43, 1.5, self.mark_plus, 4, True, HEX_RED)
        c.text(43, 91.5, self.mark_minus, 4, True, HEX_BLUE)
        c.text_c(40, 12, self.ref, 4, True)
        self.symbol(c)
        n = len(self.leds)
        for i, colour in enumerate(self.leds):
            c.disc(40 + 20 * (i - (n - 1) / 2.0), 82, 5, colour, BLACK)
        for t in self.terms:
            if t.orient == "e":
                c.line(70, t.y, 80, t.y)
                c.text(71.5, t.y - 7.5, self.marks[t.name], 4, True)
            elif t.orient == "w":
                c.line(0, t.y, 10, t.y)
                c.text(1, t.y - 7.5, self.marks[t.name], 4, True)
        return c

    def legend_rows(self):
        return [(s, HEX_RED if "+24" in s else HEX_BLUE if " 0 V" in s
                 else HEX_BLACK) for s in self.pinout]


class CylinderSensor(Sensor):
    """M18 cylindrical housing with an M12 connector, drawn horizontally
    with the sensing face on the left and the connector on the right."""
    body = 0          # length of the housing without the connector, mm
    collar = 12       # length of the rear part with the LEDs, mm
    diameter = 18
    nut_af, nut_thick = 24, 4
    nut_x = (8, 12)

    def realistic(self):
        c = Canvas()
        s = 80.0 / self.length
        d = self.diameter * s
        y0 = 50 - d / 2.0
        lb = self.body * s
        xc = (self.body - self.collar) * s
        c.rect(0, y0, lb, d, "lightgray")
        x = 3 * s
        while x < xc - 2 * s:
            c.line(x, y0 + 0.8, x, y0 + d - 0.8, "black")
            x += 2.2 * s
        c.rect(xc, y0, lb - xc, d, "gray")
        self.face(c, y0, d, s)
        for nx in self.nut_x:
            c.rect(nx * s, 50 - self.nut_af * s / 2.0, self.nut_thick * s,
                   self.nut_af * s, "gray")
        for x, kind in self.rear_parts(xc, s):
            if kind == "pot":
                c.disc(x, 50, 4.5, "lightgray", BLACK)
                c.line(x - 1.2, 50, x + 1.2, 50)
            else:
                c.disc(x, 50, 4, kind, BLACK)
        cw = 12 * s
        c.rect(lb, 50 - cw / 2.0, 80 - lb, cw, "lightgray")
        k = lb + 1.2
        while k < 80 - 1.0:
            c.line(k, 50 - cw / 2.0 + 0.8, k, 50 + cw / 2.0 - 0.8, "black")
            k += 1.6
        c.text_c(40, 50 + self.nut_af * s / 2.0 + 3, self.ref, 4, True)
        return c

    def rear_parts(self, xc, s):
        step = self.collar * s / (len(self.leds) + 1)
        return [(xc + step * (i + 1), colour)
                for i, colour in enumerate(self.leds)]

    def description_view(self):
        c = self.realistic()
        s = 80.0 / self.length
        d = self.diameter * s
        xc = (self.body - self.collar) * s
        for dot, label, side, ylab in self.callouts(s, d, xc):
            callout(c, dot, label, side, ylab)
        pin_legend(c, self.legend_rows(), 50, 108)
        return c

    def layout(self):
        """Side view, 1 px = 2 mm: the housing is the first rectangle."""
        c = Canvas()
        w, h = self.length / 2.0, self.diameter / 2.0
        c.rect(0, 0, w, h)
        c.line(self.body / 2.0, 0, self.body / 2.0, h)
        for y in (1.5, h - 1.5):
            c.line(0, y, self.body / 2.0, y, BLACK, dash="dashed")
        c.text(0, h + 2, self.ref, 4, True)
        return c


class Psm(CylinderSensor):
    ref = "PSM-BC100DPB-E2"
    family = "photoelectric"
    kind_en = "diffuse photoelectric sensor"
    kind_fr = "Capteur photo\u00e9lectrique \u00e0 r\u00e9flexion diffuse"
    short = "M18, 100 cm, PNP NO/NC, M12"
    short_fr = short
    length, height, depth = 47.2, 18, 18
    body = 36
    leds = ("green", "yellow")
    designation = ("Capteur photo\u00e9lectrique \u00e0 r\u00e9flexion "
                   "diffuse M18, port\u00e9e 100 cm, PNP NO/NC, "
                   "10-30 V DC, connecteur M12")
    description = ("Diffuse photoelectric sensor, metal M18x1 housing, "
                   "sensing distance 100 cm (infrared 940 nm), PNP output "
                   "NO/NC selectable with pin 2, 10-30 V DC, 200 mA, "
                   "single-turn distance adjustment, green LED (power) "
                   "and yellow LED (output), IP67, M12 4-pin connector. "
                   "Pins: 1 BN +24 V, 2 WH NO/NC selection (open or +24 V "
                   "= NO, 0 V = NC), 3 BU 0 V, 4 BK output. Mounting: "
                   "M18x1 thread, 2 nuts supplied. Overall length 47.2 mm "
                   "with the connector (the nuts, LED and adjustment "
                   "positions of the drawing are generic).")
    marks = {"1": "1", "2": "2", "3": "3", "4": "4"}
    pinout = ("1 BN: +24 V DC", "2 WH: NO/NC selection", "3 BU: 0 V",
              "4 BK: PNP output")
    terms = (term("1", 40, 0, "n", "24V"), term("2", 0, 50, "w"),
             term("3", 40, 100, "s", "0V"), term("4", 80, 50, "e"))

    def symbol(self, c):
        c.poly([(40, 24), (52, 36), (40, 48), (28, 36)])
        arrow(c, 33, 33, 44)
        arrow(c, 47, 39, 36)
        c.text_c(40, 53, "PNP NO/NC", 4)

    def face(self, c, y0, d, s):
        c.rect(0, y0 + 2, 2 * s, d - 4, "white")

    def rear_parts(self, xc, s):
        step = self.collar * s / 4.0
        return [(xc + step, "green"), (xc + 2 * step, "yellow"),
                (xc + 3 * step, "pot")]

    def callouts(self, s, d, xc):
        return [((2, 50 - d / 4.0), "Sensing window", -1, 20),
                ((10 * s, 50 - 12 * s), "M18x1 nuts", -1, 6),
                ((xc + 3 * s, 48), "Green LED: power", 1, 6),
                ((xc + 6 * s, 48), "Yellow LED: output", 1, 18),
                ((xc + 9 * s, 48), "Distance adjustment", 1, 88),
                ((74, 46), "M12 connector", 1, 30)]


class Lr18x(CylinderSensor):
    ref = "LR18XCF12DPOY-E2"
    family = "inductive"
    kind_en = "inductive sensor"
    kind_fr = "Capteur inductif"
    short = "M18, Sn 12 mm, PNP NO, M12"
    short_fr = short
    length, height, depth = 73, 18, 18
    body = 56
    leds = ("yellow",)
    designation = ("Capteur inductif cylindrique M18 quasi noy\u00e9, "
                   "Sn 12 mm, PNP NO, 10-30 V DC, connecteur M12")
    description = ("Inductive sensor, M18x1 housing, quasi flush, nominal "
                   "range Sn 12 mm (assured 0-9.6 mm), PNP NO, 10-30 V DC, "
                   "200 mA, 300 Hz, yellow output LED, protected against "
                   "short circuit, overload and reverse polarity, IP67, "
                   "M12 connector. Pins: 1 BN +24 V, 3 BU 0 V, 4 BK output "
                   "(NO), pin 2 not used on this variant. Wire colours are "
                   "assumed (standard M12). Mounting: M18x1 thread, 2 nuts "
                   "AF24 4 mm thick. Overall length 73 mm with the "
                   "connector (body 56 mm).")
    marks = {"1": "1", "3": "3", "4": "4"}
    pinout = ("1 BN*: +24 V DC", "3 BU*: 0 V", "4 BK*: PNP output NO",
              "* colours assumed")
    terms = (term("1", 40, 0, "n", "24V"), term("3", 40, 100, "s", "0V"),
             term("4", 80, 50, "e"))

    def symbol(self, c):
        c.poly([(40, 24), (52, 36), (40, 48), (28, 36)])
        coil(c, 34, 38, 3, 2)
        c.text_c(40, 53, "PNP NO", 4)

    def face(self, c, y0, d, s):
        c.rect(0, y0 + 2, 1.5 * s, d - 4, "black")

    def callouts(self, s, d, xc):
        return [((1, 50 - d / 4.0), "Active face (Sn 12 mm)", -1, 20),
                ((10 * s, 50 - 12 * s), "M18x1 nuts AF24", -1, 6),
                ((xc + 6 * s, 48), "Yellow LED: output", 1, 14),
                ((74, 46), "M12 connector", 1, 30)]


class Pdb(Sensor):
    ref = "PDB-CC50TGI"
    family = "laser_distance"
    kind_en = "laser distance sensor"
    kind_fr = "Capteur de distance laser"
    short = "80-500 mm, 4-20 mA + switch, cable"
    short_fr = "80-500 mm, 4-20 mA + tout ou rien, c\u00e2ble"
    length, height, depth = 51, 65, 23
    leds = ("green", "yellow")
    designation = ("Capteur de distance laser 80-500 mm, sortie analogique "
                   "4-20 mA et sortie tout ou rien, 12-24 V DC, c\u00e2ble "
                   "blind\u00e9 2 m")
    description = ("Laser distance measuring sensor (red laser 650 nm, "
                   "class 2), measuring range 80-500 mm (full scale 420 "
                   "mm), 4-20 mA analog output (load below 390 ohm) and "
                   "switch output Q (push-pull, NPN or PNP, NO or NC "
                   "settable), 12-24 V DC, OLED display and two keys, "
                   "green power LED and yellow action LED, IP67, 2 m "
                   "shielded 4-wire cable. Wires: BN +24 V, BU 0 V, BK "
                   "switch output Q, GY analog output Ia (4-20 mA, the "
                   "load is wired between Ia and 0 V). Mounting: two "
                   "holes 4.4 mm (M4 screws), 65x51x23 mm. The cable "
                   "shield is not drawn. Hole positions are read from "
                   "the datasheet drawing (about 1 mm).")
    marks = {"BN": "BN +", "BU": "BU -", "BK": "BK Q", "GY": "GY Ia"}
    mark_plus, mark_minus = "BN +", "BU -"
    pinout = ("BN: +24 V DC", "BU: 0 V", "BK: switch output Q",
              "GY: analog output Ia 4-20 mA")
    terms = (term("BN", 40, 0, "n", "24V"), term("BU", 40, 100, "s", "0V"),
             term("BK", 80, 30, "e"), term("GY", 80, 70, "e"))
    holes = ((5.5, 4.6), (46.5, 46.3))     # mm from the top left corner

    def symbol(self, c):
        c.poly([(40, 24), (52, 36), (40, 48), (28, 36)])
        arrow(c, 33, 33, 44)
        arrow(c, 47, 39, 36)
        c.disc(40, 43, 3, "red", RED)
        c.text_c(40, 53, "Q: switch", 4)
        c.text_c(40, 61, "Ia: 4-20 mA", 4)

    def realistic(self):
        c = Canvas()
        s = 1.3
        w, h = 23 * s, 65 * s
        x0, y0 = 40 - w / 2.0, 50 - h / 2.0
        c.rect(x0, y0, w, h, "lightgray")
        c.rect(x0 + 2, y0 + 2, w - 4, 33 * s, "red")
        c.rect(x0 + 2, y0 + 4 + 33 * s, w - 4, 19.5 * s, "red")
        c.rect(x0 + 8, y0 + h - 4, w - 16, 3, "gray")
        c.line(40, y0 + h, 40, 100, BLACK, "hight")
        c.text(x0 - 12, 50 + 14, self.ref, 4, True, rotation=270)
        return c

    def description_view(self):
        c = self.realistic()
        s = 1.3
        h = 65 * s
        x0, y0 = 40 - 23 * s / 2.0, 50 - h / 2.0
        callout(c, (x0 + 5, y0 + 20), "Receiving window", -1, 12)
        callout(c, (x0 + 5, y0 + 2 * 4 + 33 * s + 10), "Emitting window",
                -1, 80)
        callout(c, (40, 94), "Cable 2 m, 4 wires, shielded", 1, 92)
        pin_legend(c, self.legend_rows(), 50, 108)
        return c

    def layout(self):
        """Mounting face, 1 px = 2 mm; the housing is the first rect."""
        c = Canvas()
        c.rect(0, 0, self.length / 2.0, self.height / 2.0)
        for hx, hy in self.holes:
            c.disc(hx / 2.0, hy / 2.0, 2.2)
        c.text(self.length / 4.0 - 2, self.height / 2.0 - 3, self.ref, 3,
               True, rotation=270)
        return c


SENSORS = (Psm(), Lr18x(), Pdb())


def main():
    root = os.path.join(EXTRA, LANBAO_DIR)
    shutil.rmtree(root, ignore_errors=True)
    write_directory(root, dict(zip(("en", "fr", "de"), ROOT_NAMES)))
    for fam, n in FAMILIES.items():
        write_directory(os.path.join(root, fam),
                        dict(zip(("en", "fr", "de"), n)))
    problems = 0
    for sensor in SENSORS:
        sensor.write()
        for view, _en, _fr in VIEWS:
            for p in sensor.drawing(view)[0].layout_problems():
                problems += 1
                print(f"{sensor.ref}{view}: {p}")
        for p in sensor.layout().layout_problems():
            problems += 1
            print(f"{sensor.ref}_layout: {p}")
    with open(os.path.join(root, "devices_layout.json"), "w",
              encoding="utf-8", newline="\n") as f:
        json.dump({"devices": [s.layout_entry() for s in SENSORS]}, f,
                  indent=2)
        f.write("\n")
    print(f"{len(SENSORS)} sensors, {len(SENSORS) * (len(VIEWS) + 1)} "
          f"elements written, {problems} drawing problems")


if __name__ == "__main__":
    main()
