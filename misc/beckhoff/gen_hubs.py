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
"""Write the EK1100 coupler and the CU1128 junction (schematic, realistic
and table views; the description view is made by gen_description.py).

Public facts used (Beckhoff information system and data sheets, only the
facts):
- EK1100: EtherCAT coupler, 44 x 100 x 68 mm (W x H x D), two RJ45
  sockets (the upper one is the EtherCAT input, the lower one the output
  towards further EtherCAT devices), supply Us / Up 24 V DC, three spring
  power contacts (24 V, 0 V, PE) on the RIGHT face only, Up max. 24 V DC,
  10 A.
- CU1128: EtherCAT junction, 122 x 100 x 38 mm, 8 RJ45 sockets 100BASE-TX
  (port 1 = input from the master, ports 2 to 8 = outputs), supply 24 V DC
  (18 ... 30 V) on a 3-pin connector (+, -, PE), one link/activity LED per
  socket.  No E-bus and no power contact.

ASSUMED (not public, to be checked): the position of the sockets, the
supply terminals and the LEDs on the housing, the numbering of the supply
terminals (1 = +24 V, 2 = 0 V, 3 = PE; for the EK1100: 1 = Us +24 V,
2 = Us 0 V, 3 = Up +24 V, 4 = Up 0 V), the LEDs of the EK1100 (Us, Up, Run,
L/A of each socket), the arrangement of the CU1128 sockets (two rows of
four).

The sockets are ONE terminal each.  The cards have no I/O channel: the
table lists the function of each terminal with an empty TwinCAT ID.
Vector drawing only, no logo and no manufacturer text.

Run it after organize.py (the family folder must exist), before
gen_schematic.py / gen_description.py.

Usage: python gen_hubs.py <extra-elements>/10_electric/20_manufacturers_articles/beckhoff
"""
import copy
import math
import os
import re
import sys
import uuid
import xml.etree.ElementTree as ET

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gen_schematic import (DOTTED, POT_FILLED, POT_STYLE, POT_TEXT,  # noqa
                           THIN, WHITE, Sheet, CLIP_H, CLIP_W, POWER_Y)
from gen_ek1322 import (BLACK, DARK, FONT, FONT_BOLD, LED_STYLE,  # noqa
                        Draw, INFO)
from gen_el9410 import PE_FILL_REAL, PE_FILLED, PE_STYLE, PE_TEXT  # noqa
from realistic_fixes import num, text_width, write  # noqa: E402
import simple_tables  # noqa: E402

NS = uuid.UUID("6f1c2a5e-8b0f-4d57-9a7e-5b1d0c8e2f10")
FAMILY = "ek_couplers"
TEXT_COLOR = {"24V": "#ff0000", "0V": "#0000ff", "PE": PE_TEXT}
REAL_FILL = {"24V": POT_FILLED["24V"].replace("line-style:normal",
                                              "line-style:none"),
             "0V": POT_FILLED["0V"].replace("line-style:normal",
                                            "line-style:none"),
             "PE": PE_FILL_REAL}
WIRE = {"24V": POT_STYLE["24V"], "0V": POT_STYLE["0V"], "PE": PE_STYLE}
JUNCTION = {"24V": POT_FILLED["24V"], "0V": POT_FILLED["0V"],
            "PE": PE_FILLED}
GREY = "line-style:normal;line-weight:thin;filling:lightgray;color:black"
SLOT = "line-style:normal;line-weight:thin;filling:black;color:black"


class Hub:
    """Everything that tells the two cards apart."""
    stem = reference = designation = description = ""
    names = {}
    body = (0, 0, 0, 0)         # x, y, width, height of the housing
    cx = 0.0                    # axis of the housing
    pins = {}                   # name -> (x, y, orientation, potential)
    leds = {}                   # name -> (x, y)
    led_text = {}
    function = ()               # (terminal, text) in table order
    root_size = (84, 330, 38, 160)    # width, height, hotspot x, y

    def front(self, d):
        raise NotImplementedError

    def diagram(self, s):
        raise NotImplementedError

    def legends(self):
        """(side, y, x0, lines, size) of the description legends."""
        raise NotImplementedError



