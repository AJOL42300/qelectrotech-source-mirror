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
"""Generate the Kinco library (KS105 range and HMI Green range).

Usage: python misc/kinco/gen_kinco.py

Every product of kinco_data.py gives four elements (schematic symbol,
realistic view, description view, I/O table) and its layout footprints. All
drawings are vector and redrawn here, on a grid of 10 px for the terminals.
The files are written to extra-elements/.../kinco, always the same way
(uuid5 of fixed names), so a regeneration changes nothing.
"""
import math
import os
import uuid

import kinco_data as D

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
CATEGORY = "10_electric/20_manufacturers_articles/kinco"
OUT = os.path.join(REPO, "extra-elements", *CATEGORY.split("/"))
NS = uuid.UUID("0c1f7a52-3b8e-5d1c-9a47-6e2b5f0d8c31")

FONT = "Roboto Condensed,%d,-1,5,50,0,0,0,0,0,Regular"
FONT_BOLD = "Roboto Condensed,%d,-1,5,75,0,0,0,0,0,Bold"
LABEL_FONT = "Liberation Sans,9,-1,5,50,0,0,0,0,0,Regular"

ROLE_COLOR = {"24v": "red", "l+": "red", "0v": "blue", "l-": "blue",
              "pe": "orange"}

VIEWS = {
    "schema": ("", "", ""),
    "realistic": ("_realistic", ", realistic view", ", vue réaliste"),
    "description": ("_description", ", description view",
                    ", vue description"),
    "table": ("_table", " - I/O table", " - tableau d'E/S"),
}

LICENSE = "License: see http://qelectrotech.org/wiki/doc/elements_license"


def uid(*parts):
    return "{" + str(uuid.uuid5(NS, "/".join(parts))) + "}"


def q(text):
    text = str(text).replace("&", "&amp;").replace("<", "&lt;")
    return '"' + text.replace('"', "&quot;") + '"'


def fmt(v):
    s = ("%.2f" % v).rstrip("0").rstrip(".")
    return "0" if s in ("", "-0") else s


def ceil10(v):
    return int(math.ceil(v / 10.0)) * 10


def tw(text, size):
    """Estimated width of a text in px."""
    narrow = set("iIl.1,'|:;! ")
    wide = set("mwMWO@")
    total = 0.0
    for c in text:
        total += 0.35 if c in narrow else (0.7 if c in wide else 0.5)
    return total * size


def style(fill="none", color="black", weight="normal", line="normal"):
    return (f"line-style:{line};line-weight:{weight};filling:{fill};"
            f"color:{color}")


