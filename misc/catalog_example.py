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
"""Write examples/manufacturers_catalog_example.qet.

One set of folios per brand and category of extra-elements (a category is
the folder that groups the products, such as el1xxx_digital_in or
acti9_ic60). Every element of the category is placed once, a product and
its views (symbol, generic, realistic, description, table) on the same
row, the file name under each of them as the label. The implantation
elements (layouts, footprints, enclosures) are left out.

It reuses the project writer of misc/beckhoff/gen_example.py.

Usage: python misc/catalog_example.py
The output is deterministic (all uuids are derived from fixed names).
After a change of the elements, run it again, then
qelectrotech --resave examples/manufacturers_catalog_example.qet <copy> and
replace the example by the copy.
"""
import importlib.util
import os
import re
import sys
import uuid
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))
_spec = importlib.util.spec_from_file_location(
    "beckhoff_example", os.path.join(HERE, "beckhoff", "gen_example.py"))
bg = importlib.util.module_from_spec(_spec)
sys.path.insert(0, os.path.join(HERE, "beckhoff"))
_spec.loader.exec_module(bg)

ROOT = bg.ROOT
LIB = os.path.join(ROOT, "extra-elements")
bg.CAT = ""
bg.ELMT_DIR = LIB
bg.OUT = os.path.join(ROOT, "examples", "manufacturers_catalog_example.qet")
bg.NS = uuid.UUID("c3a7e5d2-41b8-5d6f-9a02-7e1f4b8c6d35")

SKIP_DIR = re.compile(r"(^|/)(ide-enclosures|implantation|layout)(/|$)")
SKIP_FILE = re.compile(r"_(layout|fp|fp_face|fp_cutout)\.elmt$")
RANK = {"": 0, "generic": 1, "v": 2, "realistic": 3, "description": 4,
        "table": 5}
LEFT, TOP, RIGHT, BOTTOM = 40, 30, 1190, 730


def directory_names(rel):
    path = os.path.join(LIB, *rel.split("/"), "qet_directory")
    names = {}
    if os.path.exists(path):
        for n in ET.parse(path).getroot().iter("name"):
            names[n.get("lang")] = n.text or ""
    return names


def title_of(rel):
    names = directory_names(rel)
    return names.get("fr") or names.get("en") or rel.split("/")[-1]


def product_and_variant(dirname, stem):
    for variant in ("description", "realistic", "table", "generic", "v"):
        if stem.endswith("_" + variant):
            return stem[:-len(variant) - 1], variant
    return stem, ""


def categories():
    """{category folder: [(product, variant, rel path of the element)]}."""
    out = {}
    for dp, dn, fn in os.walk(LIB):
        dn.sort()
        rel = os.path.relpath(dp, LIB).replace("\\", "/")
        if rel == "." or SKIP_DIR.search(rel):
            continue
        files = [f for f in sorted(fn)
                 if f.endswith(".elmt") and not SKIP_FILE.search(f)]
        if not files:
            continue
        leaf = rel.split("/")[-1]
        product = any(f[:-5].startswith(leaf) for f in files)
        category = rel.rsplit("/", 1)[0] if product and "/" in rel else rel
        for f in files:
            stem, variant = product_and_variant(leaf, f[:-5])
            out.setdefault(category, []).append((stem, variant,
                                                 rel + "/" + f[:-5]))
    return out


def load(rel):
    folder, name = rel.rsplit("/", 1)
    key = (folder, name)
    if key not in bg.ELEMENTS:
        root = ET.parse(os.path.join(LIB, *rel.split("/")) + ".elmt")
        bg.ELEMENTS[key] = bg.Elmt(folder, name, root.getroot())
    return bg.ELEMENTS[key]


def folios_of(category, items, first):
    """The folios of one category: the products in rows, their views in
    the same row, a new folio when the rows do not fit."""
    parts = category.split("/")
    heading = title_of(category)
    if len(parts) >= 2 and parts[-2] not in ("10_electric",
                                             "20_manufacturers_articles"):
        heading = title_of("/".join(parts[:-1])) + " - " + heading
    items = sorted(items, key=lambda i: (i[0], RANK.get(i[1], 9)))
    pages, els = [], []
    x, y, row = LEFT, TOP, 0
    previous = None
    for product, variant, rel in items:
        el = load(rel)
        w, h = bg.size(el, "width"), bg.size(el, "height")
        newline = previous is not None and previous != product
        if (newline and x > LEFT) or (x + w > RIGHT and x > LEFT):
            x, y, row = LEFT, y + row + 40, 0
        if y + h + 20 > BOTTOM and els:
            pages.append(els)
            els, x, y, row = [], LEFT, TOP, 0
        label = rel.split("/")[-1].upper()
        els.append(bg.element_xml(
            el, label, x + bg.size(el, "hotspot_x"),
            y + bg.size(el, "hotspot_y"), "catalog/" + rel))
        x += w + 30
        row = max(row, h)
        previous = product
    pages.append(els)
    folios = []
    for n, page in enumerate(pages):
        title = heading
        if len(pages) > 1:
            title += f" ({n + 1}/{len(pages)})"
        folios.append(bg.diagram_xml(first + len(folios), title, page, []))
    return folios


def tree(rel):
    inner = ""
    for (folder, name), el in sorted(bg.ELEMENTS.items()):
        if folder == rel:
            inner += (f'<element name="{name}.elmt">' +
                      ET.tostring(el.root, encoding="unicode") +
                      "</element>")
    subs = sorted({c[len(rel) + 1:].split("/")[0] for c, _ in bg.ELEMENTS
                   if c.startswith(rel + "/")})
    for s in subs:
        inner += tree(rel + "/" + s)
    names = directory_names(rel) or {"en": rel.split("/")[-1]}
    return bg.category_xml(rel.split("/")[-1], list(names.items()), inner)


def main():
    folios = []
    for category, items in sorted(categories().items()):
        folios += folios_of(category, items, len(folios) + 1)
    tops = sorted({c.split("/")[0] for c, _ in bg.ELEMENTS})
    collection = (
        '<collection><category name="import"><names><name lang="en">'
        "Imported elements</name></names>" +
        "".join(tree(t) for t in tops) + "</category></collection>")
    xrefs = "".join(
        f'<xref delayprefix="" displayhas="cross" master_label="%f-%l%c" '
        'offset="0" powerprefix="" showallconfiguredslaves="false" '
        'showpowerctc="true" showterminalname="true" '
        'slave_label="(%f-%l%c)" slave_offset="0" snapto="label" '
        f'stackoverlapping="true" switchprefix="" type="{t}" '
        'xrefpos="AlignBottom"/>'
        for t in ("cable", "coil", "commutator", "plc", "protection"))
    head = (
        f'<project title="Manufacturer elements catalogue" '
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
            "".join(folios) + collection + "</project>\n")
    ET.fromstring(text.encode("utf-8"))
    os.makedirs(os.path.dirname(bg.OUT), exist_ok=True)
    with open(bg.OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print("wrote", os.path.relpath(bg.OUT, ROOT), len(folios), "folios")


if __name__ == "__main__":
    main()
