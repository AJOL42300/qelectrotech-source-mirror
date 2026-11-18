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
"""Write examples/kinco_example.qet.

The project shows a KS105C2-16DT (A1), four expansion modules (A2 to A5)
and an M2070HE operator panel (A6), with their power supply and
communication links, then the realistic views, description views, I/O
tables and layout footprints of these products, and a catalogue folio set
with every product of the library. Folios are named by category.

It reuses the project writer of misc/beckhoff/gen_example.py, pointed at
the Kinco library.

Usage: python misc/kinco/gen_example.py
The output is deterministic (all uuids are derived from fixed names).
"""
import importlib.util
import os
import sys
import uuid

HERE = os.path.dirname(os.path.abspath(__file__))
BECKHOFF = os.path.join(HERE, "..", "beckhoff")
sys.path.insert(0, BECKHOFF)
sys.path.insert(0, HERE)
_spec = importlib.util.spec_from_file_location(
    "beckhoff_example", os.path.join(BECKHOFF, "gen_example.py"))
bg = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(bg)
import kinco_data as D  # noqa: E402

ROOT = bg.ROOT
CAT = "10_electric/20_manufacturers_articles/kinco"
bg.CAT = CAT
bg.ELMT_DIR = os.path.join(ROOT, "extra-elements", *CAT.split("/"))
bg.OUT = os.path.join(ROOT, "examples", "kinco_example.qet")
bg.NS = uuid.UUID("5e3a9c10-7b2d-5f4e-8a61-1d9c0b7e2f43")
bg.USED = {("A1", "I0.0"): "Presence sensor",
           ("A1", "Q0.0"): "Run lamp",
           ("A2", "I0.0"): "Part present"}
DEV = bg.DEV

CPU = "ks105c2_16dt"
MODULES = ["ks121_16dx", "ks122_14dt", "ks133_06iv", "ks131_04rd"]
HMI = "m2070he"
STYLE = ('style="line-style:normal;line-weight:normal;filling:none;'
         'color:black" antialias="false"')