class Builder:
    """Collects the shapes and terminals of one element."""

    def __init__(self, key):
        self.key = key
        self.shapes = []
        self.terminals = []
        self.dynamic = []
        self.bb = [0.0, 0.0, 0.0, 0.0]
        self.nt = 0

    def ext(self, x0, y0, x1, y1):
        b = self.bb
        b[0], b[1] = min(b[0], x0), min(b[1], y0)
        b[2], b[3] = max(b[2], x1), max(b[3], y1)

    def rect(self, x, y, w, h, fill="none", color="black", weight="normal",
             line="normal"):
        self.ext(x, y, x + w, y + h)
        self.shapes.append(
            f'<rect x={q(fmt(x))} y={q(fmt(y))} width={q(fmt(w))} '
            f'height={q(fmt(h))} rx="0" ry="0" antialias="false" '
            f'style={q(style(fill, color, weight, line))} />')

    def circle(self, cx, cy, d, fill="none", color="black", weight="normal"):
        self.ext(cx - d / 2, cy - d / 2, cx + d / 2, cy + d / 2)
        self.shapes.append(
            f'<circle x={q(fmt(cx - d / 2))} y={q(fmt(cy - d / 2))} '
            f'diameter={q(fmt(d))} antialias="false" '
            f'style={q(style(fill, color, weight))} />')

    def line(self, x1, y1, x2, y2, color="black", weight="normal",
             line="normal"):
        self.ext(min(x1, x2), min(y1, y2), max(x1, x2), max(y1, y2))
        self.shapes.append(
            f'<line x1={q(fmt(x1))} y1={q(fmt(y1))} x2={q(fmt(x2))} '
            f'y2={q(fmt(y2))} end1="none" end2="none" length1="1.5" '
            f'length2="3" antialias="false" '
            f'style={q(style("none", color, weight, line))} />')

    def text(self, x, y, s, size=6, bold=False, align="l"):
        w = tw(s, size)
        if align == "c":
            x -= w / 2
        elif align == "r":
            x -= w
        self.ext(x, y, x + w, y + size * 1.7)
        font = (FONT_BOLD if bold else FONT) % size
        self.shapes.append(
            f'<text x={q(fmt(x))} y={q(fmt(y))} font={q(font)} '
            f'rotation="0" color="#000000" text={q(s)} />')

    def term(self, name, x, y, orient, role="data"):
        self.nt += 1
        pot = ' potential="0V"' if role in ("0v", "l-") else ""
        self.terminals.append(
            f'<terminal x={q(fmt(x))} y={q(fmt(y))} name={q(name)} '
            f'orientation={q(orient)} type="Generic" '
            f'uuid={q(uid(self.key, "t", name))}{pot} />')
        self.ext(x, y, x, y)

    def dtext(self, x, y, info, size=9, bold=False):
        self.ext(x, y, x + 40, y + size * 1.7)
        font = ("Liberation Sans,%d,-1,5,%d,0,0,0,0,0,%s"
                % (size, 75 if bold else 50, "Bold" if bold else "Regular"))
        self.dynamic.append(
            f'<dynamic_text x={q(fmt(x))} y={q(fmt(y))} z="25" '
            f'text_width="-1" Halignment="AlignLeft" Valignment="AlignTop" '
            f'frame="false" rotation="0" keep_visual_rotation="true" '
            f'text_from="ElementInfo" '
            f'uuid={q(uid(self.key, "d", info))} font={q(font)}>'
            f'<text></text><info_name>{info}</info_name></dynamic_text>')

    def finalize(self):
        x0, y0, x1, y1 = self.bb
        self.hx = ceil10(-x0 + 5)
        self.hy = ceil10(-y0 + 5)
        self.w = self.hx + ceil10(x1 + 5)
        self.h = self.hy + ceil10(y1 + 5)


def role_color(role):
    return ROLE_COLOR.get(role, "black")


# ---------------------------------------------------------------- schematic


def fit(text, size, avail):
    return max(4, min(size, int(avail / (0.5 * max(len(text), 1)))))


def schema(p):
    b = Builder(f"{p.id}/schema")
    slots = {"w": 0, "e": 0}
    layout = []
    for g in p.groups:
        layout.append((g, slots[g.side]))
        slots[g.side] += 1 + len(g.terms)
    n = max(slots.values())
    b.rect(0, -5, 100, 10 * n + 10)
    b.line(50, -5, 50, 10 * n - 5, weight="thin", line="dotted")
    b.line(0, 10 * n - 5, 100, 10 * n - 5, weight="thin")
    for g, k in layout:
        west = g.side == "w"
        x0 = 0 if west else 50
        b.rect(x0, 10 * k - 5, 50, 10, fill="lightgray", weight="thin")
        size = fit(g.title, 6, 44)
        b.text(x0 + 3, 10 * k - 4, g.title, size, True)
        for i, t in enumerate(g.terms):
            y = 10 * (k + 1 + i)
            col = role_color(t.role)
            if west:
                b.line(-10, y, 0, y, color=col)
                b.term(t.name, -10, y, "w", t.role)
                b.text(4, y - 5, t.text, fit(t.text, 6, 44))
            else:
                b.line(100, y, 110, y, color=col)
                b.term(t.name, 110, y, "e", t.role)
                size = fit(t.text, 6, 44)
                b.text(96, y - 5, t.text, size, align="r")
    b.text(50, 10 * n - 5, p.ref, 8, True, "c")
    b.dtext(0, 10 * n + 10, "label")
    return b


# ---------------------------------------------------------------- realistic


def zone_terms(p, zone):
    return [t for g in p.groups if g.kind in ("block", "rtd")
            and g.zone == zone for t in g.terms]


def screw_row(b, terms, y, top):
    n = len(terms)
    start = -50 + 10 * math.ceil((10 - n) / 2)
    for i, t in enumerate(terms):
        x = start + 10 * i
        col = role_color(t.role)
        b.circle(x, y, 9, fill="white", color=col)
        b.term(t.name, x, y, "n" if top else "s", t.role)
        level = i % 2
        ty = (-186 - 10 * level) if top else (167 + 10 * level)
        b.text(x, ty, t.text, 6, align="c")