def rj45(d, cx, cy, w=46.0, h=40.0):
    """Front of an RJ45 socket: grey frame, opening, eight contacts."""
    d.rect(cx - w / 2, cy - h / 2, w, h, DARK)
    d.rect(cx - w / 2 + 5, cy - h / 2 + 8, w - 10, h - 14, WHITE)
    d.rect(cx - 6, cy - h / 2 + 3, 12, 5, WHITE)          # latch notch
    for k in range(8):
        x = cx - (w - 18) / 2 + k * (w - 18) / 7.0
        d.line(x, cy - h / 2 + 10, x, cy - h / 2 + 18, THIN)


def feed_terminal(d, name, x, y, pot, text):
    """Circle of a spring terminal with its coloured mark and label."""
    d.circle(x, y, 16.2, WHITE)
    d.circle(x, y, 12.6, WHITE)
    d.rect(x - 5.5, y - 11.2, 11, 1.6, REAL_FILL[pot])
    d.text_centered(x, y - 12.6, text, 7, True, TEXT_COLOR[pot])


class EK1100(Hub):
    stem = "ek1100"
    reference = "EK1100"
    designation = "EtherCAT coupler, 2 x RJ45"
    description = (
        "EtherCAT coupler: the upper RJ45 socket is the EtherCAT input "
        "(EtherCAT IN), the lower one the output towards further EtherCAT "
        "devices (EtherCAT OUT); supply terminals 1 Us +24 V DC, 2 Us 0 V, "
        "3 Up +24 V DC, 4 Up 0 V; three power contacts (24 V, 0 V, PE) on "
        "the right face. The position of the terminals and of the LEDs (Us, "
        "Up, Run, L/A IN, L/A OUT) is assumed. The label is the island id.")
    names = {
        "": ("EK1100 EtherCAT coupler, 2 x RJ45",
             "EK1100 coupleur EtherCAT, 2 x RJ45"),
        "_realistic": ("EK1100 EtherCAT coupler, realistic view",
                       "EK1100 coupleur EtherCAT, vue r\u00e9aliste"),
        "_table": ("EK1100 EtherCAT coupler - terminal table",
                   "EK1100 coupleur EtherCAT - tableau des bornes"),
    }
    # 44 mm at 3.125 px per mm
    body = (-63.0, -136.7, 137.5, 277.2)
    cx = 5.75
    pins = {"EtherCAT IN": (cx, -78, "w", None),
            "EtherCAT OUT": (cx, -28, "e", None),
            "1": (-35, 45, "s", "24V"), "2": (-35, 100, "s", "0V"),
            "3": (45, 45, "s", "24V"), "4": (45, 100, "s", "0V")}
    leds = {"Us": (-40, -122), "Up": (-40, -111), "Run": (-40, -100),
            "L/A IN": (50, -122), "L/A OUT": (50, -111)}
    led_text = {"Us": "Us", "Up": "Up", "Run": "Run", "L/A IN": "L/A1",
                "L/A OUT": "L/A2"}
    function = (("EtherCAT IN", "EtherCAT IN (RJ45)"),
                ("EtherCAT OUT", "EtherCAT OUT (RJ45)"),
                ("1", "+24 V DC (Us)"), ("2", "0 V (Us)"),
                ("3", "+24 V DC (Up)"), ("4", "0 V (Up)"))
    root_size = (150, 330, 70, 160)
    contacts_y = (POWER_Y["24V"] - 2, POWER_Y["0V"] - 2,
                  POWER_Y["PE"] - 2)   # 24 V, 0 V, PE contacts (right)

    def front(self, d):
        x, y, w, h = self.body
        d.rect(x, y, w, h, THIN)
        d.line(x, y + 12, x + w, y + 12)
        d.line(x, y + h - 12, x + w, y + h - 12)
        for name in ("EtherCAT IN", "EtherCAT OUT"):
            px, py, _o, _p = self.pins[name]
            rj45(d, px, py)
            d.text(px - 33, py + 2, name[-3:].strip(), 5)
        for led, (lx, ly) in self.leds.items():
            d.circle(lx, ly, 4, LED_STYLE)
            t = self.led_text[led]
            if lx < self.cx:
                d.text(lx + 4, ly + 2.2, t, 4)
            else:
                d.text(lx - 4 - text_width(t) * 4 / 7.0, ly + 2.2, t, 4)
        for name in ("1", "2", "3", "4"):
            px, py, _o, pot = self.pins[name]
            feed_terminal(d, name, px, py, pot,
                          "24V" if pot == "24V" else "0V")
        d.text_centered(-35, 22, "Us", 7, True)
        d.text_centered(45, 22, "Up", 7, True)
        # three spring contacts on the right face
        for yy, pot in zip(self.contacts_y, ("24V", "0V", "PE")):
            d.rect(x + w - 6, yy, 6, 4, REAL_FILL[pot])
        d.text_centered(self.cx, 130, self.reference, 8, True)

    def diagram(self, s):
        x0, y0, w, h = self.body
        s.rect(x0, y0, w, h, DOTTED)
        for top in (True, False):
            y = y0 - CLIP_H if top else y0 + h
            s.rect(self.cx - CLIP_W / 2, y, CLIP_W, CLIP_H, THIN)
            inner = y + (2.0 if top else CLIP_H - 2.0)
            s.line(self.cx - CLIP_W / 2 + 3, inner,
                   self.cx + CLIP_W / 2 - 3, inner)
        r = 4.5
        # the coupler between the two sockets
        bx0, bx1, by0, by1 = -55.0, 65.0, -66.0, -52.0
        s.rect(bx0, by0, bx1 - bx0, by1 - by0, WHITE)
        s.text_centered(self.cx + 5, (by0 + by1) / 2, "EtherCAT coupler", 5)
        px, py, _o, _p = self.pins["EtherCAT IN"]
        s.line(px, py + 7, px, by0)
        px, py, _o, _p = self.pins["EtherCAT OUT"]
        s.line(px, py - 7, px, by1)
        # E-bus towards the right face
        s.line(bx1, (by0 + by1) / 2, x0 + w, (by0 + by1) / 2)
        s.text(bx1 + 3, (by0 + by1) / 2 - 2, "E-bus", 4)
        # power contacts: rails stopping on the right face (Up), the
        # coupler has no contact on its left face
        y24, y0v = POWER_Y["24V"], POWER_Y["0V"]
        s.power_rails(x0, x0 + w, y24, y0v, left=False, right=True)
        # Us: straight into the coupler; Up: onto the rails
        wires = {"1": ("24V", -35), "2": ("0V", -60),
                 "3": ("24V", 45), "4": ("0V", 62)}
        for name, (pot, rx) in wires.items():
            px, py, _o, _p = self.pins[name]
            if name in ("1", "2"):
                if name == "1":
                    s.line(px, py - r, px, by1, WIRE[pot])
                    s.circle(px, by1, 2.4, JUNCTION[pot])
                else:
                    s.poly([(px - r, py), (rx, py), (rx, (by0 + by1) / 2),
                            (bx0, (by0 + by1) / 2)], closed=False,
                           style=WIRE[pot])
            elif name == "3":
                s.line(px, py - r, px, y24, WIRE[pot])
                s.circle(px, y24, 2.4, JUNCTION[pot])
            else:
                s.poly([(px + r, py), (rx, py), (rx, y0v)], closed=False,
                       style=WIRE[pot])
                s.circle(rx, y0v, 2.4, JUNCTION[pot])
        s.text(-31, 30, "Us", 6, True)
        s.text(49, 30, "Up", 6, True)
        for name, (px, py, _o, _p) in self.pins.items():
            if name.startswith("Ether"):
                s.circle(px, py, 14, WHITE)
                s.text_centered(px, py, name[-3:].strip(), 5)
            else:
                s.circle(px, py, 9.0, WHITE)
                s.text_centered(px, py, name, 5)
        s.text_centered(self.cx, 130, self.reference, 8)
        s.parts[-1].set("font", FONT_BOLD % 8)

    def legends(self):
        out = []
        for name in ("EtherCAT IN", "EtherCAT OUT"):
            px, py, _o, _p = self.pins[name]
            word = "input" if name.endswith("IN") else "output"
            out.append(("R", py, px + 23 + 1.5,
                        ["%s: RJ45 socket" % name, "EtherCAT %s" % word], 6))
        for name in ("1", "2", "3", "4"):
            px, py, _o, _p = self.pins[name]
            side = "L" if px < self.cx else "R"
            x0 = px + (-1 if side == "L" else 1) * (8.1 + 1.5)
            out.append((side, py, x0,
                        ["%s: %s" % (name, dict(self.function)[name])], 7))
        words = {"Us": "LED Us (system supply)",
                 "Up": "LED Up (peripheral supply)", "Run": "LED Run",
                 "L/A IN": "LED L/A IN", "L/A OUT": "LED L/A OUT"}
        for led, (lx, ly) in self.leds.items():
            side = "L" if lx < self.cx else "R"
            out.append((side, ly, lx + (-1 if side == "L" else 1) * 3.5,
                        [words[led]], 6))
        right = self.body[0] + self.body[2]
        out.append(("R", self.contacts_y[1] + 2, right + 1.5,
                    ["Power contacts", "24 V / 0 V / PE"], 6))
        return out



