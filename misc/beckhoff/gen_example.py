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
"""Write examples/beckhoff_islands_example.qet.

The project shows one EtherCAT island: its head A1.00 and four child cards
A1.10, A1.20, A1.30 and A1.40 (a power feed terminal). Each card is drawn four times (symbol, realistic
view, description view, I/O table) with the same label, on four folios, and
a few card pins
are wired so that they show as used. A2 has a safety card and two drives, A3
two drives (1 and 2 axes). The Beckhoff elements of
extra-elements/10_electric/20_manufacturers_articles/beckhoff (family folders) are embedded in the
project; nothing comes from a manufacturer.

Usage: python misc/beckhoff/gen_example.py
The output is deterministic (all uuids are derived from fixed names).
"""
import copy
import os
import sys
import uuid
import xml.etree.ElementTree as ET
from xml.sax.saxutils import quoteattr

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_titleblocks  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
TB_DIR = os.path.join(ROOT, "titleblocks")
TITLEBLOCKS = {"A3 landscape": "A3_landscape", "A3 portrait": "A3_portrait"}
ELMT_DIR = os.path.join(ROOT, "extra-elements", "10_electric",
                        "20_manufacturers_articles", "beckhoff")
OUT = os.path.join(ROOT, "examples", "beckhoff_islands_example.qet")
NS = uuid.UUID("0b0d4f0e-6f1b-4c58-9a3e-5d1c9f5b7a10")
CAT = "10_electric/20_manufacturers_articles/beckhoff"
DEV = "example_devices"
VERSION = "0.200.1"


def uid(*parts):
    return "{" + str(uuid.uuid5(NS, "/".join(parts))) + "}"


def load(category, name):
    sub = category[len(CAT):].strip("/")
    return ET.parse(os.path.join(ELMT_DIR, *sub.split("/"), name + ".elmt")
                    if sub else os.path.join(ELMT_DIR, name + ".elmt")
                    ).getroot()


def device(name, label_en, shape, link="simple"):
    """A tiny generic sensor or lamp, two terminals, no data."""
    ids = [uid("dev", name, "1"), uid("dev", name, "2")]
    return f"""<definition version="0.100.0" type="element" \
link_type="{link}" width="30" height="50" hotspot_x="15" hotspot_y="25">
<uuid uuid="{uid("dev", name)}"/>
<names><name lang="en">{label_en}</name></names>
<informations>Generic example symbol. License: see \
http://qelectrotech.org/wiki/doc/elements_license</informations>
<description>
{shape}
<dynamic_text x="12" y="-6" z="25" text_width="-1" Halignment="AlignLeft" \
Valignment="AlignTop" frame="false" rotation="0" \
keep_visual_rotation="true" text_from="ElementInfo" \
uuid="{uid("dev", name, "t")}" \
font="Liberation Sans,9,-1,5,50,0,0,0,0,0,Regular"><text></text>\
<info_name>label</info_name></dynamic_text>
<terminal x="0" y="-20" name="1" orientation="n" type="Generic" uuid="{ids[0]}"/>
<terminal x="0" y="20" name="2" orientation="s" type="Generic" uuid="{ids[1]}"/>
</description></definition>"""


DEVICES = {
    "sensor": device(
        "sensor", "Sensor (generic)",
        '<rect x="-8" y="-20" width="16" height="40" rx="0" ry="0" '
        'style="line-style:normal;line-weight:normal;filling:none;'
        'color:black" antialias="false"/>'),
    "lamp": device(
        "lamp", "Lamp (generic)",
        '<ellipse x="-10" y="-10" width="20" height="20" '
        'style="line-style:normal;line-weight:normal;filling:none;'
        'color:black" antialias="false"/>'),
}


