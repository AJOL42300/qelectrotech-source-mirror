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
"""Build the example project of the enclosure layout helper.

    python3 misc/enclosure_layout/make_example.py [--check]

Writes examples/enclosure_layout_example.qet and
examples/enclosure_layout_example_devices.json from the catalog of
extra-elements/ide-enclosures. With --check nothing is written: the exit
code is 1 when the files on disk are not what the script builds.

The project holds a wiring folio and a layout folio (front view of the
mounting plate) for three cabinets, and one folio that fails the check on
purpose. Every element it uses is embedded in the project, so the file
opens without any library.
"""

import argparse
import json
import sys
import uuid
import xml.etree.ElementTree as ET
from pathlib import Path

import enclosure_layout as el

EXAMPLES = el.REPO_ROOT / "examples"
PROJECT_FILE = EXAMPLES / "enclosure_layout_example.qet"
DEVICES_FILE = EXAMPLES / "enclosure_layout_example_devices.json"
VERSION = "0.200.1"
PLANT = "Example plant"
ORIGIN_PX = 40
ERROR_FOLIO = "ERROR DEMO - deliberate errors (CP4-DEMO)"

GENERIC_STEM = "example_generic_device"
GENERIC_FOLDER = "example"
CUSTOM_FOLDER = "custom"
LIBRARY = "ide-enclosures"

PSU = "PSU 24V 10A (generic)"
MCB_1P = "MCB 1P C10 (generic)"
MCB_3P = "MCB 3P C16 (generic)"
DEVICES = [
	("Beckhoff", "EK1100", "beckhoff_ek1100", "EtherCAT coupler"),
	("Beckhoff", "EL1008", "beckhoff_el_12", "8-channel digital input"),
	("Beckhoff", "EL2008", "beckhoff_el_12", "8-channel digital output"),
	("Generic", PSU, "din_8m", "24 V DC power supply"),
	("Generic", MCB_1P, "din_1m", "Miniature circuit breaker, 1 pole"),
	("Generic", MCB_3P, "din_3m", "Miniature circuit breaker, 3 poles"),
]
DESIGNATION = {reference: text for _, reference, _, text in DEVICES}
MAKER = {reference: maker for maker, reference, _, _ in DEVICES}

# Contents of each cabinet: (tag, reference). The tag completes the label.
CONTENTS = {
	"CP1": [("A1", "EK1100"), ("A2", "EL1008"), ("A3", "EL1008"),
		("A4", "EL2008"), ("A5", "EL2008"), ("G1", PSU),
		("F1", MCB_1P), ("F2", MCB_1P), ("F3", MCB_1P), ("F4", MCB_1P),
		("Q1", MCB_3P), ("Q2", MCB_3P)],
	"CP2": [("A1", "EK1100"), ("A2", "EL1008"), ("A3", "EL1008"),
		("A4", "EL2008"), ("F1", MCB_1P), ("F2", MCB_1P),
		("Q1", MCB_3P)],
	# Crowded on purpose: the check warns about the occupancy.
	"CP3": [("G1", PSU)] + [("Q%d" % i, MCB_3P) for i in range(1, 9)],
}
ENCLOSURE = {"CP1": "GN605020", "CP2": "GN403020", "CP3": "GNG303020",
	"CP4-DEMO": "GN403020"}
DUCT_H40, DUCT_H60, DUCT_H80 = "duct_efapel_14020", "duct_efapel_14110", \
	"duct_efapel_14120"

# Layout of each cabinet, in millimetres from the top left corner of its
# enclosure: (kind, id or reference, x, y, length for stretchable parts).
# A duct comes with its cover.
LAYOUT = {
	"CP1": [
		("duct", DUCT_H40, 25, 25, 450), ("duct", DUCT_H60, 25, 200, 450),
		("duct", DUCT_H80, 25, 495, 450),
		("part", "rail_ts35_400", 50, 112, None),
		("part", "rail_ts35_400", 50, 307, None),
		("part", "beckhoff_ek1100", 60, 80, None),
		("part", "beckhoff_el_12", 104, 80, None),
		("part", "beckhoff_el_12", 116, 80, None),
		("part", "beckhoff_el_12", 128, 80, None),
		("part", "beckhoff_el_12", 140, 80, None),
		("part", "din_8m", 200, 85, None),
		("part", "din_1m", 60, 280, None),
		("part", "din_1m", 77.5, 280, None),
		("part", "din_1m", 95, 280, None),
		("part", "din_1m", 112.5, 280, None),
		("part", "din_3m", 150, 280, None),
		("part", "din_3m", 202.5, 280, None)],
	"CP2": [
		("duct", DUCT_H40, 25, 25, 250), ("duct", DUCT_H60, 25, 335, 250),
		("part", "rail_ts35_200", 50, 112, None),
		("part", "rail_ts35_200", 50, 262, None),
		("part", "beckhoff_ek1100", 60, 80, None),
		("part", "beckhoff_el_12", 104, 80, None),
		("part", "beckhoff_el_12", 116, 80, None),
		("part", "beckhoff_el_12", 128, 80, None),
		("part", "din_1m", 60, 230, None),
		("part", "din_1m", 77.5, 230, None),
		("part", "din_3m", 100, 230, None)],
	"CP3": [],
	# Two overlapping devices, and one that leaves the usable area.
	"CP4-DEMO": [
		("part", "din_4m", 60, 100, None),
		("part", "din_4m", 100, 100, None),
		("part", "din_3m", 240, 300, None)],
}
Z = {"enclosure": 1, "plate": 2, "door": 3, "rail": 4, "duct": 5,
	"cover": 6, "module": 7}


