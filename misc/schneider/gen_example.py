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
"""Write examples/schneider_example.qet.

A catalogue of the Schneider Electric library of
extra-elements/10_electric/20_manufacturers_articles/schneider: the IEC
symbols, the front views of the modular devices, of the ComPacT NSX and
INS, and an NSX250 with its accessories (motor mechanism, auxiliary
contact, undervoltage release, terminal shields, handle).  The accessories
carry the label of the device followed by a separator (-QS1.1, -QS1-OF...)
and misc/schneider/qet_accessory_articles.py gives them to the device as
articles, which is what the example shows.

Usage: python misc/schneider/gen_example.py
The output is deterministic (all uuids are derived from fixed names).
"""
import os
import sys
import uuid
import xml.etree.ElementTree as ET
from xml.sax.saxutils import quoteattr

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "misc", "beckhoff"))
import gen_schneider as gs  # noqa: E402
import gen_titleblocks  # noqa: E402
import qet_accessory_articles as tool  # noqa: E402

TB_DIR = os.path.join(ROOT, "titleblocks")
CAT = "10_electric/20_manufacturers_articles/schneider"
ELMT_DIR = os.path.join(ROOT, "extra-elements", *CAT.split("/"))
OUT = os.path.join(ROOT, "examples", "schneider_example.qet")
NS = uuid.UUID("7f2c5b8e-3a41-4d6b-9c0e-1b8d2a6f4e93")
VERSION = "0.200.1"
TITLEBLOCKS = {"A3 landscape": "A3_landscape"}


def uid(*parts):
    return "{" + str(uuid.uuid5(NS, "/".join(parts))) + "}"


def esc(text):
    return quoteattr(text)[1:-1]


class Elmt:
    def __init__(self, folder, stem, suffix):
        self.folder, self.stem, self.suffix = folder, stem, suffix
        self.path = "embed://import/%s/%s/%s/%s%s.elmt" % (
            CAT, folder, stem, stem, suffix)
        self.file = os.path.join(ELMT_DIR, folder, stem,
                                 stem + suffix + ".elmt")
        self.root = ET.parse(self.file).getroot()
        self.terminals = {t.get("name"): t for t in
                          self.root.iter("terminal")}
        self.infos = [(i.get("name"), i.get("show", "0"), i.text or "")
                      for i in self.root.find("elementInformations")]
        self.texts = list(self.root.iter("dynamic_text"))


CACHE = {}


def get(stem, suffix=""):
    product = {p.stem: p for p in gs.PRODUCTS}[stem]
    key = (stem, suffix)
    if key not in CACHE:
        CACHE[key] = Elmt(product.folder, stem, suffix)
    return CACHE[key]


def texts_xml(el, label, key):
    out = ["<dynamic_texts>"]
    for n, dt in enumerate(el.texts):
        keep = ("Halignment", "Valignment", "font", "frame",
                "keep_visual_rotation", "rotation", "text_from",
                "text_width", "x", "y")
        attrs = " ".join('%s="%s"' % (k, esc(dt.get(k)))
                         for k in keep if dt.get(k) is not None)
        out.append(
            '<dynamic_elmt_text %s rotation_point_center="false" '
            'uuid="%s"><text>%s</text><info_name>label</info_name>'
            "</dynamic_elmt_text>" % (attrs, uid("t", key, str(n)),
                                      esc(label)))
    out.append("</dynamic_texts>")
    return "".join(out)


def element_xml(el, label, x, y, key):
    tags = ["<terminals>"]
    for i, t in enumerate(el.terminals.values()):
        tags.append('<terminal id="%d" orientation="%s" x="%s" y="%s"/>' % (
            i, t.get("orientation"), t.get("x"), t.get("y")))
    tags.append("</terminals><inputs/>")
    infos = ['<elementInformation name="label" show="1">%s'
             "</elementInformation>" % esc(label)]
    for name, show, text in el.infos:
        if name != "label":
            infos.append('<elementInformation name="%s" show="%s">%s'
                         "</elementInformation>" % (name, show, esc(text)))
    return (
        '<element freezeLabel="false" is_movable="1" orientation="0" '
        'prefix="" type="%s" uuid="%s" x="%s" y="%s" z="10">' % (
            el.path, uid("e", key), x, y) + "".join(tags) +
        "<elementInformations>" + "".join(infos) + "</elementInformations>"
        + texts_xml(el, label, key) + "<texts_groups/></element>")


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