class Elmt:
    """What a placed element needs from its definition."""

    def __init__(self, category, name, root=None):
        self.path = f"embed://import/{category}/{name}.elmt"
        self.name = name
        self.category = category
        self.root = root
        self.terminals = {}
        if root is None:
            root = ET.fromstring(DEVICES[name])
        for t in root.iter("terminal"):
            self.terminals[t.get("name")] = t
        self.infos = {}
        info = root.find("elementInformations")
        if info is not None:
            for i in info:
                self.infos[i.get("name")] = i.text or ""
        self.texts = list(root.iter("dynamic_text"))
        self.plc = root.find("kindInformations/plcMasterData")
        self.link = root.get("link_type")


ELEMENTS = {}


def exists(category, name):
    try:
        family(name)
    except KeyError:
        return False
    return True


def family(name):
    """The folder of a card, such as el1xxx_digital_in/el1004."""
    for entry in sorted(os.listdir(ELMT_DIR)):
        folder = os.path.join(ELMT_DIR, entry)
        if not os.path.isdir(folder):
            continue
        for card in sorted(os.listdir(folder)):
            if os.path.exists(os.path.join(folder, card, name + ".elmt")):
                return CAT + "/" + entry + "/" + card
    raise KeyError(name)


def get(category, name):
    if category.startswith(CAT):
        category = family(name)
    key = (category, name)
    if key not in ELEMENTS:
        ELEMENTS[key] = Elmt(category, name,
                             load(category, name)
                             if category.startswith(CAT) else None)
    return ELEMENTS[key]


def esc(text):
    return quoteattr(text)[1:-1]


# Name of the folios by card category (the family folders of the library).
CATEGORIES = {
    "ek_couplers": "Coupleurs",
    "el1xxx_digital_in": "Entrées TOR",
    "el2xxx_digital_out": "Sorties TOR",
    "el3xxx_analog_in": "Entrées analogiques",
    "el4xxx_analog_out": "Sorties analogiques",
    "el5xxx_encoder": "Codeurs",
    "el9xxx_system": "Alimentation",
    "epp_box": "EtherCAT P (EPP)",
    "twinsafe": "TwinSAFE",
    "drives_af1000": "Variateurs AF1000 (VAR)",
    "drives_ax5000": "Variateurs AX5000 (VAR)",
}
VIEWS = {"sym": "symboles", "real": "vues réalistes",
         "desc": "vues descriptives", "table": "tableaux d'E/S"}


def island_title(label, names, view):
    """"A1 - Coupleurs / Entrées TOR / ... - symboles"."""
    cats = []
    for name in names:
        cat = CATEGORIES[family(name).split("/")[-2]]
        if cat not in cats:
            cats.append(cat)
    return f"{label} - {' / '.join(cats)} - {VIEWS[view]}"


# Signal given to the wired channels: (card, signal) -> function text.
# On the EL1008 the contact 2 carries the channel I3 (Beckhoff order).
USED = {
    ("A1.10", "I1"): "Presence sensor 1",
    ("A1.10", "I3"): "Presence sensor 2",
    ("A1.20", "O1"): "Run lamp",
    ("A2.10", "SI1"): "Emergency stop, channel 1",
    ("A2.10", "SI3"): "Emergency stop, channel 2",
    ("A2.20", "IN1"): "Drive enable",
}
CARDS = [
    # label, element base name, x on the folio
    ("A1.10", "el1008", 450),
    ("A1.20", "el2008", 600),
    ("A1.30", "el5152", 750),
    ("A1.40", "el9410", 900),
]
HEAD = ("A1.00", "ek1100")
A1_NAMES = [HEAD[1]] + [name for _, name, _ in CARDS]
CARD_Y = 250
SAFE = CAT + "/twinsafe"
AF = CAT + "/drives_af1000"
AX = CAT + "/drives_ax5000"
# Islands with the safety card and the drives: label, category, name
ISLAND2 = ("A2.00", [("A2.10", SAFE, "el1918"), ("A2.20", AF, "af1103"),
                     ("A2.30", AF, "af1215")])
