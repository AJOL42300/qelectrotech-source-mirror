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
"""Generate the Murrelektronik MICO and Mean Well DIN-rail libraries.

Every product is one element with four appearances (schematic, realistic,
description with legends and LEDs, pin table) in its own sub-folder. The
terminals have the same names and positions in the four appearances.
All the drawings are original vector drawings; only factual data (type
reference, ratings, terminal names, LEDs) comes from public datasheets.

Usage: python misc/murr_meanwell/gen_elements.py
"""
import json
import os
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from qetlib import (Canvas, Terminal, write_element, write_directory,
                    text_size, RED, BLUE, YELLOW, GREEN, BLACK)

REPO = os.path.dirname(os.path.dirname(os.path.dirname(
    os.path.abspath(__file__))))
EXTRA = os.path.join(REPO, "extra-elements")
MICO_DIR = "16_murrelektronik_mico"
PSU_DIR = "17_meanwell_psu"

VIEWS = [
    ("", "", ""),
    ("_realistic", ", realistic view", ", vue r\u00e9aliste"),
    ("_description", ", description view", ", vue descriptive"),
    ("_table", " - pin table", " - tableau des bornes"),
]
HEX_RED, HEX_BLUE = "#ff0000", "#0000ff"
AUTHOR = ("Author: QElectroTech contributors\nOriginal vector drawing, "
          "not copied from a manufacturer drawing. Ratings and terminal "
          "names from the public datasheet; layout and symbols are "
          "generic, check them against the datasheet.")


def round10(v):
    return int(round(v / 10.0)) * 10


# ---------------------------------------------------------------- legends
LEVEL = 9


def legend_vertical(c, items, x1, x2, ybase, bottom, size=5):
    """Leader lines from terminals going up (or down) and then to the side
    of the body. items: (x, text). The terminal closest to the centre gets
    the farthest level so that the leader lines never cross. Returns the
    number of levels used."""
    cx = (x1 + x2) / 2.0
    left = sorted([i for i in items if i[0] < cx])
    right = sorted([i for i in items if i[0] >= cx], reverse=True)
    sign = 1 if bottom else -1
    for side, lst in ((-1, left), (1, right)):
        for k, (x, text) in enumerate(lst):
            y = ybase + sign * (4 + LEVEL * (k + 1))
            xe = x1 - 6 if side < 0 else x2 + 6
            c.disc(x, ybase - sign * 4, 2, "black")
            c.line(x, ybase - sign * 4, x, y)
            c.line(x, y, xe, y)
            if side < 0:
                c.text_r(xe - 2, y - 3.8, text, size)
            else:
                c.text(xe + 2, y - 3.8, text, size)
    return max(len(left), len(right))


def legend_side(c, xfrom, y, text, side, x1, x2, size=5):
    xe = x1 - 6 if side < 0 else x2 + 6
    c.disc(xfrom, y, 2, "black")
    c.line(xfrom, y, xe, y)
    if side < 0:
        c.text_r(xe - 2, y - 3.8, text, size)
    else:
        c.text(xe + 2, y - 3.8, text, size)


def block(c, x, y, w, h, hole_y):
    """Spring clamp or screw terminal drawn as a small block."""
    c.rect(x - w / 2.0, y, w, h, "white")
    c.disc(x, hole_y, 4, "gray")