def pin_circle(b, x, y, pin, term=None):
    col = role_color(term.role) if term else "black"
    b.circle(x, y, 9, fill="white", color=col)
    b.text(x, y - 4.25, str(pin), 5, align="c")
    if term:
        b.term(term.name, x, y, "n", term.role)


def rj45_socket(b, g, yb):
    b.text(-5, yb - 32, g.title, 6, True, "c")
    b.rect(-48, yb - 12, 86, 24, fill="lightgray", weight="thin")
    if g.terms and g.terms[0].pin is None:
        t = g.terms[0]
        b.circle(-10, yb, 9, fill="white", color="black")
        b.term(t.name, -10, yb, "n", t.role)
        b.text(-3, yb - 4.25, "EXP", 5)
        return
    used = {t.pin: t for t in g.terms}
    for pin in range(1, 9):
        pin_circle(b, -40 + 10 * (pin - 1), yb, pin, used.get(pin))


def plc_realistic(p, key):
    b = Builder(key)
    b.rect(-60, -205, 110, 395, fill="white")
    b.rect(-60, -205, 110, 55, fill="lightgray", weight="thin")
    b.rect(-60, 140, 110, 50, fill="lightgray", weight="thin")
    screw_row(b, zone_terms(p, "top"), -170, True)
    screw_row(b, zone_terms(p, "bottom"), 160, False)
    b.circle(-35, -138, 6, fill="green")
    b.text(-29, -143, "RUN", 5)
    b.circle(5, -138, 6, fill="red")
    b.text(11, -143, "ERR", 5)
    for g in p.groups:
        if g.kind == "power":
            xs = (-40, -30, -20)
            cap = "24V DC"
        elif g.kind == "rs485":
            xs = (10, 20)
            cap = "RS485"
        else:
            continue
        for x, t in zip(xs, g.terms):
            b.circle(x, -100, 9, fill="white", color=role_color(t.role))
            b.term(t.name, x, -100, "n", t.role)
            b.text(x, -118, t.name.split(".")[-1], 6, align="c")
        b.text((xs[0] + xs[-1]) / 2, -92, cap, 5, True, "c")
    yb = -40
    for g in p.groups:
        if g.kind == "rj45":
            rj45_socket(b, g, yb)
            yb += 60
    b.text(-5, 90, p.ref, 8, True, "c")
    b.dtext(-60, 196, "label")
    return b


def pinout_lines(p):
    lines = []
    for g in p.groups:
        if g.kind in ("block", "rtd"):
            continue
        parts = []
        for t in g.terms:
            sig = t.text if g.kind == "db9" else t.name.split(".")[-1]
            parts.append(f"{t.pin} {sig.split(' ', 1)[-1]}"
                         if t.pin else sig)
        text = f"{g.title}: " + ", ".join(parts)
        while len(text) > 52:
            cut = text.rfind(", ", 0, 52)
            lines.append(text[:cut + 1])
            text = "  " + text[cut + 2:]
        lines.append(text)
    return lines


def side_panel(b, p, x, y):
    b.text(x, y, p.ref, 8, True)
    y += 14
    for line in p.facts:
        b.text(x, y, line, 6)
        y += 9
    y += 5
    w, h, d = p.dims
    b.text(x, y, f"Dimensions: {w:g} x {h:g} x {d:g} mm", 6, True)
    y += 14
    for line in pinout_lines(p):
        b.text(x, y, line, 6)
        y += 9
    y += 5
    for name, col in (("+24 V", "red"), ("0 V", "blue"), ("PE / FG",
                                                           "orange")):
        b.rect(x, y + 2, 8, 4, fill=col, color=col)
        b.text(x + 12, y, name, 6)
        y += 9


def plc_description(p, key):
    b = plc_realistic(p, key)
    side_panel(b, p, 62, -205)
    return b


# --------------------------------------------------------------------- HMI


