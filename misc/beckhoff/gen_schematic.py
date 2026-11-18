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
"""Write the schematic view (the symbol) of every card.

The schematic view of <card>.elmt is a wiring schematic drawn INSIDE the
dotted outline of the card (with the clips of the rail): the numbered
terminal circles are at the same places and at the same scale as in
<card>_realistic.elmt, so the views are interchangeable, and each terminal
leads to the generic stage that its role in the I/O table suggests:

  * input: resistor and LED (the light arrows mark the optocoupler);
  * output: driver stage;
  * supply and common terminals: rail marks and ground symbol.

The terminals (names, places, orientation) come from the realistic view,
the data (names, information, I/O table) are kept from the existing
<card>.elmt: only its drawing and its size are replaced.  The stages are
generic, not taken from a datasheet: see docs/beckhoff-ethercat.md.

Usage:
  gen_schematic.py <elements dir>   (e.g. extra-elements/15_beckhoff_ethercat)
"""
import glob
import math
import os
import sys
import uuid
import xml.etree.ElementTree as ET

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from realistic_fixes import box, num, text_width, write  # noqa: E402

NS = uuid.UUID("6f1c2a5e-8b0f-4d57-9a7e-5b1d0c8e2f10")
FONT = "Roboto Condensed,%d,-1,5,50,0,0,0,0,0,Regular"
FONT_BOLD = "Roboto Condensed,%d,-1,5,75,0,0,0,0,0,Bold"
THIN = "line-style:normal;line-weight:thin;filling:none;color:black"
NORMAL = "line-style:normal;line-weight:normal;filling:none;color:black"
DOTTED = "line-style:dotted;line-weight:normal;filling:none;color:black"
FILLED = "line-style:normal;line-weight:thin;filling:black;color:black"
WHITE = "line-style:normal;line-weight:thin;filling:white;color:black"
# +24 V in red, 0 V in blue
POT_STYLE = {"24V": "line-style:normal;line-weight:normal;filling:none;"
                    "color:red",
             "0V": "line-style:normal;line-weight:normal;filling:none;"
                   "color:blue"}
POT_FILLED = {"24V": "line-style:normal;line-weight:thin;filling:red;"
                     "color:red",
              "0V": "line-style:normal;line-weight:thin;filling:blue;"
                    "color:blue"}
POT_WHITE = {"24V": "line-style:normal;line-weight:normal;filling:white;"
                    "color:red",
             "0V": "line-style:normal;line-weight:normal;filling:white;"
                   "color:blue"}
POT_TEXT = {"24V": "#ff0000", "0V": "#0000ff"}
# the yellow of the TwinSAFE housings, as in the realistic views
YELLOW_FILL = "line-style:none;line-weight:thin;filling:yellow;color:black"
NOTE = ("Schematic view made by misc/beckhoff/gen_schematic.py from the "
        "realistic view: the stages are generic, check them against the "
        "datasheet. License: see "
        "http://qelectrotech.org/wiki/doc/elements_license")

EL60 = ("el1004", "el1008", "el2004", "el2008", "el2522", "el2574", "el5152",
        "el9100", "el1904", "el2904", "el2911", "el1xxx_generic",
        "el2xxx_generic", "el3xxx_generic", "el4xxx_generic", "el2652")
# body of the 60 px wide terminals (x0, y0, x1, y1) and of the second body of
# the EL2911 drawing
EL60_BODY = (-14.5, -136.7, 22.0, 140.5)
EL2911_BODY2 = (45.5, -136.7, 81.5, 140.5)
# EK1914: generic assumption of the channel on each terminal (the symbol
# named the terminals after the channels); check it in the datasheet
EK1914_PINS = {"I1": "2", "I2": "10", "I3": "3", "I4": "11", "O1": "4",
               "O2": "12", "O3": "5", "O4": "13", "SI1": "7", "SI2": "15",
               "SO1": "8", "SO2": "16"}