ISLAND3 = ("A3.00", [("A3.10", AX, "ax5103"), ("A3.20", AX, "ax5203")])
# EtherCAT P island: the EK1322 junction heads an EPP box
EPP = CAT + "/epp_box"
ISLAND4 = ("A4.00", [("A4.10", EPP, "epp2338")], "ek1322")


def plc_xml(el, label):
    """The PLC data of an instance, with the used channels described."""
    if el.plc is None:
        return ""
    node = copy.deepcopy(el.plc)
    for io in node.iter("plcIO"):
        text = USED.get((label, io.get("address")))
        if text:
            io.set("functionText", text)
    return ET.tostring(node, encoding="unicode")


def texts_xml(el, label, key):
    """The dynamic texts of the definition, as an instance carries them."""
    out = ["<dynamic_texts>"]
    for n, dt in enumerate(el.texts):
        keep = ("Halignment", "Valignment", "font", "frame",
                "keep_visual_rotation", "rotation", "text_from",
                "text_width", "x", "y")
        attrs = " ".join(f'{k}="{esc(dt.get(k, ""))}"' for k in keep
                         if dt.get(k) is not None)
        info = dt.findtext("info_name") or ""
        text = label if info == "label" else (dt.findtext("text") or "")
        out.append(
            f'<dynamic_elmt_text {attrs} rotation_point_center="false" '
            f'uuid="{uid("t", key, str(n))}"><text>{esc(text)}</text>' +
            (f"<info_name>{esc(info)}</info_name>" if info else "") +
            "</dynamic_elmt_text>")
    out.append("</dynamic_texts>")
    return "".join(out)


def element_xml(el, label, x, y, key, prefix=""):
    tags = ["<terminals>"]
    for i, t in enumerate(el.terminals.values()):
        tags.append(
            f'<terminal id="{i}" orientation="{t.get("orientation")}" '
            f'x="{t.get("x")}" y="{t.get("y")}"/>')
    tags.append("</terminals><inputs/>")
    infos = [f'<elementInformation name="label" show="1">{esc(label)}'
             "</elementInformation>"]
    names = ["manufacturer", "manufacturer_reference", "designation"]
    names += [n for n in el.infos if "auxiliary" in n]
    for name in names:
        if el.infos.get(name):
            infos.append(
                f'<elementInformation name="{name}" show="0">'
                f"{esc(el.infos[name])}</elementInformation>")
    return (
        f'<element freezeLabel="false" is_movable="1" orientation="0" '
        f'prefix="{prefix}" type="{el.path}" uuid="{uid("e", key)}" '
        f'x="{x}" y="{y}" z="10">' + "".join(tags) +
        "<elementInformations>" + "".join(infos) + "</elementInformations>"
        + plc_xml(el, label) + texts_xml(el, label, key) +
        "<texts_groups/></element>")


def conductor_xml(key, e1, l1, el1, t1, e2, l2, el2, t2):
    tu1 = el1.terminals[t1].get("uuid")
    tu2 = el2.terminals[t2].get("uuid")
    return (
        '<conductor bicolor="false" bus="" cable="" color2="#000000" '
        'condsize="1" conductor_color="" conductor_section="" '
        'dash-size="1" displaytext="1" '
        f'element1="{uid("e", e1)}" element1_label="{esc(l1)}" '
        'element1_linked="" element1_name="" '
        f'element2="{uid("e", e2)}" element2_label="{esc(l2)}" '
        'element2_linked="" element2_name="" formula="" '
        'freezeLabel="false" function="" horizontal-alignment="AlignBottom" '
        'horizrotatetext="0" num="_" numsize="7" onetextperfolio="0" '
        f'tension_protocol="" terminal1="{tu1}" terminal2="{tu2}" '
        f'terminalname1="{t1}" terminalname2="{t2}" text_color="#000000" '
        f'type="multi" uuid="{uid("c", key)}" '
        'vertical-alignment="AlignRight" vertirotatetext="270" x="0" y="0">'
        "<sequentialNumbers/></conductor>")


