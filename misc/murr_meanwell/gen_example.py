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
"""Write examples/murr_meanwell_example.qet.

Folio 1 is a wired schematic: a Mean Well NDR-240-24 feeds a MICO Basic 4.4
that protects three lamps and a Mean Well SDR-120-24 feeds a MICO Pro with
two channels, its DC OK relay enabling the breaker. Folios 2 to 4 show the
same four devices as realistic views, description views and pin tables,
with the same labels; folios 5 and 6 show the schematic of every product
of the two libraries. Folio 7 links an EPP2338-0002 and a MICO output by
dashed conductors to M12 moulded cables, a Y distributor, a T coupler and
valve coils; folio 8 shows every M12 product. The elements of
extra-elements/16_murrelektronik_mico, 17_meanwell_psu,
18_murrelektronik_m12 and the EPP2338 of 15_beckhoff_ethercat are embedded
in the project.

Usage: python misc/murr_meanwell/gen_example.py
The output is deterministic (all uuids are derived from fixed names). The
title block templates and the folio sizes come from misc/beckhoff and
titleblocks, as for the Beckhoff example.
"""
import copy
import os
import sys
import uuid
import xml.etree.ElementTree as ET
from xml.sax.saxutils import quoteattr

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "beckhoff"))
import gen_elements  # noqa: E402
import gen_m12  # noqa: E402
import gen_titleblocks  # noqa: E402

ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
EXTRA_ROOTS = ["15_beckhoff_ethercat", "18_murrelektronik_m12"]
TB_DIR = os.path.join(ROOT, "titleblocks")
TITLEBLOCKS = {"A3 landscape": "A3_landscape", "A3 portrait": "A3_portrait"}
EXTRA = gen_elements.EXTRA
OUT = os.path.join(ROOT, "examples", "murr_meanwell_example.qet")
NS = uuid.UUID("c4f2a0de-52d1-4b6e-9d3a-7e8c1b05f6a2")
DEV = "example_devices"
VERSION = "0.200.1"


def uid(*parts):
    return "{" + str(uuid.uuid5(NS, "/".join(parts))) + "}"


def esc(text):
    return quoteattr(text)[1:-1]


LAMP = f"""<definition version="0.100.0" type="element" \
link_type="simple" width="30" height="50" hotspot_x="15" hotspot_y="25">
<uuid uuid="{uid("dev", "lamp")}"/>
<names><name lang="en">Lamp (generic)</name></names>
<informations>Generic example symbol. License: see \
http://qelectrotech.org/wiki/doc/elements_license</informations>
<description>
<ellipse x="-10" y="-10" width="20" height="20" \
style="line-style:normal;line-weight:normal;filling:none;color:black" \
antialias="false"/>
<dynamic_text x="12" y="-6" z="25" text_width="-1" Halignment="AlignLeft" \
Valignment="AlignTop" frame="false" rotation="0" \
keep_visual_rotation="true" text_from="ElementInfo" \
uuid="{uid("dev", "lamp", "t")}" \
font="Liberation Sans,9,-1,5,50,0,0,0,0,0,Regular"><text></text>\
<info_name>label</info_name></dynamic_text>
<terminal x="0" y="-20" name="1" orientation="n" type="Generic" \
uuid="{uid("dev", "lamp", "1")}"/>
<terminal x="0" y="20" name="2" orientation="s" type="Generic" \
uuid="{uid("dev", "lamp", "2")}"/>
</description></definition>"""


class Elmt:
    """What a placed element needs from its definition."""

    def __init__(self, category, name, root):
        self.path = f"embed://import/{category}/{name}.elmt"
        self.name, self.category, self.root = name, category, root
        self.terminals = {t.get("name"): t for t in root.iter("terminal")}
        self.infos = {}
        info = root.find("elementInformations")
        if info is not None:
            for i in info:
                self.infos[i.get("name")] = i.text or ""
        self.texts = list(root.iter("dynamic_text"))
        self.plc = root.find("kindInformations/plcMasterData")


ELEMENTS = {}