CLIP_W = 16.0
CLIP_H = 7.0
SAFETY = ("el1904", "el2904", "el2911", "el1918", "ek1914")


def uid(*parts):
    return "{%s}" % uuid.uuid5(NS, "/".join(parts))


class Frame:
    """A segment from (ax, ay) along (dx, dy); v is the perpendicular."""

    def __init__(self, ax, ay, dx, dy):
        self.ax, self.ay, self.dx, self.dy = ax, ay, dx, dy
        self.px, self.py = -dy, dx

    def p(self, u, v=0.0):
        return (self.ax + u * self.dx + v * self.px,
                self.ay + u * self.dy + v * self.py)


class Sheet:
    """The new drawing, with the helpers of the stages."""

    def __init__(self):
        self.parts = []
        self.bits = []      # bounding box points

    def _b(self, *xy):
        for i in range(0, len(xy), 2):
            self.bits.append((xy[i], xy[i + 1]))

    def line(self, x1, y1, x2, y2, style=THIN, end2="none"):
        self._b(x1, y1, x2, y2)
        self.parts.append(ET.Element("line", {
            "x1": num(x1), "y1": num(y1), "x2": num(x2), "y2": num(y2),
            "end1": "none", "end2": end2, "length1": "1.5",
            "length2": "3", "style": style, "antialias": "false"}))

    def poly(self, pts, style=THIN, closed=True):
        a = {"closed": "true" if closed else "false", "style": style,
             "antialias": "false"}
        for i, (x, y) in enumerate(pts, 1):
            a["x%d" % i], a["y%d" % i] = num(x), num(y)
            self._b(x, y)
        self.parts.append(ET.Element("polygon", a))

    def rect(self, x, y, w, h, style=THIN):
        self._b(x, y, x + w, y + h)
        self.parts.append(ET.Element("rect", {
            "x": num(x), "y": num(y), "width": num(w), "height": num(h),
            "rx": "0", "ry": "0", "style": style, "antialias": "false"}))

    def circle(self, cx, cy, d, style=THIN):
        self._b(cx - d / 2, cy - d / 2, cx + d / 2, cy + d / 2)
        self.parts.append(ET.Element("circle", {
            "x": num(cx - d / 2), "y": num(cy - d / 2), "diameter": num(d),
            "style": style, "antialias": "false"}))

    def text(self, x, y, s, size=5, bold=False, color="#000000"):
        self._b(x, y)
        self.parts.append(ET.Element("text", {
            "x": num(x), "y": num(y), "rotation": "0", "color": color,
            "font": (FONT_BOLD if bold else FONT) % size, "text": s}))

    def text_centered(self, cx, cy, s, size=5):
        w = text_width(s) * size / 7.0
        self.text(cx - w / 2, cy - (size * 4 / 3.0 + 1) / 2 - 0.5, s, size)

    def power_rails(self, x0, x1, y24, y0):
        """+24 V (red) and 0 V (blue) of the power contacts, which run
        through the housing: a filled square at the left edge (blade) and
        a circle at the right edge (spring), a rail between them and its
        legend."""
        for y, pot, label in ((y24, "24V", "+24 V"), (y0, "0V", "0 V")):
            self.rect(x0 - 2.5, y - 2.5, 5, 5, POT_FILLED[pot])
            self.line(x0 + 2.5, y, x1 - 2.5, y, POT_STYLE[pot])
            self.circle(x1, y, 5, POT_WHITE[pot])
            self.text(x0 + 4, y - 8, label, 4, color=POT_TEXT[pot])

    def junction(self, x, y, pot=""):
        self.circle(x, y, 2.4, POT_FILLED.get(pot, FILLED))

    # stages -------------------------------------------------------------
    def wire(self, f, u0, u1, v=0.0):
        self.line(*f.p(u0, v), *f.p(u1, v))

    def resistor(self, f, u, length=9.0, width=4.0):
        pts = [f.p(u, -width / 2), f.p(u + length, -width / 2),
               f.p(u + length, width / 2), f.p(u, width / 2)]
        self.poly(pts)

    def led(self, f, u, length=7.0):
        """Triangle pointing along the segment, bar at the end, arrows."""
        tri = [f.p(u, -3), f.p(u, 3), f.p(u + length - 1.5, 0)]
        self.poly(tri)
        self.line(*f.p(u + length - 1.5, -3), *f.p(u + length - 1.5, 3))
        for dv in (0.0, 2.8):
            a = f.p(u + 3.2 - dv * 0.0, 3.8 + dv)
            b = f.p(u + 0.2, 6.3 + dv)
            self.line(*a, *b, end2="simple")

    def driver(self, f, u, length=11.0, width=6.0):
        self.rect_f(f, u, length, width)
        self.line(*f.p(u + length - 2.5, 0), *f.p(u + 2.5, 0),
                  end2="simple")

    def rect_f(self, f, u, length, width):
        self.poly([f.p(u, -width / 2), f.p(u + length, -width / 2),
                   f.p(u + length, width / 2), f.p(u, width / 2)])

    def bar(self, f, u, width=7.0):
        self.line(*f.p(u, -width / 2), *f.p(u, width / 2),
                  style=NORMAL)

    def dot(self, f, u, d=2.4):
        x, y = f.p(u)
        self.circle(x, y, d, FILLED)

    def ground(self, f, u):
        for i, half in enumerate((4.0, 2.7, 1.4)):
            self.line(*f.p(u + 2.0 * i, -half), *f.p(u + 2.0 * i, half))

    def stage(self, f, kind, length):
        """A stage of the given kind from u = 0 to u = length."""
        if kind == "in":
            if length >= 26:
                free = (length - 16.0) / 3.0
                self.wire(f, 0, free)
                self.resistor(f, free)
                self.wire(f, free + 9.0, 2 * free + 9.0)
                self.led(f, 2 * free + 9.0)
                self.wire(f, 2 * free + 16.0, length)
            elif length >= 13:
                free = (length - 7.0) / 2.0
                self.wire(f, 0, free)
                self.led(f, free)
                self.wire(f, free + 7.0, length)
            else:
                self.wire(f, 0, length)
        elif kind == "out":
            if length >= 17:
                free = (length - 11.0) / 2.0
                self.wire(f, 0, free)
                self.driver(f, free)
                self.wire(f, free + 11.0, length)
            else:
                self.wire(f, 0, length)
        elif kind == "gnd":
            self.wire(f, 0, max(length - 6.0, 1.0))
            self.ground(f, max(length - 6.0, 1.0))
        else:
            self.wire(f, 0, length)