# ---------------------------------------------------------------- devices
class Device:
    manufacturer = ""
    ref = ""
    order = ""
    folder = ""
    family = ""
    root = ""
    short = ""
    short_fr = ""
    designation = ""
    description = ""
    W = H = 0
    terms = ()
    pins = ()

    @property
    def key(self):
        return self.manufacturer + "/" + self.ref

    def directory(self):
        return os.path.join(EXTRA, self.root, self.family, self.folder)

    def path(self, view):
        return os.path.join(self.directory(), self.folder + view + ".elmt")

    def infos(self):
        out = {"manufacturer": self.manufacturer,
               "manufacturer_reference": self.order or self.ref,
               "designation": self.designation}
        if self.description:
            out["description"] = self.description
        return out

    def folder_names(self):
        t = f"{self.manufacturer} {self.ref} ({self.short})"
        return {"en": t, "fr": f"{self.manufacturer} {self.ref} "
                f"({self.short_fr})", "de": t}

    def write(self):
        write_directory(self.directory(), self.folder_names())
        for view, en, fr in VIEWS:
            names = {"en": f"{self.manufacturer} {self.ref} "
                     f"{self.kind_en} {self.rating}" + en,
                     "fr": f"{self.kind_fr} {self.manufacturer} "
                     f"{self.ref} {self.rating}" + fr}
            key = self.key + "/" + (view or "schematic")
            canvas, ly = self.draw(view)
            write_element(self.path(view), key, names, self.infos(),
                          AUTHOR, canvas, self.terms,
                          table=(view == "_table"), label_at=(0, ly))

    def draw(self, view):
        if view == "":
            return self.schematic(), -24
        if view == "_realistic":
            return self.realistic(), -24
        if view == "_description":
            return self.description_view()
        return Canvas(), -24