def uid(*parts):
	return "{%s}" % uuid.uuid5(uuid.NAMESPACE_URL, "qelectrotech/example/"
		+ "/".join(str(p) for p in parts))


class Library:
	"""The elements the project embeds, by folder and stem."""

	def __init__(self, catalog):
		self.catalog = catalog
		self.files = {}
		for name, text in el.render_all(catalog).items():
			if name.endswith(".elmt"):
				folder, _, file = name.rpartition("/")
				self.files[file[:-5]] = (folder, text)
		self.used = set()

	def footprint(self, item):
		"""The stem of a footprint; a custom length is made on the spot."""
		stem = el.footprint_stem(item)
		if stem not in self.files:
			self.files[stem] = (CUSTOM_FOLDER,
				el.footprint_element(self.catalog, item))
		return stem

	def path(self, stem):
		"""The type attribute of an element, which is also registered."""
		if stem not in self.files:
			raise ValueError("unknown element '%s'" % stem)
		self.used.add(stem)
		return "embed://import/%s/%s/%s.elmt" % (
			LIBRARY, self.files[stem][0], stem)

	def definition(self, stem):
		return ET.fromstring(self.files[stem][1])


def generic_device():
	"""A box with the label and the manufacturer reference of a device."""
	parts = [el._rect(0, 0, 120, 30, el.STYLE_BASE % ("normal", "normal"))]
	for info, y, size in (("label", 3, 8), ("manufacturer_reference", 17, 6)):
		node = ET.Element("dynamic_text", {
			"x": "3", "y": str(y), "z": "5", "text_width": "-1",
			"Halignment": "AlignLeft", "Valignment": "AlignTop",
			"frame": "false", "rotation": "0",
			"keep_visual_rotation": "false", "text_from": "ElementInfo",
			"uuid": el._uuid(GENERIC_STEM, info), "font": el.FONT % size})
		ET.SubElement(node, "text").text = ""
		ET.SubElement(node, "info_name").text = info
		parts.append(node)
	root = el._definition(GENERIC_STEM, [
		("en", "Generic device (box)"), ("fr", "Appareil générique (boîte)")],
		[("description", "Box showing the label and the reference")],
		(10, 10, 120, 30, 10, 10), parts)
	return ET.fromstring(el._serialize(root))


def instance(library, stem, x, y, z, infos, shown, key):
	"""An <element> of a folio: position in pixels, information shown."""
	definition = (generic_device() if stem == GENERIC_STEM
		else library.definition(stem))
	type_ = ("embed://import/%s/%s.elmt" % (GENERIC_FOLDER, stem)
		if stem == GENERIC_STEM else library.path(stem))
	node = ET.Element("element", {
		"freezeLabel": "false", "is_movable": "1", "orientation": "0",
		"prefix": "", "type": type_, "uuid": uid(key, stem),
		"x": el._num(x), "y": el._num(y), "z": str(z)})
	ET.SubElement(node, "terminals")
	ET.SubElement(node, "inputs")
	information = ET.SubElement(node, "elementInformations")
	for name, value in infos:
		ET.SubElement(information, "elementInformation", {
			"name": name, "show": "1"}).text = value
	texts = ET.SubElement(node, "dynamic_texts")
	for source in definition.iter("dynamic_text"):
		name = source.findtext("info_name")
		if name not in shown:
			continue
		attributes = dict(source.attrib)
		attributes["uuid"] = uid(key, stem, name)
		text = ET.SubElement(texts, "dynamic_elmt_text", attributes)
		ET.SubElement(text, "text").text = shown[name]
		ET.SubElement(text, "info_name").text = name
	ET.SubElement(node, "texts_groups")
	return node