# -- reading the realistic view ---------------------------------------------
class Pin:
    def __init__(self, e):
        self.name = e.get("name")
        self.x = float(e.get("x"))
        self.y = float(e.get("y"))
        self.orientation = e.get("orientation")
        self.pot = e.get("potential") or ""
        self.role = "other"
        self.io = None
        self.d = 9.0


def role_of(pin):
    if pin.name.startswith("EtherCAT") or " X5" in pin.name:
        return "eth"
    if pin.io is not None:
        return "in" if pin.io.get("type").startswith("entree") else "out"
    if pin.pot == "0V":
        return "gnd"
    if pin.pot == "24V":
        return "sup"
    return "other"


class Card:
    def __init__(self, sym_path):
        self.path = sym_path
        self.stem = os.path.basename(sym_path)[:-len(".elmt")]
        self.real_path = sym_path[:-len(".elmt")] + "_realistic.elmt"
        self.sym = ET.parse(sym_path).getroot()
        self.real = ET.parse(self.real_path).getroot()
        rdesc = self.real.find("description")
        self.pins = [Pin(e) for e in rdesc.findall("terminal")]
        ios = {}
        for io in self.sym.iter("plcIO"):
            for t in io.findall("terminal"):
                ios.setdefault(t.text, io)
        for p in self.pins:
            p.io = ios.get(p.name)
            p.role = role_of(p)
        self.texts = rdesc.findall("text")
        self.dyn = rdesc.find("dynamic_text")
        self.sheet = Sheet()
        self.outlines = []      # (x0, y0, x1, y1)

    def remap(self, table):
        """Rename the terminals of the I/O table to the pins of the view."""
        ios = {}
        for io in self.sym.iter("plcIO"):
            for t in io.findall("terminal"):
                if t.text in table:
                    t.text = table[t.text]
                ios.setdefault(t.text, io)
        for p in self.pins:
            p.io = ios.get(p.name)
            p.role = role_of(p)

    # geometry ---------------------------------------------------------------
    def free(self, pin, dx, dy, margin=3.0):
        """Free length from the circle of the pin in the direction."""
        best = 1e9
        r = pin.d / 2
        for o in self.pins:
            if o is pin:
                continue
            ox, oy = o.x - pin.x, o.y - pin.y
            along = ox * dx + oy * dy
            across = abs(-ox * dy + oy * dx)
            if along > 0 and across < (pin.d + o.d) / 2 + 1.0:
                best = min(best, along - o.d / 2 - r - 1.5)
        for x0, y0, x1, y1 in self.outlines:
            if x0 <= pin.x <= x1 and y0 <= pin.y <= y1:
                if dx > 0:
                    d = x1 - pin.x
                elif dx < 0:
                    d = pin.x - x0
                elif dy > 0:
                    d = y1 - pin.y
                else:
                    d = pin.y - y0
                best = min(best, d - r - margin)
        return best

    def circle_of(self, pin):
        self.sheet.circle(pin.x, pin.y, pin.d, WHITE)
        size = 5 if len(pin.name) < 3 and pin.d >= 8.5 else 4
        if pin.name.isdigit():
            self.sheet.text_centered(pin.x, pin.y, pin.name, size)

    def stage_from(self, pin, dx, dy, kind, length):
        r = pin.d / 2
        f = Frame(pin.x + dx * r, pin.y + dy * r, dx, dy)
        self.sheet.stage(f, kind, length)
        return f

    def stub(self, pin, prefer=("up", "down", "left", "right"), cap=30.0):
        """Single ended stage in the first direction that has some room."""
        dirs = {"up": (0, -1), "down": (0, 1), "left": (-1, 0),
                "right": (1, 0)}
        kind = pin.role if pin.role in ("in", "out", "gnd") else "other"
        need = {"in": 13.0, "out": 17.0, "gnd": 9.0}.get(kind, 5.0)
        best = None
        for name in prefer:
            dx, dy = dirs[name]
            room = self.free(pin, dx, dy)
            if room >= need:
                best = (dx, dy, room)
                break
            if best is None or room > best[2]:
                best = (dx, dy, room)
        dx, dy, room = best
        if room < 5.0:
            return
        length = min(cap, room)
        f = self.stage_from(pin, dx, dy, kind, length)
        if kind == "other" or kind == "in" or kind == "out":
            self.sheet.bar(f, length)
        if pin.role == "sup":
            self.sheet.bar(f, length)

    def pair(self, upper, lower, kind=None):
        gap = lower.y - upper.y
        length = gap - (upper.d + lower.d) / 2
        f = self.stage_from(upper, 0, 1, kind or upper.role, length)
        return f

    def power(self, body):
        """The power contacts of a housing: +24 V and 0 V rails from edge
        to edge, and a branch along the left and the right side for the
        terminals that are on 24 V or 0 V. Returns the linked pins."""
        x0, y0, x1, y1 = body
        rails = {"24V": round(y0 + 14), "0V": round(y0 + 26)}
        self.sheet.power_rails(x0, x1, rails["24V"], rails["0V"])
        branch_x = {"left": x0 + 2.5, "right": x1 - 2.5}
        linked = set()
        inside = [p for p in self.pins
                  if p.role in ("gnd", "sup") and x0 <= p.x <= x1]
        mid = (min(p.x for p in inside) + max(p.x for p in inside)) / 2 \
            if inside else (x0 + x1) / 2
        by_side = {"left": [p for p in inside if p.x < mid],
                   "right": [p for p in inside if p.x >= mid]}
        # one potential per side (the housing edge has room for one
        # branch): 24 V on the left, 0 V on the right when a side carries
        # both; the other pins are joined to the pin of the same potential
        # in the same row
        wanted = {"left": "24V", "right": "0V"}
        for side, pins in by_side.items():
            if not pins:
                continue
            pots = {p.pot for p in pins}
            pot = wanted[side] if len(pots) > 1 else pins[0].pot
            kept = [p for p in pins if p.pot == pot]
            bx, top = branch_x[side], rails[pot]
            style = POT_STYLE[pot]
            self.sheet.line(bx, top, bx, max(p.y for p in kept), style)
            self.sheet.junction(bx, top, pot)
            for p in kept:
                edge = p.x + (-1 if side == "left" else 1) * p.d / 2
                self.sheet.line(edge, p.y, bx, p.y, style)
                self.sheet.junction(bx, p.y, pot)
                linked.add(p.name)
            for p in pins:
                if p in kept:
                    continue
                mate = [o for o in inside if o.pot == p.pot and
                        abs(o.y - p.y) < 1 and o is not p]
                if not mate:
                    continue
                o = mate[0]
                sign = 1 if o.x > p.x else -1
                self.sheet.line(p.x + sign * p.d / 2, p.y,
                                o.x - sign * o.d / 2, p.y, POT_STYLE[p.pot])
                linked.add(p.name)
        return linked

    # outline and clips --------------------------------------------------------
    def outline(self, x0, y0, x1, y1, clips=True):
        self.outlines.append((x0, y0, x1, y1))
        s = self.sheet
        if self.stem in SAFETY:
            s.rect(x0, y0, x1 - x0, y1 - y0, YELLOW_FILL)
        s.rect(x0, y0, x1 - x0, y1 - y0, DOTTED)
        if clips:
            cx = (x0 + x1) / 2 - 2.0
            for top in (True, False):
                y = y0 - CLIP_H if top else y1
                s.rect(cx - CLIP_W / 2, y, CLIP_W, CLIP_H, THIN)
                inner = y + (2.0 if top else CLIP_H - 2.0)
                s.line(cx - CLIP_W / 2 + 3, inner, cx + CLIP_W / 2 - 3,
                       inner)

    # copying the texts of the realistic view ----------------------------------
    def copy_reference(self, labels=False):
        for t in self.texts:
            bold = ",75," in t.get("font")
            if bold or labels:
                self.sheet.parts.append(ET.fromstring(ET.tostring(t)))
                self.sheet._b(float(t.get("x")), float(t.get("y")))

    # builders -------------------------------------------------------------------
    def build_el60(self):
        bodies = [EL60_BODY]
        if self.stem == "el2911":
            bodies.append(EL2911_BODY2)
        linked = set()
        for b in bodies:
            self.outline(*b)
            linked |= self.power(b)
        paired = set()
        if self.stem == "el2911":
            # clock output (S+) above its safe input (SI), as in the manual
            by_name = {p.name: p for p in self.pins}
            for a, b in (("1", "2"), ("3", "4"), ("5", "6"), ("7", "8")):
                self.pair(by_name[a], by_name[b], "in")
                paired.update((a, b))
        cols = {}
        for p in self.pins:
            cols.setdefault(p.x, []).append(p)
        for col in cols.values():
            col.sort(key=lambda p: p.y)
            for upper, lower in zip(col, col[1:]):
                if upper.name in paired or upper.role not in ("in", "out"):
                    continue
                gap = lower.y - upper.y
                if gap < 30 or lower.role in ("in", "sup", "eth"):
                    continue
                if lower.role == "out" and upper.role != "out":
                    continue
                self.pair(upper, lower)
                paired.add(upper.name)
                paired.add(lower.name)
        for p in self.pins:
            self.circle_of(p)
        for p in self.pins:
            if p.name in paired or p.name in linked:
                continue
            if p.role in ("in", "out"):
                self.stub(p, ("up", "down"), 22.0)
        # the labels of the channels sit above their circle, as in the
        # realistic view, when the stage does not use that room
        for t in self.texts:
            if ",75," in t.get("font"):
                continue
            ty = float(t.get("y"))
            tx = float(t.get("x")) + text_width(t.get("text")) / 2
            for p in self.pins:
                if self.stem == "el2911" and p.name in paired and \
                        p.role == "in":
                    continue    # the stage sits where the label would be
                if p.name in paired and p.role in ("in", "out") and \
                        abs(tx - p.x) < 12 and -16 < ty - p.y < -6:
                    self.sheet.parts.append(ET.fromstring(ET.tostring(t)))
        self.copy_reference()

    def build_el1918(self):
        """16 terminals in the housing of the other cards: S+ (test pulse)
        above its input SI, the stage between them."""
        self.outline(*EL60_BODY)
        self.power(EL60_BODY)
        for p in self.pins:
            p.d = 9.0
            self.circle_of(p)
        by_x = {}
        for p in self.pins:
            by_x.setdefault(p.x, []).append(p)
        for col in by_x.values():
            col.sort(key=lambda p: p.y)
            for upper, lower in zip(col[::2], col[1::2]):
                self.pair(upper, lower, "in")
        self.copy_reference()

    def build_rows(self, left_x, body):
        """EK1914: two columns of terminals, one stage each:
        the left column leads to the left, the right one to the right."""
        self.outline(*body)
        linked = self.power(body)
        eth = [p for p in self.pins if p.role == "eth"]
        for p in self.pins:
            p.d = 8.5
        for p in self.pins:
            if p.role == "eth":
                s = self.sheet
                s.rect(p.x - 9, p.y - 5, 18, 10, WHITE)
                s.text_centered(p.x, p.y, p.name.split()[-1], 5)
            else:
                self.circle_of(p)
        if len(eth) == 2:
            a, b = sorted(eth, key=lambda p: p.y)
            self.sheet.line(a.x, a.y + 5, b.x, b.y - 5, DOTTED)
        for p in self.pins:
            if p.role == "eth" or p.name in linked:
                continue
            side = ("left", "right") if p.x <= left_x else ("right", "left")
            self.stub(p, side, 22.0)
        self.copy_reference()


