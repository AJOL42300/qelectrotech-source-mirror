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
"""Small helpers shared by the Murrelektronik and Mean Well generators.

A Canvas collects the vector parts of one view of an element (lines,
rectangles, circles, polygons, texts) and the bounding box they cover.
write_element() turns a Canvas, the terminals and the pin table of a device
into a .elmt file, the way the Beckhoff library of this repository does it:
tabs, one part per line, deterministic uuids.
"""
import os
import uuid
from xml.sax.saxutils import quoteattr

NS = uuid.UUID("6f1d3a52-8c0e-4b7d-a1c9-3e5b72d94a08")
LICENCE = "License: see http://qelectrotech.org/wiki/doc/elements_license"
FONT = "Roboto Condensed"
TABLE_WIDTHS = [60, 140, 130, 90, 64, 44]
TABLE_COLUMNS = (1, 2, 5)

# Colours used by the libraries: +24 V red, 0 V blue, protective earth
# yellow and green, signals black, status LEDs green.
RED, BLUE, YELLOW, GREEN, BLACK = "red", "blue", "yellow", "green", "black"


def uid(*parts):
    return str(uuid.uuid5(NS, "/".join(parts)))


def esc(text):
    return quoteattr(str(text))[1:-1]


def num(v):
    v = round(v, 2)
    return str(int(v)) if v == int(v) else ("%.2f" % v).rstrip("0")


def text_size(s, size, bold=False):
    """Estimated width and height, in pixels, of a text in the font used."""
    return (len(s) * size * 4.0 / 3.0 * 0.47 * (1.08 if bold else 1.0) + 1,
            size * 4.0 / 3.0 + 1)


def text_ascent(size):
    """Distance from the top of the text box to the baseline. QET draws a
    text part with its (x, y) on the baseline, not on the top left corner."""
    return size * 4.0 / 3.0 * 0.93


def style(weight="thin", fill="none", color=BLACK, dash="normal"):
    return (f"line-style:{dash};line-weight:{weight};filling:{fill};"
            f"color:{color}")