def get(device, view=""):
    """The element of a product (view is the suffix of the appearance)."""
    category = os.path.relpath(device.directory(), EXTRA).replace(
        os.sep, "/")
    name = device.folder + view
    key = (category, name)
    if key not in ELEMENTS:
        ELEMENTS[key] = Elmt(category, name,
                             ET.parse(device.path(view)).getroot())
    return ELEMENTS[key]


def get_file(path):
    """The element stored in a .elmt file under extra-elements."""
    rel = os.path.relpath(path, EXTRA).replace(os.sep, "/")
    category, name = rel.rsplit("/", 1)
    name = name[:-len(".elmt")]
    key = (category, name)
    if key not in ELEMENTS:
        ELEMENTS[key] = Elmt(category, name, ET.parse(path).getroot())
    return ELEMENTS[key]


def lamp():
    key = (DEV, "lamp")
    if key not in ELEMENTS:
        ELEMENTS[key] = Elmt(DEV, "lamp", ET.fromstring(LAMP))
    return ELEMENTS[key]


# Function text of the wired pins: (label, terminal) -> text.
USED = {
    ("F1", "OUT1"): "Lamp H1", ("F1", "OUT2"): "Lamp H2",
    ("F1", "OUT3"): "Lamp H3", ("A2", "13"): "Enable F2",
    ("F2", "CTRL"): "Enable from A2", ("F2", "OUT1"): "Lamp H4",
    ("F2", "OUT2"): "Lamp H5",
}


def plc_xml(el, label):
    if el.plc is None:
        return ""
    node = copy.deepcopy(el.plc)
    for io in node.iter("plcIO"):
        text = USED.get((label, io.findtext("terminal")))
        if text:
            io.set("functionText", text)
    return ET.tostring(node, encoding="unicode")


def texts_xml(el, label, key):
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


def element_xml(el, label, x, y, key):
    tags = ["<terminals>"]
    for i, t in enumerate(el.terminals.values()):
        tags.append(
            f'<terminal id="{i}" orientation="{t.get("orientation")}" '
            f'x="{t.get("x")}" y="{t.get("y")}"/>')
    tags.append("</terminals><inputs/>")
    infos = [f'<elementInformation name="label" show="1">{esc(label)}'
             "</elementInformation>"]
    for name in ("manufacturer", "manufacturer_reference", "designation"):
        if el.infos.get(name):
            infos.append(
                f'<elementInformation name="{name}" show="0">'
                f"{esc(el.infos[name])}</elementInformation>")
    return (
        f'<element freezeLabel="false" is_movable="1" orientation="0" '
        f'prefix="" type="{el.path}" uuid="{uid("e", key)}" '
        f'x="{x}" y="{y}" z="10">' + "".join(tags) +
        "<elementInformations>" + "".join(infos) + "</elementInformations>"
        + plc_xml(el, label) + texts_xml(el, label, key) +
        "<texts_groups/></element>")