# ------------------------------------------------------------------ PSU
class Psu(Device):
    manufacturer = "Mean Well"
    root = PSU_DIR
    kind_en = "DIN-rail power supply"
    kind_fr = "Alimentation rail DIN"

    def __init__(self, series, model, volts, watts, amps, dims, outs,
                 ins="LNPE", relay=False, share=False, led="DC OK"):
        """dims: (width, height, depth) in mm of the datasheet outline.
        outs: (number of +V terminals, number of -V terminals). ins: the
        input terminals (letters of L N PE, or L1 L2 PE for the WDR)."""
        self.series, self.ref, self.watts, self.amps = (series, model,
                                                         watts, amps)
        self.volts = volts
        self.mm, self.depth_mm = dims[0], dims[2]
        self.height_mm = dims[1]
        self.nplus, self.nminus = outs
        self.relay, self.share, self.led = relay, share, led
        self.ins = {"LNPE": ["L", "N", "PE"], "LN": ["L", "N"],
                    "L1L2PE": ["L1", "L2", "PE"]}[ins]
        self.pe = "PE" in self.ins
        self.family = series.lower()
        self.folder = model.lower().replace("-", "_")
        a = ("%g" % amps)
        v = ("%g" % volts)
        self.rating = f"{v} V {a} A"
        self.short = f"{v} V, {a} A, {watts:g} W"
        self.short_fr = self.short
        self.designation = (f"DIN-rail power supply, AC input, "
                            f"{v} V DC {a} A, {watts:g} W, "
                            f"{self.mm:g} mm wide")
        self.dims_text = (f"{self.mm:g}x{self.height_mm:g}x"
                          f"{self.depth_mm:g} mm (width x height x depth)")
        notes = [f"{led} LED and V ADJ output voltage trimmer on the front"
                 if led else ""]
        notes.append(f"{self.nplus} +V and {self.nminus} -V output "
                     "terminals as on the datasheet (parallel screws)")
        if relay:
            notes.append("DC OK relay contact on terminals 13 and 14 "
                         "(the datasheet numbers the two relay screws "
                         "differently; 13/14 is a convention)")
        if share:
            notes.append("current share terminals P- and P+ (control "
                         "connector)")
        notes.append("terminal order of the symbol is a convention, the "
                     "physical order depends on the series")
        self.description = "; ".join(n for n in notes if n)
        self.build_terminals()

    def build_terminals(self):
        base = max(round10(self.mm), 40)
        for p in (10 if base <= 80 else 20, 10):
            xs = self.positions(p)
            if 20 + max(x for _, x, _ in xs) <= base or p == 10:
                break
        self.pitch = p
        self.W = max(base, 20 + max(x for _, x, _ in xs))
        self.H = 90 if self.series == "HDR" else 120
        self.cols = (self.W - 20) // p + 1
        v = ("%g" % self.volts)
        t = []
        for name, x, top in xs:
            y = 0 if top else self.H
            if top:
                ident = {"L": "AC phase", "N": "AC neutral",
                         "L1": "AC line L1", "L2": "AC line L2",
                         "PE": "Protective earth (FG)"}[name]
                t.append(Terminal(name, x, y, "n", "entree_universelle",
                                  ident, ""))
            elif name[0] in "+-" and name[1] == "V":
                plus = name[0] == "+"
                t.append(Terminal(
                    name, x, y, "s", "sortie_digitale",
                    f"+{v} V DC output" if plus else "0 V DC output",
                    f"{v}V" if plus else "0V"))
            elif name in ("P-", "P+"):
                t.append(Terminal(name, x, y, "s", "sortie_digitale",
                                  "Current share " + name, ""))
            else:
                t.append(Terminal(name, x, y, "s", "sortie_digitale",
                                  "DC OK relay contact", ""))
        self.terms = t
        self.out_x = [tt.x for tt in t if tt.y and tt.name[1:2] == "V"]

    def positions(self, p):
        """(name, x, on top) of every terminal for a pitch p."""
        out = [(n, i * p, True) for i, n in enumerate(self.ins[:2])]
        names = ([f"+V{'' if i == 0 else i + 1}" for i in range(self.nplus)]
                 + [f"-V{'' if i == 0 else i + 1}"
                    for i in range(self.nminus)])
        last = (len(names) - 1) * p
        out += [(n, i * p, False) for i, n in enumerate(names)]
        x = last + 2 * p
        if self.share:
            out += [("P-", x, False), ("P+", x + p, False)]
            x += 2 * p
        if self.relay:
            out += [("13", x, False), ("14", x + p, False)]
        if self.pe:
            out.append(("PE", max(2 * p, last + p), True))
        return out

    def label(self, name):
        """The marking printed next to a terminal (+V2 is printed +V)."""
        return name[:2] if name[1:2] == "V" and len(name) > 2 else name

    def dc_right(self):
        return max(self.out_x + [self.pitch])

    def schematic(self):
        c = Canvas()
        W, H, p = self.W, self.H, self.pitch
        y0 = 30 if H >= 120 else 26
        y1 = y0 + (40 if H >= 120 else 34)
        right = self.dc_right() + 6
        c.rect(-10, 0, W, H, dash="dotted")
        c.rect(-6, y0, right + 6, y1 - y0)
        c.line(right, y0, -6, y1)
        c.text(-4, y0 + 1, "~", 6)
        c.text(right - 9, y1 - 9, "=", 6)
        for t in self.terms:
            if t.y == 0 and t.name != "PE":
                c.line(t.x, 0, t.x, y0)
        marked = set()
        for t in self.terms:
            if t.y and t.name[1:2] == "V":
                color = RED if t.name[0] == "+" else BLUE
                c.line(t.x, y1, t.x, H, color)
                if t.name[0] not in marked:
                    marked.add(t.name[0])
                    c.text(t.x + 3, H - 20, t.name[0], 5, True,
                           HEX_RED if t.name[0] == "+" else HEX_BLUE)
        if self.pe:
            xpe = next(t.x for t in self.terms if t.name == "PE")
            ye = y0 + 22
            c.line(xpe, 0, xpe, ye, YELLOW)
            c.line(xpe, 0, xpe, ye, GREEN, dash="dashed")
            for i, w in enumerate((12, 8, 4)):
                c.line(xpe - w / 2.0, ye + 4 * i, xpe + w / 2.0,
                       ye + 4 * i)
        if self.share:
            for n in ("P-", "P+"):
                x = self.pos(n)
                c.line(x, H, x, H - 12)
            c.text_c((self.pos("P-") + self.pos("P+")) / 2.0, H - 24,
                     "CS", 4)
        if self.relay:
            xa, xb = self.pos("13"), self.pos("14")
            c.line(xa, H, xa, H - 12)
            c.line(xa, H - 12, xb - 4, H - 28)
            c.line(xb, H, xb, H - 28)
            c.text_c((xa + xb) / 2.0, H - 44, "DC OK", 4)
        return c

    def pos(self, name):
        return next(t.x for t in self.terms if t.name == name)

    def panel(self):
        H = self.H
        bh = 20 if H >= 120 else 16
        return bh, bh + 12, H - bh - 12

    def realistic(self):
        c = Canvas()
        W, H, p = self.W, self.H, self.pitch
        bh, py0, py1 = self.panel()
        bw, size = p - 2, 4
        c.rect(-10, 0, W, H, "lightgray")
        for t in self.terms:
            top = t.y == 0
            y = 0 if top else H - bh
            block(c, t.x, y, bw, bh, y + bh * 0.45 if top
                  else y + bh * 0.55)
            c.text_c(t.x, bh + 1 if top else H - bh - 7.5,
                     self.label(t.name), size, True)
        px1, px2 = -6, W - 14
        cx = (px1 + px2) / 2.0
        c.rect(px1, py0, px2 - px1, py1 - py0, "white")
        tsize = 4 if px2 - px1 < 40 else 5
        c.text_c(cx, py0 + 2, self.ref, tsize, True)
        c.text_c(cx, py0 + 10, self.rating, 4)
        cy = py0 + (py1 - py0) * 0.65
        if self.led:
            c.disc(cx - 6, cy, 4, "green")
        c.disc(cx + 5, cy, 6, "white")
        c.line(cx + 2, cy + 1.5, cx + 8, cy - 1.5)
        self.front = (cx, cy, px1, px2)
        return c

    def legend_text(self, t):
        v = "%g" % self.volts
        name = t.name
        if t.y == 0:
            return {"L": "L: AC phase input", "N": "N: AC neutral input",
                    "L1": "L1: AC line input", "L2": "L2: AC line input",
                    "PE": "PE: protective earth (FG)"}[name]
        if name[0] == "+" and name[1:2] == "V":
            return f"{name}: +{v} V DC output"
        if name[0] == "-" and name[1:2] == "V":
            return f"{name}: 0 V DC output"
        if name in ("P-", "P+"):
            return f"{name}: current share"
        return f"{name}: DC OK relay contact"

    def description_view(self):
        c = self.realistic()
        H = self.H
        x1, x2 = -10, self.W - 10
        cx, cy = self.front[0], self.front[1]
        top = [(t.x, self.legend_text(t)) for t in self.terms if t.y == 0]
        bottom = [(t.x, self.legend_text(t)) for t in self.terms if t.y]
        nt = legend_vertical(c, top, x1, x2, 0, False)
        legend_vertical(c, bottom, x1, x2, H, True)
        if self.led:
            legend_side(c, cx - 8, cy, f"{self.led} LED (green)", -1,
                        x1, x2)
        legend_side(c, cx + 8, cy, "V ADJ: output voltage trimmer", 1,
                    x1, x2)
        return c, min(-24, -LEVEL * nt - 22)

    def layout_label(self, wpx, hpx):
        """Size, rotation and left of the reference text: horizontal and
        centred between the DIN rail edges when it fits the width,
        otherwise rotated by 270 degrees."""
        s = self.ref
        for size in (5, 4):
            w, h = text_size(s, size)
            if w <= wpx - 2 and h <= 15:
                return size, 0, (wpx - w) / 2.0
        for size in (5, 4, 3):
            w, h = text_size(s, size)
            if h <= wpx - 2 and w <= hpx - 22:
                return size, 270, (wpx - h) / 2.0
        raise ValueError("no room for the layout label of " + s)

    def layout(self):
        """Front view for a cabinet layout, 1 px = 2 mm, DIN rail behind
        the body and terminal screws at the top and at the bottom."""
        c = Canvas()
        w, h = self.mm / 2.0, self.height_mm / 2.0
        c.rect(0, 0, w, h, "none")
        strip = 8
        c.line(0, strip, w, strip)
        c.line(0, h - strip, w, h - strip)
        top = [t for t in self.terms if t.y == 0]
        bottom = [t for t in self.terms if t.y]
        d = min(4.0, w / max(len(top), len(bottom)) - 1.0)
        for row, y in ((top, strip / 2.0), (bottom, h - strip / 2.0)):
            for i in range(len(row)):
                c.disc((i + 0.5) * w / len(row), y, d, "gray")
        size, rot, tx = self.layout_label(w, h)
        rail_y = h / 2.0
        rail = (rail_y - 8.75, rail_y + 8.75)
        tw, th = text_size(self.ref, size)
        gap = None
        if rot:
            gap = (tx - 1, tx + th + 1)
            ty = h - strip - 3
        else:
            ty = rail_y - th / 2.0
        for y in rail:
            if gap:
                if gap[0] > 0:
                    c.line(0, y, gap[0], y, dash="dashed")
                if gap[1] < w:
                    c.line(gap[1], y, w, y, dash="dashed")
            else:
                c.line(0, y, w, y, dash="dashed")
        c.text(tx, ty, self.ref, size, True, rotation=rot)
        space = rail[0] - strip
        if not rot and space >= 8:
            cx, cy = w / 2.0, strip + space / 2.0
            if self.led:
                c.disc(cx - 5, cy, 3, "green")
            c.disc(cx + 4, cy, 4, "white")
        return c

    def layout_infos(self):
        return {"manufacturer": self.manufacturer,
                "designation": self.designation + " (layout)",
                "description": f"{self.manufacturer} {self.ref}, "
                               f"{self.dims_text}"}

    def write(self):
        super().write()
        names = {"en": f"{self.manufacturer} {self.ref} {self.kind_en} "
                       f"{self.rating}, layout (1 px = 2 mm)",
                 "fr": f"{self.kind_fr} {self.manufacturer} {self.ref} "
                       f"{self.rating}, implantation (1 px = 2 mm)"}
        write_element(self.path("_layout"), self.key + "/layout", names,
                      self.layout_infos(), AUTHOR, self.layout(), (),
                      label_at=(0, -14), master=False)

    def layout_entry(self):
        return {"manufacturer": self.manufacturer, "reference": self.ref,
                "width": self.mm, "height": self.height_mm,
                "depth": self.depth_mm}