def hmi_realistic(p, key, panel=False):
    b = Builder(key)
    w, h, _ = p.dims
    fw, fh = w / 2.0, h / 2.0
    b.rect(-fw / 2, 0, fw, fh, fill="white")
    diag, (rx, ry) = p.screen
    a = rx / float(ry)
    dmm = diag * 25.4
    sw = dmm * a / math.sqrt(1 + a * a) / 2.0
    sh = dmm / math.sqrt(1 + a * a) / 2.0
    b.rect(-sw / 2, (fh - sh) / 2, sw, sh, fill="lightgray", weight="thin")
    b.text(0, fh + 4, p.ref, 8, True, "c")
    cy = ceil10(fh) + 60
    for g in p.groups:
        if g.kind == "power":
            b.rect(-25, cy - 12, 50, 24, fill="lightgray", weight="thin")
            for x, t in zip((-10, 0, 10), g.terms):
                b.circle(x, cy, 9, fill="white", color=role_color(t.role))
                b.term(t.name, x, cy, "n", t.role)
                b.text(x, cy - 24, t.name, 6, align="c")
            cy += 50
        elif g.kind == "db9":
            b.text(0, cy - 30, g.title, 6, True, "c")
            b.rect(-50, cy - 15, 100, 50, fill="lightgray", weight="thin")
            used = {t.pin: t for t in g.terms}
            for pin in range(1, 10):
                if pin <= 5:
                    x, y = -40 + 20 * (pin - 1), cy
                else:
                    x, y = -30 + 20 * (pin - 6), cy + 20
                pin_circle(b, x, y, pin, used.get(pin))
            cy += 80
        else:
            t = g.terms[0]
            sizes = {"Ethernet": (60, 40), "USB host": (48, 30),
                     "USB slave": (28, 22)}
            sw2, sh2 = sizes[g.title]
            b.text(0, cy - 32, g.title, 6, True, "c")
            b.rect(-sw2 / 2, cy - sh2 / 2, sw2, sh2, fill="lightgray",
                   weight="thin")
            b.circle(0, cy, 9, fill="white")
            b.term(t.name, 0, cy, "n", t.role)
            if g.title == "Ethernet":
                for i in range(8):
                    x = -17.5 + 5 * i
                    b.line(x, cy + 12, x, cy + 18)
            cy += 60
    b.dtext(-fw / 2, cy + 10, "label")
    if panel:
        side_panel(b, p, ceil10(fw / 2) + 20, 0)
    return b


def hmi_description(p, key):
    return hmi_realistic(p, key, True)


# ------------------------------------------------------------------- output


def head(p, view, b, link="master"):
    suffix, en, fr = VIEWS[view]
    uuid_ = uid(p.id, view)
    out = [f'<definition version="0.100.0" type="element" '
           f'link_type="{link}" width="{b.w}" height="{b.h}" '
           f'hotspot_x="{b.hx}" hotspot_y="{b.hy}">',
           f'\t<uuid uuid={q(uuid_)} />',
           '\t<names>',
           f'\t\t<name lang="en">{esc(p.ref + " " + p.designation + en)}'
           '</name>',
           f'\t\t<name lang="fr">{esc(p.ref + " " + p.designation_fr + fr)}'
           '</name>',
           '\t</names>']
    return out


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def info_lines(p):
    w, h, d = p.dims

    def e(name, value, show):
        return (f'\t\t<elementInformation name="{name}" show="{show}">'
                f'{esc(str(value))}</elementInformation>')

    lines = [e("manufacturer", "Kinco", 1),
             e("manufacturer_reference", p.ref, 1),
             e("designation", p.designation, 1),
             e("description", p.description, 1),
             e("quantity", "1", 0), e("unity", "pcs", 0),
             e("width", fmt(w), 0), e("height", fmt(h), 0),
             e("depth", fmt(d), 0)]
    for i, (des, desc, qty) in enumerate(p.aux, start=1):
        lines += [e(f"auxiliary{i}", des, 0),
                  e(f"description_auxiliary{i}", desc, 0),
                  e(f"designation_auxiliary{i}", des, 0),
                  e(f"quantity_auxiliary{i}", qty, 0),
                  e(f"unity_auxiliary{i}", "pcs", 0)]
    return ["\t<elementInformations>"] + lines + ["\t</elementInformations>"]


def plc_ios(p):
    lines = []
    for typ, addr, names in p.ios():
        lines.append(f'\t\t\t\t<plcIO type="{typ}" address={q(addr)} '
                     f'functionText="" comment="" crossRef="" '
                     f'terminalCount="{len(names)}">')
        lines += [f"\t\t\t\t\t<terminal>{esc(n)}</terminal>" for n in names]
        lines.append("\t\t\t\t</plcIO>")
    return lines