def conductor_xml(key, e1, l1, el1, t1, e2, l2, el2, t2, dashed=False):
    tu1 = el1.terminals[t1].get("uuid")
    tu2 = el2.terminals[t2].get("uuid")
    style = 'style="line-style: dashed;" ' if dashed else ""
    return (
        f'<conductor {style}'
        'bicolor="false" bus="" cable="" color2="#000000" '
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
    xml = ""
    for name in TITLEBLOCKS.values():
        with open(os.path.join(TB_DIR, name + ".titleblock"),
                  encoding="utf-8") as f:
            xml += ET.tostring(ET.fromstring(f.read()), encoding="unicode")
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


def by_ref(ref):
    return next(d for d in gen_elements.all_devices() if d.ref == ref)


# The four devices of the wired folio: label, product, x, y
A1, F1 = ("A1", by_ref("NDR-240-24"), 100, 80), \
    ("F1", by_ref("MICO Basic 4.4"), 100, 300)
A2, F2 = ("A2", by_ref("SDR-120-24"), 600, 80), \
    ("F2", by_ref("MICO Pro 2 channels"), 600, 300)
DEVICES = (A1, F1, A2, F2)


def folio_wired():
    els, cond = [], []
    for label, dev, x, y in DEVICES:
        els.append(element_xml(get(dev), label, x, y, f"sym/{label}"))
    lm = lamp()
    wires = [("A1", "+V", "F1", "+24V"), ("A1", "-V", "F1", "0V"),
             ("A2", "+V", "F2", "+24V"), ("A2", "-V", "F2", "0V"),
             ("A2", "13", "F2", "CTRL")]
    parts = {label: (get(dev), label) for label, dev, _, _ in DEVICES}
    for a, ta, b, tb in wires:
        cond.append(conductor_xml(
            f"sym/{a}{ta}{b}{tb}", f"sym/{a}", a, parts[a][0], ta,
            f"sym/{b}", b, parts[b][0], tb))
    loads = [("H1", "F1", "OUT1", 110), ("H2", "F1", "OUT2", 180),
             ("H3", "F1", "OUT3", 250), ("H4", "F2", "OUT1", 610),
             ("H5", "F2", "OUT2", 680)]
    for label, f, pin, x in loads:
        els.append(element_xml(lm, label, x, 470, f"sym/{label}"))
        cond.append(conductor_xml(
            f"sym/{label}", f"sym/{f}", f, parts[f][0], pin,
            f"sym/{label}", label, lm, "1"))
    return diagram_xml(1, "Wired example - power supplies and MICO "
                       "breakers", els, cond)


def folio_views(order, view, title, portrait=False):
    els = []
    x = 60
    for label, dev, _, _ in DEVICES:
        el = get(dev, view)
        hx = int(el.root.get("hotspot_x"))
        hy = int(el.root.get("hotspot_y"))
        els.append(element_xml(el, label, x + hx, 60 + hy,
                               f"{view}/{label}"))
        x += int(el.root.get("width")) + 30
    return diagram_xml(order, title, els, [], portrait)


def folio_tables():
    els = []
    for n, (label, dev, _, _) in enumerate(DEVICES):
        els.append(element_xml(get(dev, "_table"), label,
                               60 + 380 * (n % 2), 60 + 260 * (n // 2),
                               f"table/{label}"))
    return diagram_xml(4, "Example - pin tables", els, [])


def folio_m12():
    """EPP2338 and MICO outputs linked by dotted lines to M12 products."""
    epp = get_file(os.path.join(EXTRA, "15_beckhoff_ethercat", "epp_box",
                                "epp2338.elmt"))
    mico = get(by_ref("MICO Basic 4.4"))
    m12 = {p.file: get_file(p.path()) for p in gen_m12.products()}
    els, cond = [], []

    def put(el, label, x, y):
        els.append(element_xml(el, label, x, y, f"m12/{label}"))

    def link(a, ea, ta, b, eb, tb):
        cond.append(conductor_xml(f"m12/{a}{ta}{b}{tb}", f"m12/{a}", a, ea,
                                  ta, f"m12/{b}", b, eb, tb, dashed=True))

    put(epp, "A3", 100, 250)
    put(m12["m12_cable_1m"], "W1", 250, 180)
    put(m12["m12_y_0_6m"], "X1", 550, 180)
    put(m12["valve_coil_bi_18"], "Y1", 850, 120)
    put(m12["valve_coil_bi_18"], "Y2", 850, 240)
    link("A3", epp, "X01", "W1", m12["m12_cable_1m"], "X1")
    link("W1", m12["m12_cable_1m"], "X2", "X1", m12["m12_y_0_6m"], "X1")
    link("X1", m12["m12_y_0_6m"], "X2", "Y1", m12["valve_coil_bi_18"], "X1")
    link("X1", m12["m12_y_0_6m"], "X3", "Y2", m12["valve_coil_bi_18"], "X1")
    put(m12["m12_cable_2m"], "W2", 250, 320)
    put(m12["m12_t_coupler"], "X2", 550, 320)
    put(m12["m12_connector_male"], "X3", 850, 320)
    link("A3", epp, "X02", "W2", m12["m12_cable_2m"], "X1")
    link("W2", m12["m12_cable_2m"], "X2", "X2", m12["m12_t_coupler"], "X1")
    link("X2", m12["m12_t_coupler"], "X2", "X3",
         m12["m12_connector_male"], "X1")
    put(mico, "F1", 100, 560)
    put(m12["m12_cable_5m"], "W3", 250, 730)
    put(m12["valve_coil_bi_18"], "Y3", 550, 730)
    link("F1", mico, "OUT1", "W3", m12["m12_cable_5m"], "X1")
    link("W3", m12["m12_cable_5m"], "X2", "Y3", m12["valve_coil_bi_18"],
         "X1")
    return diagram_xml(7, "M12 links - dotted conductors between EPP2338, "
                       "MICO, moulded cables and valve coils", els, cond)


def folio_m12_range(order, title):
    els = []
    x, y, rowh = 60, 60, 0
    for p in gen_m12.products():
        el = get_file(p.path())
        w, h = int(el.root.get("width")), int(el.root.get("height"))
        if x + w > 1180:
            x, y, rowh = 60, y + rowh + 20, 0
        els.append(element_xml(
            el, p.order or "coil", x + int(el.root.get("hotspot_x")),
            y + int(el.root.get("hotspot_y")), f"m12range/{p.file}"))
        x += w + 20
        rowh = max(rowh, h)
    return diagram_xml(order, title, els, [])


def folio_range(order, title, devices):
    els = []
    x, y, rowh = 60, 60, 0
    for dev in devices:
        el = get(dev)
        w, h = int(el.root.get("width")), int(el.root.get("height"))
        if x + w > 1180:
            x, y, rowh = 60, y + rowh + 20, 0
        els.append(element_xml(el, dev.ref, x + int(el.root.get(
            "hotspot_x")), y + int(el.root.get("hotspot_y")),
            f"range/{dev.ref}"))
        x += w + 20
        rowh = max(rowh, h)
    return diagram_xml(order, title, els, [])


def category_names(path):
    names = ET.parse(os.path.join(EXTRA, *path.split("/"),
                                  "qet_directory")).getroot()
    return [(n.get("lang"), n.text) for n in names.iter("name")]


def category_xml(name, names, inner):
    ns = "".join(f'<name lang="{lang}">{esc(text)}</name>'
                 for lang, text in names)
    return f'<category name="{name}"><names>{ns}</names>{inner}</category>'


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


def collection_xml():
    libs = "".join(tree_xml(r) for r in
                   list(gen_elements.ROOTS) + EXTRA_ROOTS)
    devices = category_xml(
        DEV, [("en", "Example devices")],
        f'<element name="lamp.elmt">{LAMP}</element>')
    return ('<collection><category name="import"><names><name lang="en">'
            "Imported elements</name></names>" + libs + devices +
            "</category></collection>")


def main():
    folios = [folio_wired(),
              folio_views(2, "_realistic", "Example - realistic views"),
              folio_views(3, "_description", "Example - description views"),
              folio_tables(),
              folio_range(5, "Range - MICO circuit breakers",
                          gen_elements.MICOS),
              folio_range(6, "Range - Mean Well power supplies",
                          gen_elements.PSUS),
              folio_m12(),
              folio_m12_range(8, "Range - Murrelektronik M12 products")]
    xrefs = "".join(
        f'<xref delayprefix="" displayhas="cross" master_label="%f-%l%c" '
        'offset="0" powerprefix="" showallconfiguredslaves="false" '
        'showpowerctc="true" showterminalname="true" '
        'slave_label="(%f-%l%c)" slave_offset="0" snapto="label" '
        f'stackoverlapping="true" switchprefix="" type="{t}" '
        'xrefpos="AlignBottom"/>'
        for t in ("cable", "coil", "commutator", "plc", "protection"))
    head = (
        f'<project title="Murrelektronik MICO and Mean Well example" '
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