PSUS = (
    [Psu("HDR", n, 24, w, a, (mm, 90, 54.5), outs,
         "LN", led="Power")
     for n, w, a, mm, outs in (("HDR-15-24", 15, 0.63, 17.5, (1, 1)),
                               ("HDR-30-24", 36, 1.5, 35, (1, 1)),
                               ("HDR-60-24", 60, 2.5, 52.5, (2, 2)),
                               ("HDR-100-24N", 92, 3.83, 70, (2, 2)),
                               ("HDR-150-24", 150, 6.25, 105, (2, 2)))]
    + [Psu("NDR", n, 24, w, a, (mm, 125.2, d), (2, 2))
       for n, w, a, mm, d in (("NDR-120-24", 120, 5, 40, 113.5),
                              ("NDR-240-24", 240, 10, 63, 113.5),
                              ("NDR-480-24", 480, 20, 85.5, 128.5))]
    + [Psu("SDR", "SDR-75-24", 24, 76.8, 3.2, (32, 125.2, 102), (2, 2))]
    + [Psu("SDR", "SDR-120-24", 24, 120, 5, (40, 125.2, 113.5), (1, 1),
           relay=True),
       Psu("SDR", "SDR-240-24", 24, 240, 10, (63, 125.2, 113.5), (2, 2),
           relay=True),
       Psu("SDR", "SDR-480-24", 24, 480, 20, (85.5, 125.2, 128.5), (2, 2),
           relay=True),
       Psu("SDR", "SDR-960-24", 24, 960, 40, (110, 125.2, 150), (3, 3),
           relay=True, share=True)]
    + [Psu("EDR", n, 24, w, a, (mm, 125.2, d), (2, 2))
       for n, w, a, mm, d in (("EDR-75-24", 76.8, 3.2, 32, 102),
                              ("EDR-120-24", 120, 5, 40, 113.5),
                              ("EDR-150-24", 156, 6.5, 40, 113.5))]
    + [Psu("DRP", n, 24, w, a, (mm, 125.2, 100), (2, 2))
       for n, w, a, mm in (("DRP-240-24", 240, 10, 125.5),
                           ("DRP-480-24", 480, 20, 227))]
    + [Psu("WDR", f"WDR-{n}-{v}", v, w, a, (mm, 125.2, d), outs,
           "L1L2PE", relay=True)
       for n, v, w, a, mm, d, outs in (
           (60, 5, 50, 10, 32, 102, (1, 1)),
           (60, 12, 60, 5, 32, 102, (1, 1)),
           (60, 24, 60, 2.5, 32, 102, (1, 1)),
           (60, 48, 60, 1.25, 32, 102, (1, 1)),
           (120, 12, 120, 10, 40, 113.5, (1, 1)),
           (120, 24, 120, 5, 40, 113.5, (1, 1)),
           (120, 48, 120, 2.5, 40, 113.5, (1, 1)),
           (240, 24, 240, 10, 63, 113.5, (2, 2)),
           (240, 48, 240, 5, 63, 113.5, (2, 2)),
           (480, 24, 480, 20, 85.5, 128.5, (2, 2)),
           (480, 48, 480, 10, 85.5, 128.5, (2, 2)))])