def diagram_xml(order, title, elements):
    cols, colsize, rows, rowsize = gen_titleblocks.FOLIO_SIZES[
        "A3 landscape"]
    return (
        '<diagram author="" auto_page_num="" cols="%d" colsize="%d" '
        'date="null" displayAt="bottom" displaycols="true" '
        'displayrows="true" filename="" folio="%%id/%%total" '
        'freezeNewConductor="false" freezeNewElement="false" height="%d" '
        'indexrev="" locmach="" order="%d" plant="" rows="%d" rowsize="%d" '
        'title="%s" titleblocktemplate="A3 landscape" '
        'titleblocktemplateCollection="embedded" uuid="%s" '
        'version="%s-dev"><defaultconductor %s/><elements>%s</elements>'
        "<conductors/><inputs/></diagram>" % (
            cols, colsize, rows * rowsize + 20, order, rows, rowsize,
            esc(title), uid("d", str(order)), VERSION, DEFCOND,
            "".join(elements)))


# ------------------------------------------------------------- the folios

def row(items, y, x0=80, gap=60, front=False):
    """Place the views left to right; items are (label, stem, suffix)."""
    els, x = [], x0
    for label, stem, suffix in items:
        el = get(stem, suffix)
        w = int(el.root.get("width"))
        hx = int(el.root.get("hotspot_x"))
        els.append(element_xml(el, label, x + hx, y, "%s/%s%s" % (
            label, stem, suffix)))
        x += w + gap
    return els


SYMBOLS = [
    ("-Q1", "ic60n_1p"), ("-Q2", "ic60n_2p"), ("-Q3", "ic60n_3p"),
    ("-Q4", "ic60n_4p"), ("-F1", "idpna_vigi_1pn"), ("-F2", "iid_2p"),
    ("-F3", "iid_4p"), ("-F4", "vigi_ic60_3p"), ("-S1", "isw_3p_40a"),
    ("-S2", "sti_2p_25a"), ("-Q5", "nsx100b_3p_tmd100"),
    ("-Q6", "nsx250n_3p_frame"), ("-Q7", "nsx250n_3p_tm250"),
    ("-S3", "ins250_3p"), ("-S4", "ins250_4p"),
]
MODULAR = [s for _, s in SYMBOLS[:10]]


def folio_symbols():
    els = []
    for n, chunk in enumerate((SYMBOLS[:8], SYMBOLS[8:])):
        els += row([(label, stem, "") for label, stem in chunk],
                   150 + 250 * n, gap=70)
    return diagram_xml(1, "Schneider Electric - IEC symbols", els)


def folio_modular():
    first = [(label, stem, "_realistic") for label, stem in SYMBOLS[:6]]
    second = [(label, stem, "_realistic") for label, stem in SYMBOLS[6:10]]
    return diagram_xml(2, "Acti9 modular devices - front views",
                       row(first, 100, gap=40) + row(second, 450, gap=60))


def folio_compact(order, title, items):
    return diagram_xml(order, title,
                       row([(l, s, "_realistic") for l, s in items], 120,
                           gap=60))


def folio_assembly():
    """The NSX250 with its accessories, symbols and linked labels."""
    parent = ("-QS1", "nsx250n_3p_tm250")
    kids = [("-QS1.1", "mt250_220_240vac"), ("-QS1-OF", "aux_of_sd_29450"),
            ("-QS1.3", "mn_24vdc")]
    els = row([(parent[0], parent[1], "")], 150, x0=100)
    els += row([(l, s, "") for l, s in kids], 150, x0=400, gap=80)
    els += row([("-QF1", "ic60n_3p", ""), ("-QF1-OF", "iof_a9a26924", ""),
                ("-QF1.1", "vigi_ic60_3p", "")], 400, x0=100, gap=80)
    return diagram_xml(5, "Accessories linked by label prefix", els)