def kind_lines(p, table=False):
    lines = ["\t<kindInformations>",
             '\t\t<kindInformation name="type">plc</kindInformation>',
             '\t\t<plcMasterData rowHeight="%s">' % ("12.00" if table
                                                      else "8.00")]
    if table:
        widths = (60, 66, 130, 90, 64, 44)
        names = ("", "Address", "Label", "", "", "Terminal")
        vis = ("false", "true", "true", "false", "false", "true")
        lines.append("\t\t\t<columnWidths>")
        lines += [f'\t\t\t\t<column index="{i}" width="{w}"/>'
                  for i, w in enumerate(widths)]
        lines.append("\t\t\t</columnWidths>")
        lines.append("\t\t\t<columnVisibility>")
        lines += [f'\t\t\t\t<column index="{i}" visible="{v}"/>'
                  for i, v in enumerate(vis)]
        lines.append("\t\t\t</columnVisibility>")
        lines.append("\t\t\t<columnNames>")
        lines += [f'\t\t\t\t<column index="{i}">{n}</column>'
                  for i, n in enumerate(names)]
        lines.append("\t\t\t</columnNames>")
        lines.append("\t\t\t<columnOrder>5,2,1</columnOrder>")
        lines.append('\t\t\t<headerFont family="Roboto Condensed" size="7" '
                     'bold="true"/>')
        lines.append('\t\t\t<cellFont family="Roboto Condensed" size="7" '
                     'bold="false"/>')
    else:
        lines.append("\t\t\t<columnVisibility>")
        lines += [f'\t\t\t\t<column index="{i}" visible="false" />'
                  for i in range(5)]
        lines.append("\t\t\t</columnVisibility>")
    lines.append("\t\t\t<plcIOs>")
    lines += plc_ios(p)
    lines += ["\t\t\t</plcIOs>", "\t\t</plcMasterData>",
              "\t</kindInformations>"]
    return lines


def informations(note):
    return ("\t<informations>Author: QElectroTech contributors\n" + note +
            " " + LICENSE + "</informations>")


NOTE = ("Generated by misc/kinco/gen_kinco.py from public Kinco documents, "
        "vector drawing, grid 10 px.")


def element_text(p, view, b):
    b.finalize()
    lines = head(p, view, b) + info_lines(p) + kind_lines(p)
    lines.append(informations(NOTE))
    lines.append("\t<description>")
    lines += ["\t\t" + s for s in b.shapes + b.terminals + b.dynamic]
    lines += ["\t</description>", "</definition>"]
    return "\n".join(lines) + "\n"


def table_text(p):
    n = len(p.ios())
    b = Builder(f"{p.id}/table")
    b.w, b.h, b.hx, b.hy = 250, 29 + 14 + 12 * n + 15, 5, 29
    b.dtext(0, -24, "label", 9, True)
    b.dtext(60, -24, "manufacturer_reference", 7, True)
    b.dtext(60, -12, "designation", 7)
    lines = head(p, "table", b) + info_lines(p) + kind_lines(p, True)
    lines.append(informations(NOTE))
    lines.append("\t<description>")
    lines += ["\t\t" + s for s in b.dynamic]
    lines.append('\t\t<plc_table x="0" y="0" style="line-style:normal;'
                 'line-weight:thin;filling:none;color:black" '
                 'antialias="false"/>')
    lines += ["\t</description>", "</definition>"]
    return "\n".join(lines) + "\n"