def make_description(card):
    desc = ET.Element("description")
    for e in card.sheet.parts:
        desc.append(e)
    xs = [x for x, _ in card.sheet.bits] + [p.x for p in card.pins]
    ys = [y for _, y in card.sheet.bits] + [p.y for p in card.pins]
    for p in card.pins:
        e = ET.Element("terminal", {
            "x": num(p.x), "y": num(p.y), "name": p.name,
            "orientation": p.orientation, "type": "Generic",
            "uuid": card.uuids.get(p.name) or uid(card.stem, "schematic",
                                                  p.name)})
        if p.pot:
            e.set("potential", p.pot)
        desc.append(e)
    if card.dyn is not None:
        d = ET.fromstring(ET.tostring(card.dyn))
        d.set("uuid", uid(card.stem, "schematic", "label"))
        desc.append(d)
    return desc


def run(card):
    old = card.sym.find("description")
    card.uuids = {}
    for t in old.findall("terminal"):
        card.uuids[t.get("name")] = t.get("uuid")
    if card.stem in EL60:
        card.build_el60()
    elif card.stem == "ek1914":
        card.remap(EK1914_PINS)
        card.build_rows(20.0, (-48.7, -136.7, 85.0, 140.5))
    elif card.stem == "el1918":
        card.build_el1918()
    else:
        return False
    new = make_description(card)
    idx = list(card.sym).index(old)
    card.sym.remove(old)
    card.sym.insert(idx, new)
    # same origin as the realistic view; the size follows the drawing
    xs = [x for x, _ in card.sheet.bits] + [p.x for p in card.pins]
    ys = [y for _, y in card.sheet.bits] + [p.y for p in card.pins]
    x0, y0 = min(xs) - 5, min(ys) - 5
    card.sym.set("width", str(int(math.ceil(max(xs) + 5 - x0))))
    card.sym.set("height", str(int(math.ceil(max(ys) + 5 - y0))))
    card.sym.set("hotspot_x", str(int(math.ceil(-x0))))
    card.sym.set("hotspot_y", str(int(math.ceil(-y0))))
    info = card.sym.find("informations")
    if info is not None and "gen_schematic" not in (info.text or ""):
        info.text = (info.text or "").rstrip("\n") + "\n" + NOTE
    write(card.sym, card.path)
    return True