PSU_FAMILIES = {
    "hdr": ("HDR series (slim, 15-150 W)",
            "S\u00e9rie HDR (fine, 15-150 W)",
            "Serie HDR (schmal, 15-150 W)"),
    "ndr": ("NDR series (slim, 120-480 W)",
            "S\u00e9rie NDR (fine, 120-480 W)",
            "Serie NDR (schmal, 120-480 W)"),
    "sdr": ("SDR series (75-960 W)", "S\u00e9rie SDR (75-960 W)",
            "Serie SDR (75-960 W)"),
    "edr": ("EDR series (economy, 75-150 W)",
            "S\u00e9rie EDR (\u00e9conomique, 75-150 W)",
            "Serie EDR (Economy, 75-150 W)"),
    "drp": ("DRP series (with PFC, 240-480 W)",
            "S\u00e9rie DRP (avec PFC, 240-480 W)",
            "Serie DRP (mit PFC, 240-480 W)"),
    "wdr": ("WDR series (wide input 180-550 V AC, 60-480 W)",
            "S\u00e9rie WDR (entr\u00e9e large 180-550 V AC, 60-480 W)",
            "Serie WDR (Weitbereichseingang 180-550 V AC, 60-480 W)"),
}


# ----------------------------------------------------------------- MICO
class Mico(Device):
    manufacturer = "Murrelektronik"
    root = MICO_DIR
    kind_en = "MICO electronic circuit breaker"
    kind_fr = "Disjoncteur \u00e9lectronique MICO"

    def __init__(self, line, ref, order, channels, specs, rating):
        self.line, self.ref, self.order = line, ref, order
        self.channels, self.specs, self.rating = channels, specs, rating
        pro = line == "Pro"
        self.pro = pro
        self.family = "mico_" + line.lower()
        self.folder = ("mico_" + ref.lower().replace("mico ", "")
                       .replace(" ", "_").replace(".", "_"))
        self.xs = [20 * n + 10 for n in range(channels)]
        self.zs = [20 * n + 20 for n in range(channels)]
        last = (self.zs if pro else self.xs)[-1]
        self.W = max(60, last) + 20
        self.H = 110
        self.fixed = all("-" not in s for s in specs)
        self.short = f"{channels} ch., {rating}"
        self.short_fr = f"{channels} voies, {rating}"
        self.designation = (f"{channels}-channel electronic circuit "
                            f"breaker, 24 V DC, {rating}")
        notes = ["one status LED, one push button"
                 + ("" if self.fixed else " and one current selector")
                 + " per channel", "group alarm output"]
        if pro:
            notes.append("modular system (power module + channel "
                         "modules) drawn as one assembly; module layout "
                         "and LEDs assumed")
        else:
            notes.append("no separate 0 V output terminal per channel "
                         "(assumed)")
        if len(set(specs)) > 1:
            notes.append("channels 1-2 low range, 3-4 high range (assumed)")
        self.description = "; ".join(notes)
        ctl = "CTRL" if pro else "ON"
        t = [Terminal("+24V", 0, 0, "n", "entree_universelle",
                      "24 V DC supply", "24V"),
             Terminal("0V", 20, 0, "n", "entree_universelle",
                      "0 V supply", "0V"),
             Terminal(ctl, 40, 0, "n", "entree_digitale",
                      "On/restart input", ""),
             Terminal("ALARM", 60, 0, "n", "sortie_digitale",
                      "Group alarm output", "")]
        for n in range(channels):
            t.append(Terminal(f"OUT{n + 1}", self.xs[n], self.H, "s",
                              "sortie_digitale",
                              f"Channel {n + 1} output "
                              f"{specs[n]}", "24V"))
            if pro:
                t.append(Terminal(f"0V{n + 1}", self.zs[n], self.H, "s",
                                  "entree_universelle",
                                  f"Channel {n + 1} 0 V", "0V"))
        self.terms = t
        self.ctl = ctl

    def chx(self, n):
        return self.zs[n] - 5 if self.pro else self.xs[n]

    def schematic(self):
        c = Canvas()
        W, H = self.W, self.H
        last = self.xs[-1]
        c.rect(-10, 0, W, H, dash="dotted")
        c.line(0, 0, 0, 40, RED)
        c.line(0, 40, last, 40, RED)
        for x in self.xs:
            c.line(x, 40, x, 62, RED)
            c.rect(x - 6, 62, 12, 14)
            c.text_c(x, 65, "I>", 4)
            c.line(x, 76, x, H, RED)
            c.disc(x, 40, 3, "red", RED)
        if self.pro:
            c.line(20, 0, 20, H, BLUE)
            c.line(20, H - 14, self.zs[-1], H - 14, BLUE)
            for z in self.zs[1:]:
                c.line(z, H - 14, z, H, BLUE)
            for z in self.zs[:-1]:
                c.disc(z, H - 14, 3, "blue", BLUE)
        else:
            c.line(20, 0, 20, H - 14, BLUE)
            c.disc(20, H - 12, 4)
        c.line(40, 0, 40, 12)
        c.line(60, 0, 60, 12)
        c.rect(32, 12, 36, 16)
        c.text_c(50, 15.5, "CTRL", 5)
        return c

    def realistic(self):
        c = Canvas()
        W, H = self.W, self.H
        c.rect(-10, 0, W, H, "lightgray")
        for t in self.terms:
            top = t.y == 0
            narrow = self.pro and not top
            w = 8 if narrow else 16
            y = 0 if top else H - 14
            block(c, t.x, y, w, 14, y + 7)
            mark = t.name
            if narrow:
                mark = ("+" if t.name.startswith("OUT") else "-") + \
                    t.name[-1]
            c.text_c(t.x, 15.5 if top else H - 21, mark, 4, True)
        c.text(-6, 24, self.ref, 5, True)
        for n in range(self.channels):
            x = self.chx(n)
            c.disc(x, 40, 5, "green")
            c.disc(x, 52, 7, "white")
            if self.specs[n] and "-" in self.specs[n]:
                c.disc(x, 66, 8, "white")
                c.line(x - 3, 66, x + 3, 66)
            else:
                c.text_c(x, 62.5, self.specs[n].replace(" ", ""), 4, True)
        c.text_c((W - 20) / 2.0, 78, self.rating, 4)
        return c

    def description_view(self):
        c = self.realistic()
        H = self.H
        x1, x2 = -10, self.W - 10
        top = [(0, "+24V: 24 V DC supply"), (20, "0V: supply 0 V"),
               (40, "ON: switch on / restart input" if not self.pro else
                "CTRL: on/off input"),
               (60, "ALARM: group alarm output")]
        nt = legend_vertical(c, top, x1, x2, 0, False)
        bottom = [(self.xs[0], f"OUT1...OUT{self.channels}: channel "
                   "outputs")]
        if self.pro:
            bottom = [(self.xs[0], f"+1...+{self.channels}: OUT1..."
                       f"OUT{self.channels} channel outputs"),
                      (self.zs[0], f"-1...-{self.channels}: 0V1..."
                       f"0V{self.channels} channel 0 V")]
        legend_vertical(c, bottom, x1, x2, H, True)
        x = self.chx(0)
        legend_side(c, x - 3, 40, "LED per channel (green/red)", -1, x1, x2)
        legend_side(c, x - 4, 52, "Push button per channel", -1, x1, x2)
        if self.fixed:
            legend_side(c, x - 5, 63, "Fixed channel current", -1, x1, x2)
        else:
            legend_side(c, x - 5, 66, "Current selector per channel", -1,
                        x1, x2)
        return c, min(-24, -LEVEL * nt - 22)