def footprint_text(p, kind):
    """Layout footprint, 1 px = 2 mm, simple element with a label."""
    b = Builder(f"{p.id}/fp_{kind}")
    w, h, d = p.dims
    if kind == "fp":
        rw, rh, title = w / 2, h / 2, "layout, DIN rail front view"
        title_fr = "implantation, vue de face sur rail DIN"
        dims = f"{w:g} x {h:g} x {d:g} mm"
        b.rect(0, 0, rw, rh)
        b.line(0, rh * 0.15, rw, rh * 0.15, weight="thin")
        b.line(0, rh * 0.85, rw, rh * 0.85, weight="thin")
    elif kind == "face":
        rw, rh, title = w / 2, h / 2, "layout, front face"
        title_fr = "implantation, face avant"
        dims = f"{w:g} x {h:g} x {d:g} mm (width x height x depth)"
        b.rect(0, 0, rw, rh)
        diag, (rx, ry) = p.screen
        a = rx / float(ry)
        sw = diag * 25.4 * a / math.sqrt(1 + a * a) / 2.0
        sh = diag * 25.4 / math.sqrt(1 + a * a) / 2.0
        b.rect((rw - sw) / 2, (rh - sh) / 2, sw, sh, weight="thin")
    else:
        cw, ch = p.cutout
        rw, rh, title = cw / 2, ch / 2, "door cut-out"
        title_fr = "découpe de porte"
        dims = f"{cw} x {ch} mm (width x height)"
        b.rect(0, 0, rw, rh, line="dotted")
        b.line(0, 0, rw, rh, weight="thin", line="dotted")
        b.line(0, rh, rw, 0, weight="thin", line="dotted")
    b.dtext(1, 1, "label", 5)
    b.finalize()
    b.hx = b.hy = 10
    b.w, b.h = 10 + ceil10(rw + 5), 10 + ceil10(rh + 5)
    des = f"{p.ref} {title}"
    lines = [f'<definition version="0.100.0" type="element" '
             f'link_type="simple" width="{b.w}" height="{b.h}" '
             f'hotspot_x="10" hotspot_y="10">',
             f'\t<uuid uuid={q(uid(p.id, "fp", kind))} />',
             "\t<names>",
             f'\t\t<name lang="en">{esc(des)}</name>',
             f'\t\t<name lang="fr">{esc(p.ref + " " + title_fr)}</name>',
             "\t</names>", "\t<elementInformations>",
             '\t\t<elementInformation name="manufacturer" show="0">Kinco'
             '</elementInformation>',
             f'\t\t<elementInformation name="manufacturer_reference" '
             f'show="0">{esc(p.ref)}</elementInformation>',
             f'\t\t<elementInformation name="description" show="1">'
             f'{esc(p.ref + ", " + dims)}</elementInformation>',
             "\t</elementInformations>",
             informations(NOTE + " 1 px = 2 mm."),
             "\t<description>"]
    lines += ["\t\t" + s for s in b.shapes + b.dynamic]
    lines += ["\t</description>", "</definition>"]
    return "\n".join(lines) + "\n"


def directory_text(en, fr, de=None):
    lines = ["<qet-directory>", "\t<names>",
             f'\t\t<name lang="en">{esc(en)}</name>',
             f'\t\t<name lang="fr">{esc(fr)}</name>']
    if de:
        lines.insert(3, f'\t\t<name lang="de">{esc(de)}</name>')
    lines += ["\t</names>", "</qet-directory>"]
    return "\n".join(lines) + "\n"


def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)


def builders(p):
    if p.kind == "hmi":
        return {"schema": schema(p),
                "realistic": hmi_realistic(p, f"{p.id}/realistic"),
                "description": hmi_description(p, f"{p.id}/description")}
    return {"schema": schema(p),
            "realistic": plc_realistic(p, f"{p.id}/realistic"),
            "description": plc_description(p, f"{p.id}/description")}


def main():
    write(os.path.join(OUT, "qet_directory"),
          directory_text("Kinco", "Kinco", "Kinco"))
    count = 0
    for fam, (en, fr, de) in D.FAMILIES.items():
        write(os.path.join(OUT, fam, "qet_directory"),
              directory_text(en, fr, de))
    for p in D.all_products():
        folder = os.path.join(OUT, p.family, p.id)
        write(os.path.join(folder, "qet_directory"),
              directory_text(p.ref, p.ref, p.ref))
        for view, b in builders(p).items():
            write(os.path.join(folder, p.id + VIEWS[view][0] + ".elmt"),
                  element_text(p, view, b))
            count += 1
        write(os.path.join(folder, p.id + "_table.elmt"), table_text(p))
        count += 1
        kinds = ("face", "cutout") if p.kind == "hmi" else ("fp",)
        for k in kinds:
            name = f"{p.id}_fp" + ("" if k == "fp" else "_" + k)
            write(os.path.join(OUT, "implantation", name + ".elmt"),
                  footprint_text(p, k))
            count += 1
    print(f"{count} elements written to {OUT}")


if __name__ == "__main__":
    main()