def wiring_folio(library, cabinet):
	elements = []
	for index, (tag, reference) in enumerate(CONTENTS[cabinet]):
		label = "-%s-%s" % (cabinet, tag)
		infos = [("label", label), ("designation", DESIGNATION[reference]),
			("manufacturer", MAKER[reference]),
			("manufacturer_reference", reference)]
		x = 60 + (index % 5) * 180
		y = 100 + (index // 5) * 90
		node = instance(library, GENERIC_STEM, x, y, 10, infos,
			{"label": label, "manufacturer_reference": reference},
			(cabinet, "wiring", label))
		elements.append(node)
	return elements


def layout_folio(library, cabinet):
	catalog = library.catalog
	enclosure = next(e for e in catalog["enclosures"]
		if e["reference"] == ENCLOSURE[cabinet])
	origin = ORIGIN_PX * 2
	key = (cabinet, "layout")
	infos = [("label", cabinet)]
	elements = [instance(library, el.enclosure_stem(enclosure),
		origin / 2.0, origin / 2.0, Z["enclosure"], infos,
		{"label": cabinet}, key)]
	area = el.fit_area(enclosure)
	if cabinet != "CP4-DEMO":
		for kind, item in (("plate", el.plate_item(enclosure)),
				("door", el.door_item(enclosure))):
			x = (0 if kind == "door"
				else (enclosure["width"] - item["width"]) / 2.0)
			y = (0 if kind == "door"
				else (enclosure["height"] - item["height"]) / 2.0)
			stem = (el.DOOR_PREFIX if kind == "door"
				else el.PLATE_PREFIX) + enclosure["reference"]
			elements.append(instance(library, stem, (origin + x) / 2.0,
				(origin + y) / 2.0, Z[kind], infos, {}, key))
	for kind, ident, x, y, length in LAYOUT[cabinet]:
		footprint = next(f for f in catalog["footprints"]
			if f["id"] == ident)
		views = [(None, "")]
		if kind == "duct":
			footprint = el.at_length(footprint, length)
			views = [(None, "duct"), ("cover", "cover")]
		for view, zname in views:
			item = dict(footprint, view=view) if view else footprint
			z = Z[zname or footprint["kind"]]
			elements.append(instance(library, library.footprint(item),
				(origin + x) / 2.0, (origin + y) / 2.0, z, [], {},
				key + (len(elements),)))
	return elements, area


def diagram(title, cabinet, order, elements):
	node = ET.Element("diagram", {
		"author": "", "auto_page_num": "", "cols": "17", "colsize": "60",
		"date": "null", "displayAt": "bottom", "displaycols": "true",
		"displayrows": "true", "filename": "", "folio": "%id/%total",
		"freezeNewConductor": "false", "freezeNewElement": "false",
		"height": "660", "indexrev": "", "locmach": cabinet,
		"order": str(order), "plant": PLANT, "rows": "8", "rowsize": "80",
		"title": title, "uuid": uid("diagram", title),
		"version": VERSION})
	ET.SubElement(node, "defaultconductor", CONDUCTOR)
	holder = ET.SubElement(node, "elements")
	for element in elements:
		holder.append(element)
	return node


CONDUCTOR = {
	"bicolor": "false", "bus": "", "cable": "", "color2": "#000000",
	"condsize": "1", "conductor_color": "", "conductor_section": "",
	"dash-size": "1", "displaytext": "1", "formula": "", "function": "",
	"horizontal-alignment": "AlignBottom", "horizrotatetext": "0",
	"num": "_", "numsize": "7", "onetextperfolio": "0",
	"tension_protocol": "", "text_color": "#000000", "type": "multi",
	"vertical-alignment": "AlignRight", "vertirotatetext": "270"}
XREF = {
	"delayprefix": "", "displayhas": "cross", "master_label": "%f-%l%c",
	"offset": "0", "powerprefix": "", "showallconfiguredslaves": "false",
	"showpowerctc": "true", "showterminalname": "true",
	"slave_label": "(%f-%l%c)", "slave_offset": "0", "snapto": "label",
	"stackoverlapping": "true", "switchprefix": "",
	"xrefpos": "AlignBottom"}


def new_diagrams():
	node = ET.Element("newdiagrams")
	ET.SubElement(node, "border", {
		"cols": "17", "colsize": "60", "displaycols": "true",
		"displayrows": "true", "rows": "8", "rowsize": "80"})
	ET.SubElement(node, "inset", {
		"author": "", "auto_page_num": "", "date": "null",
		"displayAt": "bottom", "filename": "", "folio": "", "indexrev": "",
		"locmach": "", "plant": "", "title": "", "version": ""})
	ET.SubElement(node, "conductors", CONDUCTOR)
	ET.SubElement(node, "report", {"label": "%f-%l%c"})
	xrefs = ET.SubElement(node, "xrefs")
	for kind in ("cable", "coil", "commutator", "plc", "protection"):
		ET.SubElement(xrefs, "xref", dict(XREF, type=kind))
	ET.SubElement(node, "conductors_autonums", {
		"auto_break_conductors": "false", "current_autonum": "",
		"freeze_new_conductors": "false"})
	ET.SubElement(node, "folio_autonums")
	ET.SubElement(node, "element_autonums", {
		"current_autonum": "", "freeze_new_elements": "false"})
	ET.SubElement(node, "guides")
	return node


def category(parent, name, names):
	node = ET.SubElement(parent, "category", {"name": name})
	holder = ET.SubElement(node, "names")
	for lang, text in names:
		ET.SubElement(holder, "name", {"lang": lang}).text = text
	return node


def collection(library):
	"""The embedded elements: only the ones the folios use."""
	root = ET.Element("collection")
	top = category(root, "import", [("en", "Imported elements"),
		("fr", "Éléments importés")])
	known = ET.fromstring(el.render_all(library.catalog)["qet_directory"])
	root_names = [(n.get("lang"), n.text) for n in known.iter("name")]
	example = category(top, GENERIC_FOLDER, [("en", "Example"),
		("fr", "Exemple")])
	definition = ET.SubElement(example, "element", {
		"name": GENERIC_STEM + ".elmt"})
	definition.append(generic_device())
	library_node = category(top, LIBRARY, root_names)
	folders = {}
	for stem in sorted(library.used):
		folder, text = library.files[stem]
		if folder not in folders:
			names = ([("en", "Rails and ducts at a chosen length"),
				("fr", "Rails et goulottes à la longueur choisie")]
				if folder == CUSTOM_FOLDER else [
				(n.get("lang"), n.text) for n in ET.fromstring(
				el.render_all(library.catalog)[folder + "/qet_directory"])
				.iter("name")])
			folders[folder] = category(library_node, folder, names)
		item = ET.SubElement(folders[folder], "element", {
			"name": stem + ".elmt"})
		item.append(ET.fromstring(text))
	return root


def build(catalog):
	"""(text of the project, devices) of the example."""
	library = Library(catalog)
	folios = []
	for cabinet in ("CP1", "CP2", "CP3"):
		folios.append(("%s - Wiring" % cabinet, cabinet,
			wiring_folio(library, cabinet)))
		folios.append(("%s - Layout (front view)" % cabinet, cabinet,
			layout_folio(library, cabinet)[0]))
	folios.append((ERROR_FOLIO, "CP4-DEMO",
		layout_folio(library, "CP4-DEMO")[0]))
	project = ET.Element("project", {
		"title": "Enclosure layout example", "uuid": uid("project"),
		"version": VERSION})
	ET.SubElement(project, "properties")
	ET.SubElement(project, "usage", {"enabled": "true",
		"time_spent": "0"})
	ET.SubElement(project, "symbol_texts", {"upright": "true"})
	project.append(new_diagrams())
	for order, (title, cabinet, elements) in enumerate(folios, 1):
		project.append(diagram(title, cabinet, order, elements))
	project.append(collection(library))
	ET.indent(project, space="    ")
	devices = [{"reference": reference, "manufacturer": maker,
		"footprint": footprint} for maker, reference, footprint, _ in DEVICES]
	return ET.tostring(project, encoding="unicode") + "\n", devices


def devices_text(devices):
	lines = ",\n".join("\t" + json.dumps(
		d, ensure_ascii=False) for d in devices)
	return '{"devices": [\n%s\n]}\n' % lines


def write(path, text):
	with open(path, "w", encoding="utf-8", newline="\n") as handle:
		handle.write(text)


def main(argv=None):
	parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
	parser.add_argument("--check", action="store_true",
		help="only compare with the files on disk")
	arguments = parser.parse_args(argv)
	text, devices = build(el.load_catalog())
	wanted = {PROJECT_FILE: text, DEVICES_FILE: devices_text(devices)}
	if arguments.check:
		stale = [p.name for p, t in wanted.items() if not p.exists()
			or p.read_bytes().decode("utf-8") != t]
		for name in stale:
			print("%s is not up to date" % name)
		return 1 if stale else 0
	for path, content in wanted.items():
		write(path, content)
		print("wrote %s" % path.relative_to(el.REPO_ROOT))
	return 0


if __name__ == "__main__":
	sys.exit(main())