class CU1128(Hub):
    stem = "cu1128"
    reference = "CU1128"
    designation = "EtherCAT junction, 8 x RJ45"
    description = (
        "EtherCAT junction with eight RJ45 sockets (100BASE-TX): port 1 is "
        "the input from the master, ports 2 to 8 are outputs. Supply 24 V "
        "DC (18 to 30 V) on a three-pin connector: 1 +24 V, 2 0 V, 3 PE. "
        "One link/activity LED per socket. No E-bus and no power contact. "
        "The position of the sockets (two rows of four), of the supply "
        "terminals and of the LEDs is assumed.")
    names = {
        "": ("CU1128 EtherCAT junction, 8 x RJ45",
             "CU1128 jonction EtherCAT, 8 x RJ45"),
        "_realistic": ("CU1128 EtherCAT junction, realistic view",
                       "CU1128 jonction EtherCAT, vue r\u00e9aliste"),
        "_table": ("CU1128 EtherCAT junction - terminal table",
                   "CU1128 jonction EtherCAT - tableau des bornes"),
    }
    # 122 mm at 3.125 px per mm
    body = (-190.0, -136.7, 381.0, 277.2)
    cx = 0.5
    cols = (-45.0, 5.0, 55.0, 105.0)
    rows = (-72.0, 8.0)
    pins = {}
    leds = {}
    led_text = {}
    function = ()
    root_size = (400, 330, 200, 160)

    def __init__(self):
        pins, leds, led_text, function = {}, {}, {}, []
        for n in range(8):
            px = self.cols[n % 4]
            py = self.rows[n // 4]
            name = "P%d" % (n + 1)
            pins[name] = (px, py, "n" if n < 4 else "s", None)
            leds["L/A %d" % (n + 1)] = (px - 18, py - 26)
            led_text["L/A %d" % (n + 1)] = "L/A%d" % (n + 1)
            role = "input from the master" if n == 0 else "output"
            function.append((name, "Port %d: RJ45, EtherCAT %s"
                             % (n + 1, role)))
        for k, (name, pot, text) in enumerate((
                ("1", "24V", "+24 V DC"), ("2", "0V", "0 V"),
                ("3", "PE", "PE"))):
            pins[name] = (-150 + 35 * k, 62, "s", pot)
            function.append((name, text))
        self.pins, self.leds, self.led_text = pins, leds, led_text
        self.function = tuple(function)

    def front(self, d):
        x, y, w, h = self.body
        d.rect(x, y, w, h, THIN)
        d.line(x, y + 12, x + w, y + 12)
        d.line(x, y + h - 12, x + w, y + h - 12)
        for n in range(8):
            px, py, _o, _p = self.pins["P%d" % (n + 1)]
            rj45(d, px, py)
            d.text_centered(px, py + 30, "%d" % (n + 1), 6)
        for led, (lx, ly) in self.leds.items():
            d.circle(lx, ly, 4, LED_STYLE)
        # supply connector: three terminal points
        d.rect(-168, 38, 106, 50, DARK)
        for name in ("1", "2", "3"):
            px, py, _o, pot = self.pins[name]
            d.circle(px, py, 16.2, WHITE)
            d.circle(px, py, 12.6, WHITE)
            d.rect(px - 5.5, py - 11.2, 11, 1.6, REAL_FILL[pot])
            d.text_centered(px, py - 12.6,
                            {"24V": "24V", "0V": "0V", "PE": "PE"}[pot], 7,
                            True, TEXT_COLOR[pot])
        d.text_centered(self.cx, 130, self.reference, 8, True)

    def diagram(self, s):
        x0, y0, w, h = self.body
        s.rect(x0, y0, w, h, DOTTED)
        for top in (True, False):
            y = y0 - CLIP_H if top else y0 + h
            s.rect(self.cx - CLIP_W / 2, y, CLIP_W, CLIP_H, THIN)
            inner = y + (2.0 if top else CLIP_H - 2.0)
            s.line(self.cx - CLIP_W / 2 + 3, inner,
                   self.cx + CLIP_W / 2 - 3, inner)
        # the switch between the eight ports
        bx0, bx1, by0, by1 = -70.0, 130.0, -26.0, -14.0
        s.rect(bx0, by0, bx1 - bx0, by1 - by0, WHITE)
        s.text_centered((bx0 + bx1) / 2, (by0 + by1) / 2, "EtherCAT junction",
                        5)
        for n in range(8):
            px, py, _o, _p = self.pins["P%d" % (n + 1)]
            if n < 4:
                s.line(px, py + 7, px, by0)
            else:
                s.line(px, py - 7, px, by1)
        # supply: the three wires go to the junction
        for name in ("1", "2", "3"):
            px, py, _o, pot = self.pins[name]
            k = int(name) - 1
            ex = -30.0 + 10 * k
            s.poly([(px, py - 4.5), (px, 28 + 6 * k), (ex, 28 + 6 * k),
                    (ex, by1)], closed=False, style=WIRE[pot])
            s.circle(ex, by1, 2.4, JUNCTION[pot])
        for name, (px, py, _o, _p) in self.pins.items():
            if name.startswith("P"):
                s.circle(px, py, 14, WHITE)
                s.text_centered(px, py, name, 5)
            else:
                s.circle(px, py, 9.0, WHITE)
                s.text_centered(px, py, name, 5)
        s.text_centered(self.cx, 130, self.reference, 8)
        s.parts[-1].set("font", FONT_BOLD % 8)

    def legends(self):
        out = []
        for n, side in ((1, "L"), (5, "L"), (4, "R"), (8, "R")):
            px, py, _o, _p = self.pins["P%d" % n]
            text = dict(self.function)["P%d" % n].split(": ", 1)[1]
            out.append((side, py, px + (-1 if side == "L" else 1) * 24.5,
                        ["P%d: %s" % (n, text)], 6))
        for n in (1, 5):
            lx, ly = self.leds["L/A %d" % n]
            out.append(("L", ly, lx - 3.5,
                        ["LEDs L/A %d to %d" % (n, n + 3)], 6))
        px, py, _o, _p = self.pins["1"]
        out.append(("L", py, px - 9.6, ["1: +24 V DC", "2: 0 V", "3: PE"], 6))
        return out



HUBS = {"ek1100": EK1100(), "cu1128": CU1128()}


def uid(hub, *parts):
    return "{%s}" % uuid.uuid5(NS, "/".join((hub.stem,) + parts))


def header(hub, root, suffix, link="simple"):
    root.set("link_type", link)
    root.find("uuid").set("uuid", uid(hub, "view", suffix))
    names = root.find("names")
    for n in list(names):
        names.remove(n)
    for lang, text in zip(("en", "fr"), hub.names[suffix]):
        e = ET.SubElement(names, "name", {"lang": lang})
        e.text = text
    infos = root.find("elementInformations")
    for n in list(infos):
        infos.remove(n)
    for key, value in (("manufacturer", "Beckhoff"),
                       ("manufacturer_reference", hub.reference),
                       ("designation", hub.designation),
                       ("description", hub.description)):
        e = ET.SubElement(infos, "elementInformation",
                          {"name": key, "show": "1"})
        e.text = value


def terminals(hub, desc):
    for name, (x, y, o, pot) in hub.pins.items():
        e = ET.Element("terminal", {
            "x": num(x), "y": num(y), "name": name, "orientation": o,
            "type": "Generic", "uuid": uid(hub, "terminal", name)})
        if pot:
            e.set("potential", pot)
        desc.append(e)


def realistic(hub, template, path):
    root = ET.parse(template).getroot()
    header(hub, root, "_realistic")
    root.find("informations").text = (
        INFO + "\nRealistic view made by misc/beckhoff/gen_hubs.py: scale "
        "3.125 px per mm, grid 5 px. License: see "
        "http://qelectrotech.org/wiki/doc/elements_license")
    desc = root.find("description")
    dyn = desc.find("dynamic_text")
    for e in list(desc):
        desc.remove(e)
    d = Draw()
    hub.front(d)
    for e in d.parts:
        desc.append(e)
    terminals(hub, desc)
    if dyn is not None:
        dyn.set("uuid", uid(hub, "dynamic", "realistic"))
        dyn.set("x", num(hub.body[0]))
        desc.append(dyn)
    w, h, hx, hy = hub.root_size
    root.set("width", str(w))
    root.set("height", str(h))
    root.set("hotspot_x", str(hx))
    root.set("hotspot_y", str(hy))
    write(root, path)
    return root


def schematic(hub, real, path):
    root = copy.deepcopy(real)
    header(hub, root, "")
    root.find("informations").text = (
        INFO + "\nSchematic view made by misc/beckhoff/gen_hubs.py: the "
        "stages between the sockets are generic. License: see "
        "http://qelectrotech.org/wiki/doc/elements_license")
    s = Sheet()
    hub.diagram(s)
    old = root.find("description")
    desc = ET.Element("description")
    for e in s.parts:
        desc.append(e)
    terminals(hub, desc)
    dyn = old.find("dynamic_text")
    if dyn is not None:
        dyn.set("uuid", uid(hub, "dynamic", "schematic"))
        desc.append(dyn)
    idx = list(root).index(old)
    root.remove(old)
    root.insert(idx, desc)
    xs = [x for x, _ in s.bits] + [p[0] for p in hub.pins.values()]
    ys = [y for _, y in s.bits] + [p[1] for p in hub.pins.values()]
    x_min, y_min = min(xs) - 5, min(ys) - 5
    root.set("width", str(int(math.ceil(max(xs) + 5 - x_min))))
    root.set("height", str(int(math.ceil(max(ys) + 5 - y_min))))
    root.set("hotspot_x", str(int(math.ceil(-x_min))))
    root.set("hotspot_y", str(int(math.ceil(-y_min))))
    write(root, path)


def table(hub, template, path):
    with open(template, "r", encoding="utf-8") as f:
        text = f.read()
    names = "".join('\t\t<name lang="%s">%s</name>\n' % (lang, t)
                    for lang, t in zip(("en", "fr"), hub.names["_table"]))
    text = re.sub(r"<names>.*?</names>", "<names>\n" + names + "\t</names>",
                  text, count=1, flags=re.S)
    infos = "".join(
        '\t\t<elementInformation name="%s" show="1">%s</elementInformation>\n'
        % (k, v) for k, v in (("manufacturer", "Beckhoff"),
                              ("manufacturer_reference", hub.reference),
                              ("designation", hub.designation),
                              ("description", hub.description)))
    text = re.sub(r"<elementInformations>.*?</elementInformations>",
                  "<elementInformations>\n" + infos +
                  "\t</elementInformations>", text, count=1, flags=re.S)
    ios = "".join(
        '\t\t\t\t<plcIO type="entree_universelle" address="" '
        'functionText="%s" comment="" crossRef="" terminalCount="1">\n'
        "\t\t\t\t\t<terminal>%s</terminal>\n\t\t\t\t</plcIO>\n"
        % (label, name) for name, label in hub.function)
    text = re.sub(r"<plcIOs>.*?</plcIOs>",
                  lambda m: "<plcIOs>\n" + ios + "\t\t\t</plcIOs>", text,
                  count=1, flags=re.S)
    text = re.sub(r'<uuid uuid="[^"]*"/>',
                  '<uuid uuid="%s"/>' % uid(hub, "view", "_table"), text, 1)
    k = [0]

    def retag(m):
        k[0] += 1
        return 'uuid="%s"' % uid(hub, "dynamic", "table", str(k[0]))
    text = re.sub(r'(?<=text_from="ElementInfo" )uuid="[^"]*"', retag, text)
    text = re.sub(r"<informations>.*?</informations>",
                  lambda m: "<informations>" + INFO +
                  "\nTable made by misc/beckhoff/gen_hubs.py. "
                  "</informations>", text, count=1, flags=re.S)
    text = simple_tables.convert(text)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


def main(base):
    template = os.path.join(base, "el9xxx_system", "el9100",
                            "el9100_realistic.elmt")
    table_template = os.path.join(base, "el1xxx_digital_in", "el1004",
                                  "el1004_table.elmt")
    for hub in HUBS.values():
        folder = os.path.join(base, FAMILY, hub.stem)
        os.makedirs(folder, exist_ok=True)
        directory = os.path.join(folder, "qet_directory")
        if not os.path.exists(directory):
            with open(directory, "w", encoding="utf-8", newline="\n") as f:
                f.write('<qet-directory>\n\t<names>\n'
                        '\t\t<name lang="en">%s</name>\n'
                        '\t\t<name lang="fr">%s</name>\n'
                        '\t</names>\n</qet-directory>\n'
                        % (hub.reference, hub.reference))
        real = realistic(hub, template,
                         os.path.join(folder, hub.stem + "_realistic.elmt"))
        schematic(hub, real, os.path.join(folder, hub.stem + ".elmt"))
        table(hub, table_template,
              os.path.join(folder, hub.stem + "_table.elmt"))
        print("%s: schematic, realistic and table written" % hub.stem)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