class Canvas:
    """The parts of one view and their bounding box."""

    def __init__(self):
        self.parts = []
        self.box = [1e9, 1e9, -1e9, -1e9]
        self.text_boxes, self.rect_boxes, self.circle_shapes = [], [], []

    def layout_problems(self, margin=0.3):
        """Texts that touch another text or cross the outline of a
        rectangle or a circle (a text fully inside a shape is fine)."""
        found = []
        boxes = self.text_boxes
        for i, a in enumerate(boxes):
            for b in boxes[i + 1:]:
                if (a[0] < b[2] + margin and b[0] < a[2] + margin
                        and a[1] < b[3] + margin and b[1] < a[3] + margin):
                    found.append(f"texts {a[4]!r} and {b[4]!r} overlap")
            for r in self.rect_boxes:
                inter = (a[0] < r[2] and r[0] < a[2]
                         and a[1] < r[3] and r[1] < a[3])
                inside = (a[0] >= r[0] + margin and a[2] <= r[2] - margin
                          and a[1] >= r[1] + margin
                          and a[3] <= r[3] - margin)
                if inter and not inside:
                    found.append(f"text {a[4]!r} crosses a rectangle")
            for cx, cy, rad in self.circle_shapes:
                nx = min(max(cx, a[0]), a[2])
                ny = min(max(cy, a[1]), a[3])
                near = ((nx - cx) ** 2 + (ny - cy) ** 2) ** 0.5
                far = max(((x - cx) ** 2 + (y - cy) ** 2) ** 0.5
                          for x in (a[0], a[2]) for y in (a[1], a[3]))
                if near < rad + margin and far > rad - margin:
                    found.append(f"text {a[4]!r} crosses a circle")
        return found

    def grow(self, x1, y1, x2, y2):
        b = self.box
        b[0], b[1] = min(b[0], x1, x2), min(b[1], y1, y2)
        b[2], b[3] = max(b[2], x1, x2), max(b[3], y1, y2)

    def line(self, x1, y1, x2, y2, color=BLACK, weight="thin",
             dash="normal"):
        self.grow(x1, y1, x2, y2)
        self.parts.append(
            f'<line x1="{num(x1)}" y1="{num(y1)}" x2="{num(x2)}" '
            f'y2="{num(y2)}" end1="none" end2="none" length1="1.5" '
            f'length2="3" style="{style(weight, "none", color, dash)}" '
            'antialias="false"/>')

    def rect(self, x, y, w, h, fill="none", color=BLACK, weight="thin",
             dash="normal"):
        self.grow(x, y, x + w, y + h)
        self.rect_boxes.append((x, y, x + w, y + h))
        self.parts.append(
            f'<rect x="{num(x)}" y="{num(y)}" width="{num(w)}" '
            f'height="{num(h)}" rx="0" ry="0" '
            f'style="{style(weight, fill, color, dash)}" '
            'antialias="false"/>')

    def circle(self, x, y, d, fill="none", color=BLACK, weight="thin"):
        """x, y is the top left corner of the bounding square."""
        self.grow(x, y, x + d, y + d)
        self.circle_shapes.append((x + d / 2.0, y + d / 2.0, d / 2.0))
        self.parts.append(
            f'<circle x="{num(x)}" y="{num(y)}" diameter="{num(d)}" '
            f'style="{style(weight, fill, color)}" antialias="false"/>')

    def disc(self, cx, cy, d, fill="none", color=BLACK, weight="thin"):
        self.circle(cx - d / 2.0, cy - d / 2.0, d, fill, color, weight)

    def poly(self, points, closed=True, fill="none", color=BLACK,
             weight="thin"):
        for x, y in points:
            self.grow(x, y, x, y)
        coords = " ".join(f'x{i + 1}="{num(x)}" y{i + 1}="{num(y)}"'
                          for i, (x, y) in enumerate(points))
        self.parts.append(
            f'<polygon closed="{"true" if closed else "false"}" '
            f'style="{style(weight, fill, color)}" antialias="false" '
            f"{coords}/>")

    def text(self, x, y, s, size=5, bold=False, color="#000000",
             rotation=0):
        """x, y is the top left corner of the text box. With rotation=270
        the text reads upwards from x, y (the box is then above and to the
        right of that point). The file gets the baseline position, which
        is what QET uses."""
        w, h = text_size(s, size, bold)
        asc = text_ascent(size)
        if rotation == 270:
            box = (x, y - w, x + h, y)
            fx, fy = x + asc, y
        else:
            box = (x, y, x + w, y + h)
            fx, fy = x, y + asc
        self.grow(box[0], box[1], box[2], box[3])
        self.text_boxes.append(box + (s,))
        self.parts.append(
            f'<text x="{num(fx)}" y="{num(fy)}" rotation="{rotation}" '
            f'color="{color}" font="{FONT},{size},-1,5,'
            f'{75 if bold else 50},0,0,0,0,0,Regular" text="{esc(s)}"/>')

    def text_c(self, cx, y, s, size=5, bold=False, color="#000000"):
        self.text(cx - text_size(s, size, bold)[0] / 2.0, y, s, size, bold,
                  color)

    def text_m(self, cx, cy, s, size=5, bold=False, color="#000000"):
        """Centred on cx horizontally and on cy vertically."""
        w, h = text_size(s, size, bold)
        self.text(cx - w / 2.0, cy - h / 2.0, s, size, bold, color)

    def text_r(self, rx, y, s, size=5, bold=False, color="#000000"):
        """Right aligned: rx is the right edge."""
        self.text(rx - text_size(s, size, bold)[0], y, s, size, bold, color)


class Terminal:
    def __init__(self, name, x, y, orient, plc_type, ident, potential="0V"):
        self.name, self.x, self.y, self.orient = name, x, y, orient
        self.plc_type, self.ident, self.potential = plc_type, ident, potential


def dyn_text(key, x, y, info, size, bold):
    return (
        f'<dynamic_text x="{num(x)}" y="{num(y)}" z="25" text_width="-1" '
        'Halignment="AlignLeft" Valignment="AlignTop" frame="false" '
        'rotation="0" keep_visual_rotation="true" text_from="ElementInfo" '
        f'uuid="{{{uid(key, info)}}}" font="Liberation Sans,{size},-1,5,'
        f'{75 if bold else 50},0,0,0,0,0,Regular">'
        f"<text></text><info_name>{info}</info_name></dynamic_text>")


COLUMN_NAMES = ["", "ID", "Label", "", "", "N\u00b0 pin"]


def plc_xml(terminals, table):
    """The plcMasterData block; the columns are visible in the table view
    only. The ID (address) holds the signal, the pin is the terminal name."""
    out = ['\t\t<plcMasterData rowHeight="12.00">', "\t\t\t<columnWidths>"]
    out += [f'\t\t\t\t<column index="{i}" width="{w}"/>'
            for i, w in enumerate(TABLE_WIDTHS)]
    out += ["\t\t\t</columnWidths>", "\t\t\t<columnVisibility>"]
    out += [f'\t\t\t\t<column index="{i}" visible='
            f'"{"true" if table and i in TABLE_COLUMNS else "false"}"/>'
            for i in range(6)]
    out += ["\t\t\t</columnVisibility>", "\t\t\t<columnNames>"]
    out += [f'\t\t\t\t<column index="{i}">{esc(n)}</column>'
            for i, n in enumerate(COLUMN_NAMES)]
    out += ["\t\t\t</columnNames>", "\t\t\t<columnOrder>5,2,1"
            "</columnOrder>",
            '\t\t\t<headerFont family="Roboto Condensed" size="7" '
            'bold="true"/>',
            '\t\t\t<cellFont family="Roboto Condensed" size="7" '
            'bold="false"/>', "\t\t\t<plcIOs>"]
    for t in terminals:
        out.append(
            f'\t\t\t\t<plcIO type="{t.plc_type}" address="{esc(t.ident)}" '
            'functionText="" comment="" crossRef="" terminalCount="1">')
        out.append(f"\t\t\t\t\t<terminal>{esc(t.name)}</terminal>")
        out.append("\t\t\t\t</plcIO>")
    out += ["\t\t\t</plcIOs>", "\t\t</plcMasterData>"]
    return "\n".join(out)