MICOS = [
    Mico("Basic", "MICO Basic 2.4", "9000-41042-0100400", 2,
         ["1-4 A"] * 2, "2 x 1-4 A"),
    Mico("Basic", "MICO Basic 4.4", "9000-41034-0100400", 4,
         ["1-4 A"] * 4, "4 x 1-4 A"),
    Mico("Basic", "MICO Basic 4.4.10", "9000-41034-0101000", 4,
         ["1-4 A"] * 2 + ["4-10 A"] * 2, "2 x 1-4 A, 2 x 4-10 A"),
    Mico("Basic", "MICO Basic 8.6", "9000-41068-0600000", 8,
         ["6 A"] * 8, "8 x 6 A"),
    Mico("Pro", "MICO Pro 1 channel", "9000-41091-0101000", 1,
         ["1-10 A"], "1 x 1-10 A"),
    Mico("Pro", "MICO Pro 2 channels", "9000-41092-0101000", 2,
         ["1-10 A"] * 2, "2 x 1-10 A"),
    Mico("Pro", "MICO Pro 4 channels", "9000-41094-0101000", 4,
         ["1-10 A"] * 4, "4 x 1-10 A"),
]
MICO_FAMILIES = {
    "mico_basic": ("MICO Basic (2 to 8 channels)",
                   "MICO Basic (2 \u00e0 8 voies)",
                   "MICO Basic (2 bis 8 Kan\u00e4le)"),
    "mico_pro": ("MICO Pro (modular, 1 to 4 channels)",
                 "MICO Pro (modulaire, 1 \u00e0 4 voies)",
                 "MICO Pro (modular, 1 bis 4 Kan\u00e4le)"),
}
ROOTS = {
    MICO_DIR: ("Murrelektronik MICO electronic circuit breakers",
               "Disjoncteurs \u00e9lectroniques Murrelektronik MICO",
               "Murrelektronik MICO elektronische Sicherungsmodule"),
    PSU_DIR: ("Mean Well DIN-rail power supplies",
              "Alimentations rail DIN Mean Well",
              "Mean Well Hutschienen-Netzteile"),
}