def drive_schematic(path):
    """Power stage inside the housing of a drive symbol (AF1000, AX5000):
    mains, rectifier, DC link, inverter and the motor terminals. The pins
    and their texts of the symbol are kept; the stage is generic."""
    root = ET.parse(path).getroot()
    desc = root.find("description")
    if any("dotted" in e.get("style", "") for e in desc):
        return False
    pins = {e.get("name"): (float(e.get("x")), float(e.get("y")))
            for e in desc.findall("terminal")}
    mains = [y for n, (x, y) in pins.items()
             if n in ("L1", "L2", "L3", "L3N")]
    motor = [y for n, (x, y) in pins.items()
             if n in ("M1-U", "M1-V", "M1-W")]
    if not mains or not motor:
        return False
    for e in desc.findall("rect"):
        if float(e.get("width")) >= 150:    # the housing
            e.set("style", DOTTED)
    s = Sheet()
    rect_y0, rect_y1 = min(mains) - 10, max(mains) + 10
    inv_y0, inv_y1 = min(motor) - 10, max(motor) + 10
    ymid = (rect_y0 + rect_y1) / 2
    s.rect(-25, rect_y0, 30, rect_y1 - rect_y0)
    s.line(-25, rect_y1, 5, rect_y0)
    s.text(-22, rect_y0 + 3, "~", 7)
    s.text(-4, rect_y1 - 14, "=", 7)
    s.rect(20, inv_y0, 30, inv_y1 - inv_y0)
    s.line(20, inv_y1, 50, inv_y0)
    s.text(23, inv_y0 + 3, "=", 7)
    s.text(40, inv_y1 - 14, "~", 7)
    for x_dc, dy in ((26, -4), (34, 4)):
        s.poly([(5, ymid + dy), (x_dc, ymid + dy), (x_dc, inv_y1)],
               closed=False)
    for y in motor:
        s.line(50, y, 56, y)
        s.dot(Frame(56, y, 1, 0), 0)
    s.text(-22, rect_y0 - 11, "AC", 5)
    s.text(23, inv_y0 - 11, "U V W", 5)
    for e in s.parts:
        desc.insert(1, e)
    xs = [x for x, _ in s.bits]
    ys = [y for _, y in s.bits]
    info = root.find("informations")
    if info is not None and "gen_schematic" not in (info.text or ""):
        info.text = (info.text or "").rstrip("\n") + "\n" + NOTE
    write(root, path)
    return True


def main():
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    base = sys.argv[1]
    for sym in sorted(glob.glob(os.path.join(base, "drives_*", "a[fx]*.elmt"))):
        if sym.endswith(("_realistic.elmt", "_description.elmt",
                         "_table.elmt")):
            continue
        if drive_schematic(sym):
            print("schematic", os.path.relpath(sym, base))
    for real in sorted(glob.glob(os.path.join(base, "*",
                                              "*_realistic.elmt"))):
        sym = real[:-len("_realistic.elmt")] + ".elmt"
        if not os.path.exists(sym):
            continue
        card = Card(sym)
        if run(card):
            print("schematic", os.path.relpath(sym, base))


if __name__ == "__main__":
    main()