def write_element(path, key, names, infos, informations, canvas, terminals,
                  table=False, label_at=(0, -24), margin=10, master=True,
                  extra_texts=()):
    """Write one .elmt file. names: lang -> text; infos: ordered name ->
    text. The terminals are in the local coordinates of the canvas; the
    bounding box of the parts gives the size and the hotspot. With
    master=False the element is a plain simple element (no pin table).
    extra_texts: (info, x, y, size) dynamic texts shown next to the label."""
    lx, ly = label_at
    if table:
        width = sum(TABLE_WIDTHS[i] for i in TABLE_COLUMNS) + 10
        height = 29 + 12 * (len(terminals) + 1) + 17
        hx, hy = 5, 29
    else:
        x1, y1, x2, y2 = canvas.box
        for t in terminals:
            stub = 4
            dx = {"w": -stub, "e": stub}.get(t.orient, 0)
            dy = {"n": -stub, "s": stub}.get(t.orient, 0)
            x1, y1 = min(x1, t.x + dx), min(y1, t.y + dy)
            x2, y2 = max(x2, t.x + dx), max(y2, t.y + dy)
        x1, y1 = min(x1, lx), min(y1, ly)
        x2, y2 = max(x2, lx + 40), max(y2, ly + 14)
        for _info, tx, ty, _size in extra_texts:
            x2, y2 = max(x2, tx + 40), max(y2, ty + 14)
        hx = -(-int(-x1 + margin) // 10) * 10
        hy = -(-int(-y1 + margin) // 10) * 10
        width = -(-int(x2 + margin + hx) // 10) * 10
        height = -(-int(y2 + margin + hy) // 10) * 10
    out = [f'<definition version="0.100.0" type="element" '
           f'link_type="{"master" if master else "simple"}" '
           f'width="{width}" height="{height}" '
           f'hotspot_x="{hx}" hotspot_y="{hy}">',
           f'\t<uuid uuid="{{{uid(key)}}}"/>', "\t<names>"]
    for lang, text in names.items():
        out.append(f'\t\t<name lang="{lang}">{esc(text)}</name>')
    out += ["\t</names>", "\t<elementInformations>"]
    for name, text in infos.items():
        out.append(f'\t\t<elementInformation name="{name}" show="1">'
                   f"{esc(text)}</elementInformation>")
    out.append("\t</elementInformations>")
    if master:
        out += ["\t<kindInformations>",
                '\t\t<kindInformation name="type">plc</kindInformation>',
                plc_xml(terminals, table), "\t</kindInformations>"]
    out += [f"\t<informations>{informations}\n{LICENCE}</informations>",
            "\t<description>"]
    out.append("\t\t" + dyn_text(key, lx, ly, "label", 9, True))
    for info, tx, ty, size in extra_texts:
        out.append("\t\t" + dyn_text(key, tx, ty, info, size, False))
    if table:
        out.append("\t\t" + dyn_text(key, 60, -24, "designation", 7, False))
        out.append('\t\t<plc_table x="0" y="0" style="line-style:normal;'
                   'line-weight:thin;filling:none;color:black" '
                   'antialias="false"/>')
    for p in canvas.parts:
        out.append("\t\t" + p)
    if not table:
        for t in terminals:
            out.append(
                f'\t\t<terminal x="{num(t.x)}" y="{num(t.y)}" '
                f'name="{esc(t.name)}" orientation="{t.orient}" '
                'type="Generic" '
                f'uuid="{{{uid(key, "terminal", t.name)}}}"'
                + (f' potential="{t.potential}"' if t.potential else "")
                + "/>")
    out += ["\t</description>", "</definition>", ""]
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(out))


def write_directory(folder, names):
    """names: lang -> text, the translated name of a folder."""
    os.makedirs(folder, exist_ok=True)
    out = ["<qet-directory>", "\t<names>"]
    out += [f'\t\t<name lang="{lang}">{esc(text)}</name>'
            for lang, text in names.items()]
    out += ["\t</names>", "</qet-directory>", ""]
    with open(os.path.join(folder, "qet_directory"), "w", encoding="utf-8",
              newline="\n") as f:
        f.write("\n".join(out))