def generic(name, title, terms, side):
    """A generic box with one terminal per name, on the given side."""
    n = len(terms)
    x = -50 if side == "w" else 50
    orient = "w" if side == "w" else "e"
    height = 10 + ((10 * n + 30 + 9) // 10) * 10
    lines = [
        f'<definition version="0.100.0" type="element" link_type="simple" '
        f'width="100" height="{height}" hotspot_x="50" hotspot_y="10">',
        f'<uuid uuid="{bg.uid("dev", name)}"/>',
        f'<names><name lang="en">{title} (generic)</name></names>',
        "<informations>Generic example symbol. License: see "
        "http://qelectrotech.org/wiki/doc/elements_license</informations>",
        "<description>",
        f'<rect x="-40" y="-5" width="80" height="{10 * n + 10}" rx="0" '
        f'ry="0" {STYLE}/>',
        f'<text x="-36" y="-3" rotation="0" color="#000000" '
        f'font="Roboto Condensed,6,-1,5,75,0,0,0,0,0,Bold" '
        f'text="{title}"/>']
    for i, t in enumerate(terms):
        y = 10 * (i + 1)
        x0 = -40 if side == "w" else 40
        lines.append(f'<line x1="{x}" y1="{y}" x2="{x0}" y2="{y}" '
                     'end1="none" end2="none" length1="1.5" length2="3" '
                     f'{STYLE}/>')
        tx = -36 if side == "w" else 36 - 3 * len(t)
        lines.append(f'<text x="{tx}" y="{y - 5}" rotation="0" '
                     'color="#000000" font="Roboto Condensed,6,-1,5,50,0,'
                     f'0,0,0,0,Regular" text="{t}"/>')
        lines.append(f'<terminal x="{x}" y="{y}" name="{t}" '
                     f'orientation="{orient}" type="Generic" '
                     f'uuid="{bg.uid("dev", name, t)}"/>')
    lines.append(
        f'<dynamic_text x="-40" y="{10 * n + 8}" z="25" text_width="-1" '
        'Halignment="AlignLeft" Valignment="AlignTop" frame="false" '
        'rotation="0" keep_visual_rotation="true" text_from="ElementInfo" '
        f'uuid="{bg.uid("dev", name, "label")}" '
        'font="Liberation Sans,9,-1,5,50,0,0,0,0,0,Regular"><text></text>'
        "<info_name>label</info_name></dynamic_text>")
    lines += ["</description></definition>"]
    return "\n".join(lines)


bg.DEVICES["psu"] = generic("psu", "24 V DC", ["24V", "0V", "PE"], "e")
bg.DEVICES["pc"] = generic("pc", "PC", ["ETH"], "w")
bg.DEVICES["can_node"] = generic("can_node", "CAN node",
                                 ["H", "L", "GND"], "w")


class Inst:
    def __init__(self, key, label, el):
        self.key, self.label, self.el = key, label, el

    def place(self, x, y):
        return bg.element_xml(self.el, self.label, x, y, self.key)


def sym(key, label, name):
    return Inst(key, label, bg.get(CAT, name))


def dev(key, label, name):
    return Inst(key, label, bg.get(DEV, name))


def wire(a, ta, b, tb):
    return bg.conductor_xml(f"{a.key}:{ta}-{b.key}:{tb}", a.key, a.label,
                            a.el, ta, b.key, b.label, b.el, tb)


def flat(rel, name):
    category = CAT + "/implantation"
    key = (category, name)
    if key not in bg.ELEMENTS:
        bg.ELEMENTS[key] = bg.Elmt(category, name, bg.load(category, name))
    return bg.ELEMENTS[key]


class Folios:
    """Numbers the folios in the order they are added."""

    def __init__(self):
        self.items = []

    def add(self, title, elements, conductors=(), portrait=False):
        self.items.append(bg.diagram_xml(len(self.items) + 1, title,
                                         elements, list(conductors),
                                         portrait))

    def grid(self, title, items, left=40, top=30, right=1190, bottom=740):
        """items are (element, label, key); wraps to rows and folios."""
        pages, els = [], []
        x, y, row = left, top, 0
        for el, label, key in items:
            w = int(el.root.get("width"))
            h = int(el.root.get("height"))
            if x + w > right and x > left:
                x, y, row = left, y + row + 40, 0
            if y + h > bottom and els:
                pages.append(els)
                els, x, y, row = [], left, top, 0
            els.append(bg.element_xml(
                el, label, x + int(el.root.get("hotspot_x")),
                y + int(el.root.get("hotspot_y")), key))
            x += w + 30
            row = max(row, h)
        pages.append(els)
        for n, page in enumerate(pages):
            t = title
            if len(pages) > 1:
                t += f" ({n + 1}/{len(pages)})"
            self.add(t, page)


def folio_power(f):
    cpu = sym("pwr/A1", "A1", CPU)
    mods = [sym(f"pwr/A{n + 2}", f"A{n + 2}", m)
            for n, m in enumerate(MODULES)]
    g1 = dev("pwr/G1", "G1", "psu")
    b1 = dev("pwr/B1", "B1", "sensor")
    h1 = dev("pwr/H1", "H1", "lamp")
    b2 = dev("pwr/B2", "B2", "sensor")
    els = [g1.place(110, 100), cpu.place(300, 70), b1.place(230, 430),
           h1.place(430, 450), b2.place(450, 430)]
    for n, m in enumerate(mods):
        els.append(m.place(470 + 170 * n + (0 if n else 20), 70))
    cond = []
    chain = [cpu] + mods
    for a in chain:
        cond.append(wire(g1, "24V", a, "24V"))
        cond.append(wire(g1, "0V", a, "0V"))
    cond.append(wire(g1, "PE", cpu, "PE"))
    for a, b in zip(chain, chain[1:]):
        cond.append(wire(a, "RS485.B", b, "RS485.B"))
        cond.append(wire(a, "RS485.A", b, "RS485.A"))
    cond.append(wire(b1, "1", cpu, "I0.0"))
    cond.append(wire(cpu, "Q0.0", h1, "1"))
    cond.append(wire(b2, "1", mods[0], "I0.0"))
    f.add("A1 - Automate KS105 et modules d'extension - symboles, "
          "alimentation et RS485", els, cond)


def folio_comm(f):
    cpu = sym("com/A1", "A1", CPU)
    hmi = sym("com/A6", "A6", HMI)
    g1 = dev("com/G1", "G1", "psu")
    pc = dev("com/PC1", "PC1", "pc")
    node = dev("com/N1", "N1", "can_node")
    els = [g1.place(110, 100), cpu.place(300, 70), hmi.place(640, 70),
           pc.place(960, 100), node.place(450, 450)]
    cond = [wire(g1, "24V", cpu, "24V"), wire(g1, "0V", cpu, "0V"),
            wire(g1, "24V", hmi, "24V"), wire(g1, "0V", hmi, "0V"),
            wire(g1, "PE", cpu, "PE"), wire(g1, "PE", hmi, "FG"),
            wire(cpu, "RS485.B", hmi, "COM0.1"),
            wire(cpu, "RS485.A", hmi, "COM0.6"),
            wire(hmi, "ETH", pc, "ETH"),
            wire(cpu, "CAN1.H", node, "H"),
            wire(cpu, "CAN1.L", node, "L"),
            wire(cpu, "CAN1.GND", node, "GND")]
    f.add("A1, A6 - Automate KS105 et IHM M2 - symboles, "
          "communication", els, cond)


def views(f):
    plc = [("A1", CPU)] + [(f"A{n + 2}", m)
                           for n, m in enumerate(MODULES)]
    for suffix, key, title in (
            ("_realistic", "real", "vues réalistes"),
            ("_description", "desc", "vues descriptives"),
            ("_table", "table", "tableaux d'E/S"),
            ("_din", "din", "vues rail DIN verticales")):
        if suffix == "_din":
            f.grid(f"A1 - Automate KS105 - {title}",
                   [(bg.get(CAT, CPU + suffix), "A1", f"{key}/A1")])
            continue
        f.grid(f"A1-A5 - Automate KS105 et modules - {title}",
               [(bg.get(CAT, n + suffix), lab, f"{key}/{lab}")
                for lab, n in plc])
        f.grid(f"A6 - IHM M2 - {title}",
               [(bg.get(CAT, HMI + suffix), "A6", f"{key}/A6")])


def folio_layout(f):
    plc = [("A1", CPU)] + [(f"A{n + 2}", m)
                           for n, m in enumerate(MODULES)]
    items = [(flat("", n + "_fp"), lab, f"fp/{lab}") for lab, n in plc]
    items.append((flat("", HMI + "_fp_face"), "A6", "fp/A6"))
    items.append((flat("", HMI + "_fp_cutout"), "A6 cut-out",
                  "fp/A6c"))
    f.grid("Implantation - rail DIN, face avant et découpe de porte "
           "(1 px = 2 mm)", items, 40, 30, 600, 700)


def folio_catalogue(f):
    cats = {}
    for p in D.all_products():
        cats.setdefault(p.family, []).append(p)
    for fam, (_, fr, _) in D.FAMILIES.items():
        if fam == "implantation":
            items = []
            for p in D.all_products():
                kinds = ("face", "cutout") if p.kind == "hmi" else ("fp",)
                for k in kinds:
                    name = f"{p.id}_fp" + ("" if k == "fp" else "_" + k)
                    items.append((flat("", name),
                                  p.ref + ("" if k == "fp" else " " + k),
                                  f"catalogue/{name}"))
            f.grid(f"Catalogue - {fr}", items, 40, 30, 1190, 740)
            continue
        f.grid(f"Catalogue - {fr}",
               [(bg.get(CAT, p.id), p.ref, f"catalogue/{p.id}")
                for p in cats[fam]])
        if fam == "ks105_cpu":
            f.grid(f"Catalogue - {fr}, rail DIN vertical",
                   [(bg.get(CAT, p.id + "_din"), p.ref,
                     f"catalogue/{p.id}_din") for p in cats[fam]])


def main():
    f = Folios()
    folio_power(f)
    folio_comm(f)
    views(f)
    folio_layout(f)
    folio_catalogue(f)
    xrefs = "".join(
        f'<xref delayprefix="" displayhas="cross" master_label="%f-%l%c" '
        'offset="0" powerprefix="" showallconfiguredslaves="false" '
        'showpowerctc="true" showterminalname="true" '
        'slave_label="(%f-%l%c)" slave_offset="0" snapto="label" '
        f'stackoverlapping="true" switchprefix="" type="{t}" '
        'xrefpos="AlignBottom"/>'
        for t in ("cable", "coil", "commutator", "plc", "protection"))
    head = (
        f'<project title="Kinco KS105 and HMI M2 example" '
        f'uuid="{bg.uid("project")}" version="{bg.VERSION}"><properties/>'
        + bg.titleblock_templates() +
        '<usage enabled="true" time_spent="0"/><newdiagrams>'
        '<border cols="20" colsize="61" displaycols="true" '
        'displayrows="true" rows="10" rowsize="80"/>'
        '<inset author="" auto_page_num="" date="null" displayAt="bottom" '
        'filename="" folio="" indexrev="" locmach="" plant="" title="" '
        'titleblocktemplate="A3 landscape" '
        'titleblocktemplateCollection="embedded" version=""/>'
        f"<conductors {bg.DEFCOND}/><report label=\"%f-%l%c\"/>"
        f"<xrefs>{xrefs}</xrefs>"
        '<conductors_autonums auto_break_conductors="false" '
        'current_autonum="" freeze_new_conductors="false"/><folio_autonums/>'
        '<element_autonums current_autonum="" freeze_new_elements="false"/>'
        "<guides/></newdiagrams>")
    text = ('<?xml version="1.0" encoding="UTF-8"?>\n' + head +
            "".join(f.items) + bg.collection_xml() + "</project>\n")
    bg.ET.fromstring(text.encode("utf-8"))
    os.makedirs(os.path.dirname(bg.OUT), exist_ok=True)
    with open(bg.OUT, "w", encoding="utf-8", newline="\n") as out:
        out.write(text)
    print("wrote", os.path.relpath(bg.OUT, ROOT), len(f.items), "folios")


if __name__ == "__main__":
    main()
