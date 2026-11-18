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
"""Write examples/lanbao_example.qet.

Folio 1 is a wired schematic: a Mean Well HDR-60-24 supplies the three
Lanbao sensors (BN to +24 V, BU to 0 V). The PNP outputs of the PSM-BC100DPB-E2
and of the LR18XCF12DPOY-E2 are wired to two inputs of an EL1008 digital input
terminal; the 4...20 mA output Ia of the PDB-CC50TGI is wired to an analog
input terminal. The switch output Q of the PDB-CC50TGI and pin 2 of the
PSM-BC100DPB-E2 (open: NO) are left unconnected. Folio 2 shows the realistic
views and the description views of the three sensors. The used elements are
embedded in the project.

Usage: python misc/lanbao/gen_lanbao_example.py
The output is deterministic (all uuids are derived from fixed names).
"""
import importlib.util
import os
import sys
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))
MURR = os.path.join(os.path.dirname(HERE), "murr_meanwell")
sys.path.insert(0, HERE)
sys.path.insert(0, MURR)
sys.path.insert(1, os.path.join(os.path.dirname(HERE), "beckhoff"))
SPEC = importlib.util.spec_from_file_location(
    "murr_gen_example", os.path.join(MURR, "gen_example.py"))
ge = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ge)
import gen_sensors  # noqa: E402

ROOT = ge.ROOT
OUT = os.path.join(ROOT, "examples", "lanbao_example.qet")
ge.OUT = OUT
EXTRA = ge.EXTRA
CARD_IN = os.path.join(EXTRA, "15_beckhoff_ethercat", "el1xxx_digital_in",
                       "el1008.elmt")
CARD_AI = os.path.join(EXTRA, "15_beckhoff_ethercat", "el3xxx_analog_in",
                       "el3xxx_generic.elmt")
EXTRA_ROOTS = ["15_beckhoff_ethercat", "17_meanwell_psu",
               gen_sensors.LANBAO_DIR]
SENSORS = {s.ref: s for s in gen_sensors.SENSORS}
PSU_REF = "HDR-60-24"

# Function text of the wired inputs: (label, address) -> text.
USED = {("A1", "I1"): "B2 PSM diffuse 100 cm, PNP output",
        ("A1", "I2"): "B3 LR18X inductive Sn 12 mm, PNP NO",
        ("A2", "1"): "B1 PDB laser 80-500 mm, 4-20 mA"}


def get_clean(path):
    """get_file without the terminal tags of the plcIO nodes."""
    el = ge.get_file(path)
    el.terminals = {k: v for k, v in el.terminals.items() if k}
    return el


def hot(el):
    return int(el.root.get("hotspot_x")), int(el.root.get("hotspot_y"))


def folio_wired():
    psu = get_clean(ge.by_ref(PSU_REF).path(""))
    sens = {ref: ge.get(s) for ref, s in SENSORS.items()}
    a1, a2 = get_clean(CARD_IN), get_clean(CARD_AI)
    # (label, element, hotspot position): the sensors are placed by the
    # origin of their drawing so that the terminals fall on known points.
    placed = [("G1", psu, 120, 100), ("A1", a1, 900, 580),
              ("A2", a2, 1100, 360)]
    origins = [("B1", "PDB-CC50TGI", 260, 260),
               ("B2", "PSM-BC100DPB-E2", 440, 390),
               ("B3", "LR18XCF12DPOY-E2", 620, 520)]
    for label, ref, ox, oy in origins:
        hx, hy = hot(sens[ref])
        placed.append((label, sens[ref], ox + hx, oy + hy))
    elements, parts = [], {}
    for label, el, x, y in placed:
        parts[label] = el
        elements.append(ge.element_xml(el, label, x, y, f"sym/{label}",
                                       used=USED))
    wires = [("G1", "+V", "B1", "BN"), ("G1", "+V", "B2", "1"),
             ("G1", "+V", "B3", "1"), ("G1", "-V", "B1", "BU"),
             ("G1", "-V", "B2", "3"), ("G1", "-V", "B3", "3"),
             ("B2", "4", "A1", "1"), ("B3", "4", "A1", "2"),
             ("B1", "GY", "A2", "1")]
    conductors = []
    for a, ta, b, tb in wires:
        conductors.append(ge.conductor_xml(
            f"sym/{a}{ta}{b}{tb}", f"sym/{a}", a, parts[a], ta,
            f"sym/{b}", b, parts[b], tb))
    return ge.diagram_xml(1, "Lanbao sensors - wired example", elements,
                          conductors)


def folio_views():
    elements = []
    y = 60
    for label, ref in (("B1", "PDB-CC50TGI"), ("B2", "PSM-BC100DPB-E2"),
                       ("B3", "LR18XCF12DPOY-E2")):
        x = 60
        for view in ("_realistic", "_description", "_layout"):
            el = ge.get(SENSORS[ref], view)
            hx, hy = hot(el)
            elements.append(ge.element_xml(
                el, label, x + hx, y + hy, f"{view}/{label}"))
            x += int(el.root.get("width")) + 30
        y += 260
    return ge.diagram_xml(2, "Lanbao sensors - views and layout", elements,
                          [])


def collection_xml():
    libs = "".join(ge.tree_xml(r) for r in EXTRA_ROOTS)
    return ('<collection><category name="import"><names><name lang="en">'
            "Imported elements</name></names>" + libs +
            "</category></collection>")


def main():
    folios = [folio_wired(), folio_views()]
    xrefs = "".join(
        f'<xref delayprefix="" displayhas="cross" master_label="%f-%l%c" '
        'offset="0" powerprefix="" showallconfiguredslaves="false" '
        'showpowerctc="true" showterminalname="true" '
        'slave_label="(%f-%l%c)" slave_offset="0" snapto="label" '
        f'stackoverlapping="true" switchprefix="" type="{t}" '
        'xrefpos="AlignBottom"/>'
        for t in ("cable", "coil", "commutator", "plc", "protection"))
    head = (
        f'<project title="Lanbao sensors example" '
        f'uuid="{ge.uid("lanbao", "project")}" version="{ge.VERSION}">'
        "<properties/>" + ge.titleblock_templates() +
        '<usage enabled="true" time_spent="0"/><newdiagrams>'
        '<border cols="20" colsize="61" displaycols="true" '
        'displayrows="true" rows="10" rowsize="80"/>'
        '<inset author="" auto_page_num="" date="null" displayAt="bottom" '
        'filename="" folio="" indexrev="" locmach="" plant="" title="" '
        'titleblocktemplate="A3 landscape" '
        'titleblocktemplateCollection="embedded" version=""/>'
        f"<conductors {ge.DEFCOND}/><report label=\"%f-%l%c\"/>"
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