def all_devices():
    return MICOS + PSUS


def main():
    for root in ROOTS:
        shutil.rmtree(os.path.join(EXTRA, root), ignore_errors=True)
    for root, n in ROOTS.items():
        write_directory(os.path.join(EXTRA, root),
                        dict(zip(("en", "fr", "de"), n)))
    for fam, n in MICO_FAMILIES.items():
        write_directory(os.path.join(EXTRA, MICO_DIR, fam),
                        dict(zip(("en", "fr", "de"), n)))
    for fam, n in PSU_FAMILIES.items():
        write_directory(os.path.join(EXTRA, PSU_DIR, fam),
                        dict(zip(("en", "fr", "de"), n)))
    for d in all_devices():
        d.write()
    with open(os.path.join(EXTRA, PSU_DIR, "devices_layout.json"), "w",
              encoding="utf-8", newline="\n") as f:
        json.dump({"devices": [d.layout_entry() for d in PSUS]}, f,
                  indent=2)
        f.write("\n")
    problems = 0
    for d in PSUS:
        for name, canvas in (("layout", d.layout()),
                             ("realistic", d.realistic())):
            for p in canvas.layout_problems():
                problems += 1
                print(f"{d.ref} {name}: {p}")
    print(f"{len(all_devices())} products, "
          f"{len(MICOS) * len(VIEWS) + len(PSUS) * (len(VIEWS) + 1)} "
          f"elements written, {problems} drawing problems")


if __name__ == "__main__":
    main()