def folio_assembly_front():
    els = row([("-QS1", "nsx250n_3p_tm250", "_accessories")], 200, x0=60)
    kids = [("-QS1.1", "mt250_220_240vac"), ("-QS1-OF", "aux_of_sd_29450"),
            ("-QS1.3", "mn_24vdc"), ("-QS1.4", "terminal_shield_lv429517")]
    x, y = 520, 100
    for n, (label, stem) in enumerate(kids):
        el = get(stem, "_realistic")
        w = int(el.root.get("width"))
        hx = int(el.root.get("hotspot_x"))
        hy = int(el.root.get("hotspot_y"))
        if n == 3:
            x, y = 520, 420
        els.append(element_xml(el, label, x + hx, y + hy,
                               "front/%s" % label))
        x += w + 30
    return diagram_xml(6, "NSX250 with accessories - front views", els)


# ------------------------------------------------------------- collection

def directory_names(path):
    root = ET.parse(os.path.join(path, "qet_directory")).getroot()
    return [(n.get("lang"), n.text) for n in root.iter("name")]


def category_xml(name, names, inner):
    ns = "".join('<name lang="%s">%s</name>' % (lang, esc(text))
                 for lang, text in names)
    return '<category name="%s"><names>%s</names>%s</category>' % (
        name, ns, inner)


def tree_xml(directory, used):
    inner = ""
    for entry in sorted(os.listdir(directory)):
        path = os.path.join(directory, entry)
        if os.path.isdir(path):
            inner += tree_xml(path, used)
        elif entry.endswith(".elmt") and path in used:
            inner += '<element name="%s">%s</element>' % (
                entry, ET.tostring(used[path].root, encoding="unicode"))
    if not inner:
        return ""
    return category_xml(os.path.basename(directory),
                        directory_names(directory), inner)


def collection_xml():
    used = {el.file: el for el in CACHE.values()}
    inner = category_xml(
        "20_manufacturers_articles",
        [("en", "Manufacturers articles"), ("fr", "Articles constructeurs")],
        tree_xml(ELMT_DIR, used))
    inner = category_xml("10_electric",
                         [("en", "Electric"), ("fr", "Electrique")], inner)
    return ('<collection><category name="import"><names><name lang="en">'
            "Imported elements</name></names>" + inner +
            "</category></collection>")


def main():
    folios = [
        folio_symbols(), folio_modular(),
        folio_compact(3, "ComPacT NSX - front views",
                      [("-Q5", "nsx100b_3p_tmd100"),
                       ("-Q6", "nsx250n_3p_frame"),
                       ("-Q7", "nsx250n_3p_tm250")]),
        folio_compact(4, "ComPacT INS250 - front views",
                      [("-S3", "ins250_3p"), ("-S4", "ins250_4p")]),
        folio_assembly(), folio_assembly_front()]
    head = (
        '<project title="Schneider Electric breakers example" uuid="%s" '
        'version="%s"><properties/>' % (uid("project"), VERSION)
        + titleblock_templates() +
        '<usage enabled="true" time_spent="0"/><newdiagrams>'
        '<border cols="20" colsize="61" displaycols="true" '
        'displayrows="true" rows="10" rowsize="80"/>'
        '<inset author="" auto_page_num="" date="null" displayAt="bottom" '
        'filename="" folio="" indexrev="" locmach="" plant="" title="" '
        'titleblocktemplate="A3 landscape" '
        'titleblocktemplateCollection="embedded" version=""/>'
        '<conductors %s/><report label="%%f-%%l%%c"/><xrefs/>'
        '<conductors_autonums auto_break_conductors="false" '
        'current_autonum="" freeze_new_conductors="false"/><folio_autonums/>'
        '<element_autonums current_autonum="" freeze_new_elements="false"/>'
        "<guides/></newdiagrams>" % DEFCOND)
    text = ("<project" + head[len("<project"):] + "".join(folios)
            + collection_xml() + "</project>\n")
    root = ET.fromstring(text.encode("utf-8"))
    result = tool.process(root)
    if result.warnings or result.errors:
        sys.exit("\n".join(result.warnings + result.errors))
    text = ('<?xml version="1.0" encoding="UTF-8"?>\n'
            + ET.tostring(root, encoding="unicode") + "\n")
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print("wrote", os.path.relpath(OUT, ROOT))


if __name__ == "__main__":
    main()