DEFCOND = (
    'bicolor="false" bus="" cable="" color2="#000000" condsize="1" '
    'conductor_color="" conductor_section="" dash-size="1" displaytext="1" '
    'formula="" function="" horizontal-alignment="AlignBottom" '
    'horizrotatetext="0" num="_" numsize="7" onetextperfolio="0" '
    'tension_protocol="" text_color="#000000" type="multi" '
    'vertical-alignment="AlignRight" vertirotatetext="270"')


def titleblock_templates():
    """The A3 title block templates of the titleblocks folder, embedded so
    that the example does not depend on an installed library."""
    xml = ""
    for name in TITLEBLOCKS.values():
        with open(os.path.join(TB_DIR, name + ".titleblock"),
                  encoding="utf-8") as f:
            root = ET.fromstring(f.read())
        xml += ET.tostring(root, encoding="unicode")
    return "<titleblocktemplates>" + xml + "</titleblocktemplates>"


def diagram_xml(order, title, elements, conductors, portrait=False):
    size = "A3 portrait" if portrait else "A3 landscape"
    cols, colsize, rows, rowsize = gen_titleblocks.FOLIO_SIZES[size]
    return (
        f'<diagram author="" auto_page_num="" cols="{cols}" '
        f'colsize="{colsize}" '
        'date="null" displayAt="bottom" displaycols="true" '
        'displayrows="true" filename="" folio="%id/%total" '
        f'freezeNewConductor="false" freezeNewElement="false" '
        f'height="{rows * rowsize + 20}" '
        f'indexrev="" locmach="" order="{order}" plant="" rows="{rows}" '
        f'rowsize="{rowsize}" title="{esc(title)}" '
        f'titleblocktemplate="{size}" '
        'titleblocktemplateCollection="embedded" '
        f'uuid="{uid("d", str(order))}" '
        f'version="{VERSION}-dev"><defaultconductor {DEFCOND}/>'
        "<elements>" + "".join(elements) + "</elements><conductors>" +
        "".join(conductors) + "</conductors><inputs/></diagram>")


def folio_symbols():
    els = []
    head = get(CAT, HEAD[1])
    els.append(element_xml(head, HEAD[0], 200, CARD_Y,
                           f"sym/{HEAD[0]}"))
    conductors = []
    for label, name, x in CARDS:
        els.append(element_xml(get(CAT, name), label, x, CARD_Y,
                               f"sym/{label}"))
    sensor, lamp = get(DEV, "sensor"), get(DEV, "lamp")
    for n, (label, dx) in enumerate((("B1", -80), ("B2", -120))):
        els.append(element_xml(sensor, label, 450 + dx, 340,
                               f"sym/{label}"))
        conductors.append(conductor_xml(
            f"sym/{label}", "sym/A1.10", "A1.10", get(CAT, "el1008"),
            str(n + 1), f"sym/{label}", label, sensor, "1"))
    els.append(element_xml(lamp, "H1", 560, 340, "sym/H1"))
    conductors.append(conductor_xml(
        "sym/H1", "sym/A1.20", "A1.20", get(CAT, "el2008"), "1",
        "sym/H1", "H1", lamp, "1"))
    return diagram_xml(1, island_title("A1", A1_NAMES, "sym"), els,
                       conductors)


def folio_realistic():
    els = []
    for label, name, x in CARDS:
        els.append(element_xml(get(CAT, name + "_realistic"), label,
                               x - 300, 230, f"real/{label}"))
    return diagram_xml(2, island_title("A1", A1_NAMES, "real"), els, [])


def folio_description():
    els = []
    x = 60
    for label, name, _ in CARDS:
        el = get(CAT, name + "_description")
        hx = int(el.root.get("hotspot_x"))
        els.append(element_xml(el, label, x + hx, 230, f"desc/{label}"))
        x += int(el.root.get("width")) + 40
    return diagram_xml(3, island_title("A1", A1_NAMES, "desc"), els, [])


def folio_tables():
    els = []
    for n, (label, name, x) in enumerate(CARDS):
        els.append(element_xml(get(CAT, name + "_table"), label, 60,
                               60 + 190 * n, f"table/{label}"))
    return diagram_xml(4, island_title("A1", A1_NAMES, "table"), els, [],
                       portrait=True)


def island_folios(first, head_label, cards, head="ek1100"):
    """Symbols, realistic views and I/O tables of an island of drives or
    safety cards; first is the order of its first folio."""
    els = [element_xml(get(CAT, head), head_label, 120, 320,
                       f"sym/{head_label}")]
    conductors = []
    for n, (label, cat, name) in enumerate(cards):
        els.append(element_xml(get(cat, name), label, 360 + 230 * n,
                               {"el1918": 175, "epp2338": 230}.get(name, 70),
                               f"sym/{label}"))
    sensor = get(DEV, "sensor")
    wired = []
    if cards[0][2] == "el1918":
        wired = [("B3", 250, "A2.10", get(SAFE, "el1918"), ("2", "4"))]
    if cards[1:] and cards[1][2] == "af1103":
        wired.append(("B4", 450, "A2.20", get(AF, "af1103"), ("IN1",)))
    for label, x, card, el, pins in wired:
        els.append(element_xml(sensor, label, x, 400, f"sym/{label}"))
        for i, pin in enumerate(pins):
            conductors.append(conductor_xml(
                f"sym/{label}/{i}", f"sym/{card}", card, el, pin,
                f"sym/{label}", label, sensor, str(i + 1)))
    names = [head] + [name for _, _, name in cards]
    folios = [diagram_xml(first, island_title(head_label[:2], names, "sym"),
                          els, conductors)]
    here = [(label, cat, name) for label, cat, name in cards
            if exists(cat, name + "_realistic")]
    if exists(CAT, head + "_realistic"):
        here.insert(0, (head_label, CAT, head))
    if here:
        els = [element_xml(get(cat, name + "_realistic"), label,
                           60 + 230 * n, 220, f"realistic/{label}")
               for n, (label, cat, name) in enumerate(here)]
        folios.append(diagram_xml(
            first + len(folios),
            island_title(head_label[:2], names, "real"), els, []))
    els = [element_xml(get(cat, name + "_table"), label,
                       60 + 480 * (n % 2), 60 + 320 * (n // 2),
                       f"table/{label}")
           for n, (label, cat, name) in enumerate(cards)]
    folios.append(diagram_xml(
        first + len(folios), island_title(head_label[:2], names, "table"),
        els, []))
    return folios


def catalogue_folios(first):
    """One or more folios per card category with the symbol of EVERY card
    of the library, in rows, the reference under each symbol."""
    folios = []
    left, top, right, bottom = 40, 30, 1190, 730
    for entry in sorted(os.listdir(ELMT_DIR)):
        folder = os.path.join(ELMT_DIR, entry)
        if not os.path.isdir(folder):
            continue
        cards = [c for c in sorted(os.listdir(folder))
                 if os.path.exists(os.path.join(folder, c, c + ".elmt"))]
        pages, els = [], []
        x, y, row = left, top, 0
        for card in cards:
            el = get(CAT + "/" + entry + "/" + card, card)
            w = int(el.root.get("width"))
            h = int(el.root.get("height"))
            if x + w > right and x > left:
                x, y, row = left, y + row + 40, 0
            if y + h + 20 > bottom and els:
                pages.append(els)
                els, x, y, row = [], left, top, 0
            els.append(element_xml(
                el, card.upper(), x + int(el.root.get("hotspot_x")),
                y + int(el.root.get("hotspot_y")), f"catalogue/{card}"))
            x += w + 30
            row = max(row, h)
        pages.append(els)
        for n, page in enumerate(pages):
            title = f"Catalogue - {CATEGORIES[entry]}"
            if len(pages) > 1:
                title += f" ({n + 1}/{len(pages)})"
            folios.append(diagram_xml(first + len(folios), title, page, []))
    return folios


def category_names(path):
    sub = path[len(CAT):].strip("/")
    names = ET.parse(os.path.join(ELMT_DIR, *(sub.split("/") if sub else []),
                                  "qet_directory")).getroot()
    return [(n.get("lang"), n.text) for n in names.iter("name")]


def tree_xml(path):
    inner = ""
    for (category, name), el in sorted(ELEMENTS.items()):
        if category == path:
            inner += (f'<element name="{name}.elmt">' +
                      ET.tostring(el.root, encoding="unicode") +
                      "</element>")
    subs = sorted({c[len(path) + 1:].split("/")[0]
                   for c, _ in ELEMENTS if c.startswith(path + "/")})
    for s in subs:
        inner += tree_xml(path + "/" + s)
    return category_xml(path.split("/")[-1], category_names(path), inner)


def category_xml(name, names, inner):
    ns = "".join(f'<name lang="{lang}">{esc(text)}</name>'
                 for lang, text in names)
    return f'<category name="{name}"><names>{ns}</names>{inner}</category>'


def collection_xml():
    beckhoff = category_xml(
        "20_manufacturers_articles",
        [("en", "Manufacturers articles"), ("fr", "Articles constructeurs")],
        tree_xml(CAT))
    beckhoff = category_xml(
        "10_electric", [("en", "Electric"), ("fr", "Electrique")], beckhoff)
    inner = ""
    for name in sorted(DEVICES):
        inner += f'<element name="{name}.elmt">{DEVICES[name]}</element>'
    devices = category_xml(DEV, [("en", "Example devices")], inner)
    return ('<collection><category name="import"><names><name lang="en">'
            "Imported elements</name></names>" + beckhoff + devices +
            "</category></collection>")


def main():
    folios = [folio_symbols(), folio_realistic(), folio_description(),
              folio_tables()]
    folios += island_folios(5, *ISLAND2)
    folios += island_folios(8, *ISLAND3)
    folios += island_folios(11, *ISLAND4[:2], head=ISLAND4[2])
    folios += catalogue_folios(len(folios) + 1)
    xrefs = "".join(
        f'<xref delayprefix="" displayhas="cross" master_label="%f-%l%c" '
        'offset="0" powerprefix="" showallconfiguredslaves="false" '
        'showpowerctc="true" showterminalname="true" '
        'slave_label="(%f-%l%c)" slave_offset="0" snapto="label" '
        f'stackoverlapping="true" switchprefix="" type="{t}" '
        'xrefpos="AlignBottom"/>'
        for t in ("cable", "coil", "commutator", "plc", "protection"))
    head = (
        f'<project title="Beckhoff EtherCAT island example" '
        f'uuid="{uid("project")}" version="{VERSION}"><properties/>'
        + titleblock_templates() +
        '<usage enabled="true" time_spent="0"/><newdiagrams>'
        '<border cols="20" colsize="61" displaycols="true" '
        'displayrows="true" rows="10" rowsize="80"/>'
        '<inset author="" auto_page_num="" date="null" displayAt="bottom" '
        'filename="" folio="" indexrev="" locmach="" plant="" title="" '
        'titleblocktemplate="A3 landscape" '
        'titleblocktemplateCollection="embedded" version=""/>'
        f"<conductors {DEFCOND}/><report label=\"%f-%l%c\"/>"
        f"<xrefs>{xrefs}</xrefs>"
        '<conductors_autonums auto_break_conductors="false" '
        'current_autonum="" freeze_new_conductors="false"/><folio_autonums/>'
        '<element_autonums current_autonum="" freeze_new_elements="false"/>'
        "<guides/></newdiagrams>")
    text = ('<?xml version="1.0" encoding="UTF-8"?>\n' + head +
            "".join(folios) + collection_xml() + "</project>\n")
    ET.fromstring(text.encode("utf-8"))
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print("wrote", os.path.relpath(OUT, ROOT))


if __name__ == "__main__":
    main()
