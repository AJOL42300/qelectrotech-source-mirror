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
"""Enclosure layout helper: IDE cabinets as elements, and checks.

Commands, driven by extra-elements/ide-enclosures/catalog.json and
devices.json:

    python3 misc/enclosure_layout/enclosure_layout.py generate
    python3 misc/enclosure_layout/enclosure_layout.py check project.qet
    python3 misc/enclosure_layout/enclosure_layout.py report project.qet
    python3 misc/enclosure_layout/enclosure_layout.py link REF --footprint ID
    python3 misc/enclosure_layout/enclosure_layout.py length ID MM [MM...]

"generate" writes the front-view enclosure, door and mounting plate
elements and the footprint elements (DIN modules, DIN rails, ducts) next to
the catalog. "length" writes a rail or a duct at the length given in
millimetres (a duct also as a realistic view or as its cover). "articles"
lists the references of the ducts. "check"
reads a project and reports footprints that leave the usable area of their
enclosure, are deeper than it or overlap, devices that do not fit in their
cabinet, and components found in several cabinets. "report" lists the
contents of each cabinet. "link" ties a manufacturer reference to a
footprint. See docs/enclosure_layout.md.
"""

import argparse
import csv
import json
import math
import re
import sys
import uuid
import xml.etree.ElementTree as ET
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_DIR = REPO_ROOT / "extra-elements" / "ide-enclosures"
ENCLOSURE_PREFIX = "ide_enc_"
FOOTPRINT_PREFIX = "ide_fp_"
DOOR_PREFIX = "ide_door_"
PLATE_PREFIX = "ide_plate_"
# A duct has three views: the footprint (None), a realistic top view with
# its slots and the cover alone. Each has its own stem prefix and folder.
VIEW_PREFIX = {None: FOOTPRINT_PREFIX, "realistic": "ide_fpr_",
               "cover": "ide_cov_"}
VIEW_FOLDER = {None: "footprints", "realistic": "duct-realistic",
               "cover": "duct-covers"}
CUT_FOLDER = "duct-cut-lengths"
# Pixels between two slots of the realistic view (schematic pitch).
SLOT_PITCH_PX = 20
# Suffix of the stem of a footprint drawn at a chosen length, in mm.
LENGTH_SUFFIX = re.compile(r"^(.+)_L(\d+)$")
# Millimetres of tolerance when comparing positions and sizes.
EPSILON = 0.01
# Share of the usable area above which a cabinet is reported as crowded.
FILL_WARNING = 0.8
# Label prefixes of terminals, which may be spread over several cabinets.
SHARED_PREFIXES = ("X",)
DEFINITION_VERSION = "0.100.0"
LICENSE_TEXT = ("License: see "
                "http://qelectrotech.org/wiki/doc/elements_license")
STYLE_BASE = "line-style:%s;line-weight:%s;filling:none;color:black"
FONT = "Liberation Sans,%d,-1,5,50,0,0,0,0,0,Regular"


def normalize_reference(text):
    return " ".join((text or "").split()).casefold()


def read_devices(path):
    """The device entries of a devices file ([] when it does not exist)."""
    path = Path(path)
    if not path.exists():
        return []
    with open(path, encoding="utf-8") as handle:
        return json.load(handle).get("devices", [])


def write_devices(path, devices):
    """Write the devices file, one device per line to keep diffs small."""
    lines = ",\n".join("\t" + json.dumps(d, ensure_ascii=False)
                       for d in devices)
    with open(path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write('{"devices": [\n%s\n]}\n' % lines if lines
                     else '{"devices": []}\n')


def load_catalog(path=None, devices_path=None):
    """Return the catalog read from path (default: the shipped one).

    The links between device references and footprints are read from
    devices.json next to the catalog, or from devices_path.
    """
    path = Path(path) if path else DEFAULT_DIR / "catalog.json"
    with open(path, encoding="utf-8") as handle:
        catalog = json.load(handle)
    catalog["devices_path"] = (Path(devices_path) if devices_path
                               else path.parent / "devices.json")
    catalog["_by_stem"] = {}
    for enclosure in catalog["enclosures"]:
        stem = ENCLOSURE_PREFIX + enclosure["reference"]
        catalog["_by_stem"][stem] = ("enclosure", enclosure)
    for footprint in catalog["footprints"]:
        stem = FOOTPRINT_PREFIX + footprint["id"]
        catalog["_by_stem"][stem] = ("footprint", footprint)
        if footprint["kind"] == "duct":
            for view in ("realistic", "cover"):
                item = dict(footprint, view=view)
                catalog["_by_stem"][footprint_stem(item)] = (
                    "footprint", item)
    for enclosure in catalog["enclosures"]:
        for kind, prefix, item in (("door", DOOR_PREFIX, door_item(enclosure)),
                                   ("plate", PLATE_PREFIX,
                                    plate_item(enclosure))):
            if item:
                catalog["_by_stem"][prefix + enclosure["reference"]] = (
                    kind, item)
    by_id = {f["id"]: f for f in catalog["footprints"]}
    catalog["_devices"] = []
    for device in read_devices(catalog["devices_path"]):
        if "footprint" in device:
            if device["footprint"] not in by_id:
                raise ValueError("device %s: unknown footprint '%s'" % (
                    device["reference"], device["footprint"]))
            size = by_id[device["footprint"]]
        else:
            size = device
        catalog["_devices"].append({
            "key": normalize_reference(device["reference"]),
            "manufacturer": normalize_reference(
                device.get("manufacturer")),
            "reference": device["reference"],
            "width": size["width"], "height": size["height"],
            "depth": size["depth"]})
    return catalog


def find_device(catalog, manufacturer, reference):
    """The footprint dimensions linked to a device, or None.

    A device entry with a manufacturer only matches that manufacturer.
    """
    key = normalize_reference(reference)
    maker = normalize_reference(manufacturer)
    for device in catalog["_devices"]:
        if device["key"] == key and (not device["manufacturer"]
                                     or device["manufacturer"] == maker):
            return device
    return None


def enclosure_stem(enclosure):
    return ENCLOSURE_PREFIX + enclosure["reference"]


def footprint_stem(footprint):
    return VIEW_PREFIX[footprint.get("view")] + footprint["id"]


def door_item(enclosure):
    """The door of an enclosure as a placeable part, or None."""
    door = enclosure.get("door")
    if not door:
        return None
    return {"part": "door", "enclosure": enclosure["reference"],
            "reference": enclosure["reference"], "data": door,
            "width": door["width"], "height": door["height"],
            "depth": door.get("thickness", 0)}


def plate_item(enclosure):
    """The mounting plate of an enclosure as a placeable part, or None."""
    plate = enclosure.get("plate")
    if not plate:
        return None
    return {"part": "plate", "enclosure": enclosure["reference"],
            "reference": plate.get("reference") or enclosure["reference"],
            "data": plate, "width": plate["width"],
            "height": plate["height"], "depth": plate.get("thickness", 0)}


def at_length(footprint, length):
    """A copy of a stretchable footprint drawn at the given length (mm)."""
    if not footprint.get("stretch"):
        raise ValueError("%s has a fixed length" % footprint["id"])
    if length <= 0:
        raise ValueError("the length must be positive")
    copy = dict(footprint)
    copy["id"] = "%s_L%d" % (footprint["id"], length)
    copy["width"] = length
    copy["label"] = "%s L%d" % (footprint["label"], length)
    copy["name_en"] = "%s, %d mm long" % (footprint["name_en"], length)
    copy["name_fr"] = "%s, longueur %d mm" % (footprint["name_fr"], length)
    return copy


def lookup(catalog, stem):
    """(kind, item) of an element stem; None when the catalog lacks it.

    The stem of a stretchable footprint may end with _L and a length in
    mm: it stands for the footprint at that length.
    """
    found = catalog["_by_stem"].get(stem)
    if found:
        return found
    match = LENGTH_SUFFIX.match(stem)
    base = catalog["_by_stem"].get(match.group(1)) if match else None
    if base and base[0] == "footprint" and base[1].get("stretch"):
        return ("footprint", at_length(base[1], int(match.group(2))))
    return None


def fit_area(enclosure):
    """The area, in millimetres, where footprints must stay.

    Returns (x0, y0, x1, y1, depth) relative to the top left corner of the
    front view, or None when the catalog does not give the usable space.
    When both the plate and the usable space are known the smaller of the
    two wins on each side. The area is centred in the front view.
    """
    plate = enclosure.get("plate")
    usable = enclosure.get("usable")
    if plate and usable:
        height = min(plate["height"], usable["height"])
        width = min(plate["width"], usable["width"])
    elif plate:
        height, width = plate["height"], plate["width"]
    elif usable:
        height, width = usable["height"], usable["width"]
    else:
        return None
    depth = usable["depth"] if usable else enclosure["depth"]
    x0 = (enclosure["width"] - width) / 2.0
    y0 = (enclosure["height"] - height) / 2.0
    return (x0, y0, x0 + width, y0 + height, depth)


def _num(value):
    text = ("%.2f" % value).rstrip("0").rstrip(".")
    return text or "0"


RAIL_SNAP_BASES = ("datasheet", "image", "standard", "assumed")
RAIL_SNAP_MOUNTS = ("origin", "start", "center", "end")
RAIL_SNAP_GAP_MM = 2
RAIL_AXIS_SOURCE = ("EN 60715: the axis of a rail is the middle of its "
                    "35 mm width")


def rail_snap(footprint):
    """What a footprint declares for rail snapping, as a dict.

    A DIN rail is a "rail", a modular device a "component" fixed on it; a
    duct takes no part (None). The catalog can give a "rail_snap" object
    for a footprint: mount (origin, start, center or end: where its fixing
    line lies, default center), mount_offset_mm (default 0), gap_mm
    (default 2), basis (datasheet, image, standard or assumed: where the
    position comes from; assumed when the catalog says nothing, except
    for a rail), source (the document or the image) and verified (the date
    the position was compared with its source). See docs/rail-snapping.md.
    """
    role = {"rail": "rail", "module": "component"}.get(footprint["kind"])
    if role is None:
        return None
    declared = footprint.get("rail_snap", {})
    data = {
        "role": role,
        "mount": declared.get("mount", "center"),
        "mount_offset_mm": float(declared.get("mount_offset_mm", 0)),
        "gap_mm": float(declared.get("gap_mm", RAIL_SNAP_GAP_MM)),
        "basis": declared.get("basis",
                              "standard" if role == "rail" else "assumed"),
        "source": declared.get("source",
                               RAIL_AXIS_SOURCE if role == "rail" else ""),
        "verified": declared.get("verified", ""),
    }
    if data["mount"] not in RAIL_SNAP_MOUNTS:
        raise ValueError("footprint %s: unknown rail_snap mount '%s'"
                         % (footprint["id"], data["mount"]))
    if data["basis"] not in RAIL_SNAP_BASES:
        raise ValueError("footprint %s: unknown rail_snap basis '%s'"
                         % (footprint["id"], data["basis"]))
    if data["gap_mm"] < 0:
        raise ValueError("footprint %s: negative rail_snap gap"
                         % footprint["id"])
    return data


def _rail_snap_node(footprint, scale, width, height):
    """The <railSnap> element of a footprint, None when it has none."""
    data = rail_snap(footprint)
    if data is None:
        return None
    attributes = {
        "role": data["role"], "axis": "horizontal", "x": "0", "y": "0",
        "width": _num(width), "height": _num(height),
        "mount": data["mount"]}
    if data["mount_offset_mm"]:
        attributes["mount_offset_mm"] = _num(data["mount_offset_mm"])
    if data["role"] == "component":
        attributes["gap_mm"] = _num(data["gap_mm"])
    attributes["mm_per_px"] = _num(scale)
    attributes["basis"] = data["basis"]
    if data["source"]:
        attributes["source"] = data["source"]
    if data["verified"]:
        attributes["verified"] = data["verified"]
    return ET.Element("railSnap", attributes)


def _ceil10(value):
    return int(math.ceil(value / 10.0 - 1e-9)) * 10


def _uuid(stem, part):
    return "{%s}" % uuid.uuid5(uuid.NAMESPACE_URL,
                               "qelectrotech/ide-enclosures/%s/%s"
                               % (stem, part))


def _rect(x, y, width, height, style):
    return ET.Element("rect", {
        "x": _num(x), "y": _num(y), "width": _num(width),
        "height": _num(height), "rx": "0", "ry": "0",
        "antialias": "false", "style": style})


def _static_text(stem, part, x, y, text, size):
    return ET.Element("text", {
        "x": _num(x), "y": _num(y), "text": text, "font": FONT % size,
        "rotation": "0", "color": "#000000", "uuid": _uuid(stem, part)})


def _label_text(stem, x, y, size):
    node = ET.Element("dynamic_text", {
        "x": _num(x), "y": _num(y), "z": "5", "text_width": "-1",
        "Halignment": "AlignLeft", "Valignment": "AlignTop",
        "frame": "false", "rotation": "0",
        "keep_visual_rotation": "false", "text_from": "ElementInfo",
        "uuid": _uuid(stem, "label"), "font": FONT % size})
    ET.SubElement(node, "text").text = ""
    ET.SubElement(node, "info_name").text = "label"
    return node


def _definition(stem, names, infos, frame, parts, extra=()):
    """Build the definition of one element.

    frame is (left, top, drawing width, drawing height, right, bottom):
    the drawing starts at the origin of the element, which lies left and
    top pixels into the frame. The frame is on the grid of 10 like the
    other elements. extra are further children of the definition (the
    railSnap element), written after the information.
    """
    left, top, drawing_w, drawing_h, right, bottom = frame
    root = ET.Element("definition", {
        "version": DEFINITION_VERSION, "type": "element",
        "link_type": "simple",
        "width": str(_ceil10(left + drawing_w + right)),
        "height": str(_ceil10(top + drawing_h + bottom)),
        "hotspot_x": str(left), "hotspot_y": str(top)})
    ET.SubElement(root, "uuid", {"uuid": _uuid(stem, "element")})
    names_node = ET.SubElement(root, "names")
    for lang, text in names:
        ET.SubElement(names_node, "name", {"lang": lang}).text = text
    infos_node = ET.SubElement(root, "elementInformations")
    for name, value in infos:
        ET.SubElement(infos_node, "elementInformation",
                      {"show": "1", "name": name}).text = value
    for node in extra:
        root.append(node)
    ET.SubElement(root, "informations").text = (
        "Generated by misc/enclosure_layout from the catalog of "
        "extra-elements/ide-enclosures.\n" + LICENSE_TEXT)
    description = ET.SubElement(root, "description")
    for part in parts:
        description.append(part)
    ET.indent(root, space="    ")
    return root


def _serialize(root):
    return ET.tostring(root, encoding="unicode") + "\n"


def _size_text(height, width, depth):
    return "%sx%sx%s" % (_num(height), _num(width), _num(depth))


def enclosure_element(catalog, enclosure):
    """The front view of an enclosure, at the scale of the catalog."""
    scale = float(catalog["scale_mm_per_px"])
    stem = enclosure_stem(enclosure)
    width = enclosure["width"] / scale
    height = enclosure["height"] / scale
    size = _size_text(enclosure["height"], enclosure["width"],
                      enclosure["depth"])
    parts = [_rect(0, 0, width, height,
                   STYLE_BASE % ("normal", "normal"))]
    area = fit_area(enclosure)
    if area:
        x0, y0, x1, y1, _depth = area
        parts.append(_rect(x0 / scale, y0 / scale, (x1 - x0) / scale,
                           (y1 - y0) / scale,
                           STYLE_BASE % ("dashed", "thin")))
        usable = "usable area %sx%s mm" % (_num(y1 - y0), _num(x1 - x0))
    else:
        usable = "usable area not recorded"
    parts.append(_static_text(stem, "reference", 0, -26,
                              "%s %s" % (catalog["manufacturer"],
                                         enclosure["reference"]), 8))
    parts.append(_static_text(stem, "size", 0, -14,
                              "%s mm, %s" % (size, usable), 5))
    parts.append(_label_text(stem, 0, height + 2, 8))
    names = [
        ("en", "%s %s %s (front view)" % (
            catalog["manufacturer"], enclosure["reference"], size)),
        ("fr", "%s %s %s (vue de face)" % (
            catalog["manufacturer"], enclosure["reference"], size)),
    ]
    infos = [
        ("description", "%s %s mm (height x width x depth)" % (
            enclosure["family"], size)),
        ("manufacturer", catalog["manufacturer"]),
        ("manufacturer_reference", enclosure["reference"]),
    ]
    return _serialize(_definition(stem, names, infos,
                                  (10, 30, width, height, 10, 20), parts))


def _duct_parts(view, width, height):
    """The drawing of the realistic view or of the cover of a duct.

    Seen from above: the walls are one pixel thick. The realistic view
    shows the open duct with schematic slots in its base; the cover is a
    plain lid with the lines of its edges.
    """
    normal = STYLE_BASE % ("normal", "normal")
    thin = STYLE_BASE % ("normal", "thin")
    parts = [_rect(0, 0, width, height, normal)]
    wall = 1 if height > 4 else 0
    if view == "cover":
        parts.append(_rect(wall, wall, width - 2 * wall, height - 2 * wall,
                           STYLE_BASE % ("dashed", "thin")))
        return parts
    inner_w, inner_h = width - 2 * wall, height - 2 * wall
    parts.append(_rect(wall, wall, inner_w, inner_h, thin))
    slot_h = max(1, round(inner_h * 0.6))
    slot_y = wall + (inner_h - slot_h) / 2.0
    x = SLOT_PITCH_PX / 2.0
    while x + 3 <= width - wall:
        parts.append(_rect(x - 3, slot_y, 6, slot_h, thin))
        x += SLOT_PITCH_PX
    return parts


def footprint_element(catalog, footprint):
    """A footprint: the front view of a part, at the scale of the catalog."""
    scale = float(catalog["scale_mm_per_px"])
    stem = footprint_stem(footprint)
    view = footprint.get("view")
    width = footprint["width"] / scale
    height = footprint["height"] / scale
    if view:
        parts = _duct_parts(view, width, height)
    else:
        parts = [_rect(0, 0, width, height,
                       STYLE_BASE % ("normal", "normal"))]
    if width >= 24:
        parts.append(_static_text(stem, "name", 2, height / 2.0 - 3,
                                  footprint["label"], 5))
    parts.append(_label_text(stem, 1, 1, 5))
    name_en, name_fr = footprint["name_en"], footprint["name_fr"]
    if view == "realistic":
        name_en += " (realistic view)"
        name_fr += " (vue réaliste)"
    elif view == "cover":
        name_en += ", cover (top view)"
        name_fr += ", couvercle (vue de dessus)"
    names = [("en", name_en), ("fr", name_fr)]
    infos = [("description", "%s, %sx%sx%s mm (width x height x depth)" % (
        name_en, _num(footprint["width"]),
        _num(footprint["height"]), _num(footprint["depth"])))]
    if footprint.get("manufacturer"):
        infos.append(("manufacturer", footprint["manufacturer"]))
    if footprint.get("reference"):
        infos.append(("manufacturer_reference", footprint["reference"]))
    snap = _rail_snap_node(footprint, scale, width, height)
    return _serialize(_definition(stem, names, infos,
                                  (10, 10, width, height, 10, 10), parts,
                                  [snap] if snap is not None else []))


def part_element(catalog, kind, item):
    """The front view of the door or of the mounting plate of an enclosure."""
    scale = float(catalog["scale_mm_per_px"])
    prefix = DOOR_PREFIX if kind == "door" else PLATE_PREFIX
    stem = prefix + item["enclosure"]
    width = item["width"] / scale
    height = item["height"] / scale
    size = _size_text(item["height"], item["width"], item["depth"])
    data = item["data"]
    manufacturer = catalog["manufacturer"]
    if kind == "door":
        kind_en, kind_fr = "door", "porte"
        what = "%s door" % data.get("type", "opaque")
        if data.get("reversible"):
            what += ", reversible hinge side"
        title = "%s %s" % (manufacturer, item["enclosure"])
    else:
        kind_en, kind_fr = "mounting plate", "platine de montage"
        what = ("%s mounting plate" % data.get("material", "")).strip()
        title = "%s %s" % (manufacturer, item["reference"])
    parts = [_rect(0, 0, width, height, STYLE_BASE % ("normal", "normal"))]
    parts.append(_static_text(stem, "reference", 0, -26, title, 8))
    parts.append(_static_text(stem, "size", 0, -14,
                              "%s of %s, %s mm" % (kind_en, item["enclosure"],
                                                   size), 5))
    parts.append(_label_text(stem, 0, height + 2, 8))
    names = [
        ("en", "%s %s of %s %s (front view)" % (
            manufacturer, kind_en, manufacturer, item["enclosure"])),
        ("fr", "%s %s de %s %s (vue de face)" % (
            manufacturer, kind_fr, manufacturer, item["enclosure"])),
    ]
    infos = [
        ("description", "%s of %s, %s mm (height x width x thickness)" % (
            what, item["enclosure"], size)),
        ("manufacturer", manufacturer),
        ("manufacturer_reference", item["reference"]),
    ]
    return _serialize(_definition(stem, names, infos,
                                  (10, 30, width, height, 10, 20), parts))


def directory_file(names):
    root = ET.Element("qet-directory")
    names_node = ET.SubElement(root, "names")
    for lang, text in names:
        ET.SubElement(names_node, "name", {"lang": lang}).text = text
    ET.indent(root, space="    ")
    return _serialize(root)


def render_all(catalog):
    """Everything generate writes: {path relative to the catalog: text}."""
    files = {
        "qet_directory": directory_file([
            ("en", "IDE enclosures and cabinets"),
            ("fr", "Armoires et coffrets IDE")]),
        "enclosures/qet_directory": directory_file([
            ("en", "Enclosures (front view)"),
            ("fr", "Armoires et coffrets (vue de face)")]),
        "footprints/qet_directory": directory_file([
            ("en", "Footprints (DIN rails, ducts and modular devices)"),
            ("fr", "Empreintes (rails DIN, goulottes et appareils "
                   "modulaires)")]),
        "doors/qet_directory": directory_file([
            ("en", "Doors (front view)"),
            ("fr", "Portes (vue de face)")]),
        "plates/qet_directory": directory_file([
            ("en", "Mounting plates (front view)"),
            ("fr", "Platines de montage (vue de face)")]),
        VIEW_FOLDER["realistic"] + "/qet_directory": directory_file([
            ("en", "Ducts, realistic view (top view)"),
            ("fr", "Goulottes, vue réaliste (vue de dessus)")]),
        VIEW_FOLDER["cover"] + "/qet_directory": directory_file([
            ("en", "Duct covers (top view)"),
            ("fr", "Couvercles de goulottes (vue de dessus)")]),
        CUT_FOLDER + "/qet_directory": directory_file([
            ("en", "Ducts cut to standard lengths (footprints)"),
            ("fr", "Goulottes coupées à longueur standard (empreintes)")]),
    }
    for enclosure in catalog["enclosures"]:
        name = "enclosures/%s.elmt" % enclosure_stem(enclosure)
        files[name] = enclosure_element(catalog, enclosure)
        for kind, folder, item in (("door", "doors", door_item(enclosure)),
                                   ("plate", "plates",
                                    plate_item(enclosure))):
            if item:
                name = "%s/%s%s.elmt" % (
                    folder, DOOR_PREFIX if kind == "door" else PLATE_PREFIX,
                    enclosure["reference"])
                files[name] = part_element(catalog, kind, item)
    for footprint in catalog["footprints"]:
        name = "footprints/%s.elmt" % footprint_stem(footprint)
        files[name] = footprint_element(catalog, footprint)
        if footprint["kind"] != "duct":
            continue
        for view in ("realistic", "cover"):
            item = dict(footprint, view=view)
            name = "%s/%s.elmt" % (VIEW_FOLDER[view], footprint_stem(item))
            files[name] = footprint_element(catalog, item)
        for length in catalog.get("duct_cut_lengths", []):
            item = at_length(footprint, length)
            name = "%s/%s.elmt" % (CUT_FOLDER, footprint_stem(item))
            files[name] = footprint_element(catalog, item)
    return files


def generate(catalog, out_dir):
    """Write the elements; remove the ones the catalog no longer lists.

    The custom folder, where "length" writes, is left alone.
    """
    out_dir = Path(out_dir)
    files = render_all(catalog)
    for name, text in files.items():
        target = out_dir / name
        target.parent.mkdir(parents=True, exist_ok=True)
        with open(target, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(text)
    for sub in ("enclosures", "footprints", "doors", "plates", CUT_FOLDER,
                VIEW_FOLDER["realistic"], VIEW_FOLDER["cover"]):
        for old in (out_dir / sub).glob("*.elmt"):
            if "%s/%s" % (sub, old.name) not in files:
                old.unlink()
    return sorted(files)


def write_lengths(catalog, footprint_id, lengths, out_dir, view=None):
    """Write a stretchable footprint at each length (mm) to out_dir.

    A duct may be written in its realistic view or as its cover.
    """
    if view not in VIEW_PREFIX:
        raise ValueError("unknown view '%s'" % view)
    found = catalog["_by_stem"].get(VIEW_PREFIX[view] + footprint_id)
    if not found:
        raise ValueError("unknown footprint '%s'%s" % (
            footprint_id, " for the %s view" % view if view else ""))
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    directory = out_dir / "qet_directory"
    if not directory.exists():
        with open(directory, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(directory_file([
                ("en", "Rails and ducts at a chosen length"),
                ("fr", "Rails et goulottes à la longueur choisie")]))
    written = []
    for length in lengths:
        item = at_length(found[1], length)
        target = out_dir / (footprint_stem(item) + ".elmt")
        with open(target, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(footprint_element(catalog, item))
        written.append(target)
    return written


class Finding:
    def __init__(self, folio, severity, message):
        self.folio = folio
        self.severity = severity
        self.message = message

    def __str__(self):
        if self.folio is None:
            return "%s: %s" % (self.severity.upper(), self.message)
        return "%s: folio '%s': %s" % (self.severity.upper(), self.folio,
                                       self.message)


class _Placed:
    """An element placed on a folio, in millimetres."""

    def __init__(self, stem, kind, item, element, scale):
        self.stem = stem
        self.kind = kind
        self.item = item
        self.name = element.get("uuid", stem)
        self.x = float(element.get("x", "0")) * scale
        self.y = float(element.get("y", "0")) * scale
        try:
            self.turns = int(element.get("orientation", "0")) % 4
        except ValueError:
            self.turns = 0

    def corners(self):
        """Corners of the front view in folio coordinates."""
        width, height = self.item["width"], self.item["height"]
        points = [(0, 0), (width, 0), (width, height), (0, height)]
        cos, sin = [(1, 0), (0, 1), (-1, 0), (0, -1)][self.turns]
        return [(self.x + px * cos - py * sin, self.y + px * sin + py * cos)
                for px, py in points]

    def to_local(self, point):
        """A folio point in the coordinates of this element's front view."""
        cos, sin = [(1, 0), (0, 1), (-1, 0), (0, -1)][self.turns]
        dx, dy = point[0] - self.x, point[1] - self.y
        return (dx * cos + dy * sin, -dx * sin + dy * cos)

    def label(self):
        if self.kind in ("door", "plate"):
            return "%s of %s" % (self.kind, self.item["enclosure"])
        return self.item.get("reference") or self.item["id"]


def _bounds(points):
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    return (min(xs), min(ys), max(xs), max(ys))


def _stem_of(type_attribute):
    name = type_attribute.replace("\\", "/").rsplit("/", 1)[-1]
    return name[:-5] if name.endswith(".elmt") else name


def _part_findings(title, owner, part):
    """Findings for a door or a mounting plate placed on an enclosure."""
    findings = []
    reference = owner.item["reference"]
    if part.item["enclosure"] != reference:
        findings.append(Finding(title, "warning",
            "%s belongs to %s but is placed on %s"
            % (part.label(), part.item["enclosure"], reference)))
    local = [owner.to_local(c) for c in part.corners()]
    fx0, fy0, fx1, fy1 = _bounds(local)
    over = max(-fx0, -fy0, fx1 - owner.item["width"],
               fy1 - owner.item["height"])
    if over > EPSILON:
        findings.append(Finding(title, "error",
            "%s leaves the outline of %s by %s mm"
            % (part.label(), reference, _num(over))))
    return findings


def check_diagram(catalog, title, elements):
    """Findings for one folio; elements are its <element> nodes."""
    scale = float(catalog["scale_mm_per_px"])
    enclosures, footprints, parts = [], [], []
    for element in elements:
        stem = _stem_of(element.get("type", ""))
        found = lookup(catalog, stem)
        if not found:
            continue
        kind, item = found
        placed = _Placed(stem, kind, item, element, scale)
        if kind == "enclosure":
            enclosures.append(placed)
        elif kind == "footprint":
            footprints.append(placed)
        else:
            parts.append(placed)

    findings = []
    members = {id(e): [] for e in enclosures}
    for footprint in footprints + parts:
        centre = [sum(c) / 4.0 for c in zip(*footprint.corners())]
        owner = None
        for enclosure in enclosures:
            lx, ly = enclosure.to_local(centre)
            if (0 <= lx <= enclosure.item["width"]
                    and 0 <= ly <= enclosure.item["height"]):
                owner = enclosure
                break
        if owner is None:
            findings.append(Finding(title, "warning",
                "%s is not inside any enclosure" % footprint.label()))
        elif footprint.kind == "footprint":
            members[id(owner)].append(footprint)
        else:
            findings += _part_findings(title, owner, footprint)

    for enclosure in enclosures:
        area = fit_area(enclosure.item)
        reference = enclosure.item["reference"]
        if area is None:
            findings.append(Finding(title, "warning",
                "%s has no usable area in the catalog: footprints are "
                "checked against its outer size" % reference))
            area = (0, 0, enclosure.item["width"], enclosure.item["height"],
                    enclosure.item["depth"])
        x0, y0, x1, y1, depth = area
        boxes = []
        for footprint in members[id(enclosure)]:
            local = [enclosure.to_local(c) for c in footprint.corners()]
            fx0, fy0, fx1, fy1 = _bounds(local)
            boxes.append((footprint, (fx0, fy0, fx1, fy1)))
            over = max(x0 - fx0, y0 - fy0, fx1 - x1, fy1 - y1)
            if over > EPSILON:
                findings.append(Finding(title, "error",
                    "%s leaves the usable area of %s by %s mm"
                    % (footprint.label(), reference, _num(over))))
            if footprint.item["depth"] > depth + EPSILON:
                findings.append(Finding(title, "error",
                    "%s is %s mm deep, %s allows %s mm"
                    % (footprint.label(), _num(footprint.item["depth"]),
                       reference, _num(depth))))
        for index, (first, a) in enumerate(boxes):
            for second, b in boxes[index + 1:]:
                # A cover lies over its duct on purpose.
                if "cover" in (first.item.get("view"),
                               second.item.get("view")):
                    continue
                # A device sits on its rail on purpose.
                if {first.item.get("kind"),
                        second.item.get("kind")} == {"rail", "module"}:
                    continue
                if (min(a[2], b[2]) - max(a[0], b[0]) > EPSILON
                        and min(a[3], b[3]) - max(a[1], b[1]) > EPSILON):
                    findings.append(Finding(title, "error",
                        "%s overlaps %s in %s" % (first.label(),
                                                  second.label(), reference)))
    if footprints and not enclosures:
        findings.append(Finding(title, "warning",
            "footprints placed but no enclosure on this folio"))
    return findings


def rail_snap_findings(catalog, title, elements):
    """Warnings for the footprints of a folio whose fixing position on the
    rail is assumed or not yet compared with its source.

    One warning for each kind of footprint: the position on the rail is
    what rail snapping aligns, so a wrong value moves every device.
    """
    findings, seen = [], set()
    for element in elements:
        found = lookup(catalog, _stem_of(element.get("type", "")))
        if not found or found[0] != "footprint":
            continue
        item = found[1]
        data = rail_snap(item)
        if data is None or item["id"].split("_L")[0] in seen:
            continue
        seen.add(item["id"].split("_L")[0])
        if data["basis"] == "assumed":
            reason = "is assumed"
        elif data["basis"] != "standard" and not data["verified"]:
            reason = "comes from the %s but is not verified" % data["basis"]
        else:
            continue
        findings.append(Finding(title, "warning",
            "%s: the fixing position on the rail %s; check it against the "
            "data sheet or an image and record it in the catalog "
            "(rail_snap)" % (item.get("reference") or item["id"], reason)))
    return findings


def _info(element, name):
    node = element.find(
        "elementInformations/elementInformation[@name='%s']" % name)
    return (node.text or "").strip() if node is not None else ""


class _Component:
    """A symbol of the schema, with the cabinet it belongs to."""

    def __init__(self, folio, element, folio_cabinet, shared_prefixes):
        self.folio = folio
        self.label = _info(element, "label")
        self.reference = _info(element, "manufacturer_reference")
        self.manufacturer = _info(element, "manufacturer")
        own = _info(element, "location")
        self.cabinet = own or folio_cabinet
        self.explicit = bool(own)
        # Terminals are legitimately spread over several cabinets.
        self.shared = (element.get("prefix", "") in shared_prefixes
                       or "terminal" in element.get("type", "").lower())


def _cabinet_entry(cabinets, name):
    return cabinets.setdefault(name, {
        "enclosure": None, "folio": None, "components": [], "items": [],
        "parts": [], "profiles": [], "unreferenced": 0, "occupied": 0,
        "available": None})


def _share(value, total):
    return "%d%%" % round(100.0 * value / total)


def analyse_project(catalog, folios):
    """Check a project; folios is a list of (title, location, elements).

    The cabinet of a symbol is its own "location" information, else the
    "location" (locmach) of its folio. An enclosure element stands for the
    cabinet named like its label. Returns (findings, cabinets), where
    cabinets maps a cabinet name to its enclosure, contents and occupancy.
    """
    shared = tuple(catalog.get("shared_prefixes", SHARED_PREFIXES))
    findings, cabinets = [], {}
    for title, folio_cabinet, elements in folios:
        findings += check_diagram(catalog, title, elements)
        findings += rail_snap_findings(catalog, title, elements)
        for element in elements:
            found = lookup(catalog, _stem_of(element.get("type", "")))
            if found and found[0] == "footprint":
                # Rails and ducts are bought by the metre of the layout; a
                # cover comes with its duct.
                profile = found[1]
                name = _info(element, "location") or folio_cabinet
                if (name and profile["kind"] in ("duct", "rail")
                        and profile.get("view") != "cover"):
                    _cabinet_entry(cabinets, name)["profiles"].append(profile)
                continue
            if found:
                name = (_info(element, "label") or _info(element, "location")
                        or folio_cabinet)
                kind, item = found
                if not name:
                    findings.append(Finding(title, "warning",
                        "%s has no label: its contents cannot be listed"
                        % ("enclosure " + item["reference"]
                           if kind == "enclosure" else
                           "%s of %s" % (kind, item["enclosure"]))))
                    continue
                entry = _cabinet_entry(cabinets, name)
                if kind != "enclosure":
                    entry["parts"].append((kind, item, title))
                elif entry["enclosure"]:
                    findings.append(Finding(title, "warning",
                        "cabinet %s has several enclosure elements" % name))
                else:
                    entry["enclosure"], entry["folio"] = item, title
                continue
            component = _Component(title, element, folio_cabinet, shared)
            if component.cabinet and (component.label
                                      or component.reference):
                _cabinet_entry(cabinets, component.cabinet)[
                    "components"].append(component)
    findings += _duplicate_findings(cabinets)
    for name in sorted(cabinets):
        findings += _cabinet_findings(catalog, name, cabinets[name])
    return findings, cabinets


def _duplicate_findings(cabinets):
    seen = {}
    for name, entry in cabinets.items():
        for component in entry["components"]:
            if component.label:
                seen.setdefault(normalize_reference(component.label),
                                []).append((name, component))
    findings = []
    for items in seen.values():
        names = sorted({name for name, _ in items})
        if len(names) < 2:
            continue
        where = ", ".join("%s (folio '%s')" % (
            name, ", ".join(sorted({c.folio for n, c in items if n == name})))
            for name in names)
        everything_shared = all(c.shared for _, c in items)
        findings.append(Finding(None,
            "info" if everything_shared else "warning",
            "%s is in several cabinets: %s%s" % (
                items[0][1].label, where,
                " (terminal: expected)" if everything_shared else "")))
    return findings


def _cabinet_findings(catalog, name, entry):
    """Fill the contents of one cabinet and check that they fit."""
    findings = []
    unique, seen = [], set()
    for component in entry["components"]:
        key = normalize_reference(component.label)
        if component.label and key in seen:
            continue
        seen.add(key)
        unique.append(component)
    groups = {}
    for component in unique:
        if not component.reference:
            entry["unreferenced"] += 1
            continue
        key = (normalize_reference(component.manufacturer),
               normalize_reference(component.reference))
        group = groups.setdefault(key, {
            "manufacturer": component.manufacturer,
            "reference": component.reference, "quantity": 0})
        group["quantity"] += 1
    entry["items"] = [groups[key] for key in sorted(groups)]

    item = entry["enclosure"]
    for kind, part, _folio in entry["parts"]:
        if item and part["enclosure"] != item["reference"]:
            findings.append(Finding(None, "warning",
                "%s: the %s belongs to %s but the cabinet is %s"
                % (name, kind, part["enclosure"], item["reference"])))
        elif not item:
            findings.append(Finding(None, "info",
                "%s: the %s of %s has no enclosure element"
                % (name, kind, part["enclosure"])))
    area = None
    if item:
        area = fit_area(item)
        if area is None:
            findings.append(Finding(None, "warning",
                "%s (%s) has no usable area in the catalog: the outer size "
                "is used" % (name, item["reference"])))
            area = (0, 0, item["width"], item["height"], item["depth"])
        entry["available"] = (area[2] - area[0]) * (area[3] - area[1])
    for group in entry["items"]:
        device = find_device(catalog, group["manufacturer"],
                             group["reference"])
        group["device"] = device
        if device is None:
            findings.append(Finding(None, "info",
                "%s: no footprint linked to %s, not counted in the space "
                "check" % (name, group["reference"])))
            continue
        entry["occupied"] += (group["quantity"] * device["width"]
                              * device["height"])
        if not area:
            continue
        fits = any(w <= area[2] - area[0] + EPSILON
                   and h <= area[3] - area[1] + EPSILON
                   for w, h in ((device["width"], device["height"]),
                                (device["height"], device["width"])))
        if not fits:
            findings.append(Finding(None, "error",
                "%s: %s (%sx%s mm) does not fit in the usable area of %s"
                % (name, group["reference"], _num(device["width"]),
                   _num(device["height"]), item["reference"])))
        if device["depth"] > area[4] + EPSILON:
            findings.append(Finding(None, "error",
                "%s: %s is %s mm deep, %s allows %s mm"
                % (name, group["reference"], _num(device["depth"]),
                   item["reference"], _num(area[4]))))
    if item and entry["available"]:
        limit = float(catalog.get("fill_warning", FILL_WARNING))
        if entry["occupied"] > entry["available"] + EPSILON:
            findings.append(Finding(None, "error",
                "%s: the devices take %s of the usable area of %s"
                % (name, _share(entry["occupied"], entry["available"]),
                   item["reference"])))
        elif entry["occupied"] > limit * entry["available"]:
            findings.append(Finding(None, "warning",
                "%s: the devices take %s of the usable area of %s; ducts "
                "and spacing are not counted"
                % (name, _share(entry["occupied"], entry["available"]),
                   item["reference"])))
    elif entry["components"] and not item:
        findings.append(Finding(None, "info",
            "%s has no enclosure element: the space is not checked" % name))
    return findings


def read_project(project_path):
    """The folios of a .qet project as (title, location, elements)."""
    tree = ET.parse(str(project_path))
    folios = []
    for index, diagram in enumerate(tree.getroot().iter("diagram"), 1):
        title = diagram.get("title") or "folio %d" % index
        elements = [e for e in diagram.iter("element") if e.get("type")]
        folios.append((title, (diagram.get("locmach") or "").strip(),
                       elements))
    return folios


def check_project(catalog, project_path):
    """Check every folio of a .qet project; returns the findings."""
    return analyse_project(catalog, read_project(project_path))[0]


def _profile_groups(entry):
    """The rails and ducts of a cabinet: [(item, quantity)], by length."""
    groups = {}
    for profile in entry["profiles"]:
        key = (profile["kind"], profile.get("reference", ""),
               profile["id"], profile["width"])
        groups.setdefault(key, [profile, 0])[1] += 1
    return [tuple(groups[key]) for key in sorted(groups)]


def _profile_name(profile):
    if profile["kind"] == "rail":
        return "DIN rail %s" % profile["label"]
    return "%s duct %s" % (profile.get("manufacturer", ""),
                           profile.get("reference") or profile["label"])


def report_rows(cabinets):
    """The contents of every cabinet as rows of strings, header first."""
    rows = [["cabinet", "enclosure", "manufacturer", "reference",
             "quantity", "width mm", "height mm", "depth mm"]]
    for name in sorted(cabinets):
        entry = cabinets[name]
        enclosure = entry["enclosure"]["reference"] if entry["enclosure"] \
            else ""
        for group in entry["items"]:
            device = group.get("device")
            rows.append([name, enclosure, group["manufacturer"],
                         group["reference"], str(group["quantity"])]
                        + ([_num(device[k]) for k in
                            ("width", "height", "depth")] if device
                           else ["", "", ""]))
        for profile, quantity in _profile_groups(entry):
            rows.append([name, enclosure, profile.get("manufacturer", ""),
                         profile.get("reference") or profile["label"],
                         str(quantity), _num(profile["width"]),
                         _num(profile["height"]), _num(profile["depth"])])
    return rows


def render_report(cabinets):
    lines = []
    for name in sorted(cabinets):
        entry = cabinets[name]
        item = entry["enclosure"]
        head = "Cabinet %s" % name
        if item:
            head += ": %s %s" % (item["family"], item["reference"])
        lines.append(head)
        for kind, part, _folio in entry["parts"]:
            lines.append("  %s %s (%sx%s mm)" % (
                kind, part["reference"], _num(part["height"]),
                _num(part["width"])))
        for group in entry["items"]:
            device = group.get("device")
            size = ("%sx%sx%s mm" % tuple(_num(device[k]) for k in
                                          ("width", "height", "depth"))
                    if device else "no footprint")
            lines.append("  %3d x %s %s (%s)" % (
                group["quantity"], group["manufacturer"],
                group["reference"], size))
        if entry["unreferenced"]:
            lines.append("  %d component(s) without manufacturer "
                         "reference" % entry["unreferenced"])
        for profile, quantity in _profile_groups(entry):
            lines.append("  %3d x %s, %s mm long" % (
                quantity, _profile_name(profile), _num(profile["width"])))
        if entry["available"]:
            lines.append("  space: %s of the usable area" % _share(
                entry["occupied"], entry["available"]))
        lines.append("")
    return "\n".join(lines)


def article_rows(catalog):
    """The articles of the ducts as rows of strings, header first.

    One row per orderable reference. The length is the one of the
    profile as sold; the cover comes with the duct.
    """
    rows = [["manufacturer", "reference", "description", "width mm",
             "height mm", "length mm", "interior mm", "cover", "source"]]
    for duct in catalog["footprints"]:
        if duct["kind"] != "duct":
            continue
        interior = duct.get("interior")
        for article in duct.get("articles", []):
            rows.append([
                duct.get("manufacturer", ""), article["reference"],
                "%s, %s" % (duct["name_en"], article["finish"]),
                _num(duct["height"]), _num(duct["depth"]),
                _num(duct["width"]),
                "%sx%s" % (_num(interior["width"]), _num(interior["height"]))
                if interior else "",
                "included" if duct.get("cover", {}).get("included") else "",
                duct.get("source", "")])
    return rows


def link_device(catalog, reference, manufacturer=None, footprint=None,
                size=None):
    """Add or replace a device in the devices file; returns the entry."""
    entry = {"reference": reference}
    if manufacturer:
        entry["manufacturer"] = manufacturer
    if footprint:
        if footprint not in {f["id"] for f in catalog["footprints"]}:
            raise ValueError("unknown footprint '%s'" % footprint)
        entry["footprint"] = footprint
    else:
        entry.update(size)
    devices = [d for d in read_devices(catalog["devices_path"])
               if (normalize_reference(d["reference"]),
                   normalize_reference(d.get("manufacturer")))
               != (normalize_reference(reference),
                   normalize_reference(manufacturer))]
    devices.append(entry)
    devices.sort(key=lambda d: normalize_reference(d["reference"]))
    write_devices(catalog["devices_path"], devices)
    return entry


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--catalog", help="catalog.json to use")
    parser.add_argument("--devices", help="devices.json to use")
    sub = parser.add_subparsers(dest="command", required=True)
    gen = sub.add_parser("generate", help="write the .elmt files")
    gen.add_argument("--out", help="output folder (default: next to the "
                     "catalog)")
    chk = sub.add_parser("check", help="check the footprints and cabinets "
                         "of a project")
    chk.add_argument("project", help=".qet file to read (never modified)")
    rep = sub.add_parser("report", help="list the contents of each cabinet")
    rep.add_argument("project", help=".qet file to read (never modified)")
    rep.add_argument("--csv", help="also write the contents to this file")
    lnk = sub.add_parser("link", help="link a device reference to its "
                         "footprint")
    lnk.add_argument("reference", help="manufacturer reference of the device")
    lnk.add_argument("--manufacturer", help="only match this manufacturer")
    lnk.add_argument("--footprint", help="id of a footprint of the catalog")
    for key in ("width", "height", "depth"):
        lnk.add_argument("--" + key, type=float, help="%s in mm" % key)
    lng = sub.add_parser("length", help="write a rail or a duct at a chosen "
                         "length")
    lng.add_argument("footprint", help="id of a stretchable footprint")
    lng.add_argument("lengths", type=int, nargs="+", help="lengths in mm")
    lng.add_argument("--out", help="output folder (default: custom, next "
                     "to the catalog)")
    lng.add_argument("--view", choices=("realistic", "cover"),
                     help="write a duct in this view instead of its "
                     "footprint")
    art = sub.add_parser("articles", help="list the articles of the ducts "
                         "as CSV")
    art.add_argument("--csv", help="write the list to this file instead of "
                     "the screen")
    args = parser.parse_args(argv)

    catalog_path = Path(args.catalog) if args.catalog else None
    catalog = load_catalog(catalog_path, args.devices)
    if args.command == "articles":
        handle = (open(args.csv, "w", encoding="utf-8", newline="")
                  if args.csv else sys.stdout)
        csv.writer(handle, lineterminator="\n").writerows(
            article_rows(catalog))
        if args.csv:
            handle.close()
        return 0
    if args.command == "length":
        out = Path(args.out) if args.out else (
            (catalog_path.parent if catalog_path else DEFAULT_DIR)
            / "custom")
        try:
            written = write_lengths(catalog, args.footprint, args.lengths,
                                    out, args.view)
        except ValueError as error:
            parser.error(str(error))
        for path in written:
            print(path)
        return 0
    if args.command == "generate":
        out = Path(args.out) if args.out else (
            catalog_path.parent if catalog_path else DEFAULT_DIR)
        written = generate(catalog, out)
        print("%d files written in %s" % (len(written), out))
        return 0
    if args.command == "link":
        size = {k: getattr(args, k) for k in ("width", "height", "depth")}
        if not args.footprint and None in size.values():
            parser.error("give --footprint, or --width, --height and "
                         "--depth")
        entry = link_device(catalog, args.reference, args.manufacturer,
                            args.footprint, size)
        print("linked: %s" % json.dumps(entry, ensure_ascii=False))
        return 0
    findings, cabinets = analyse_project(
        catalog, read_project(args.project))
    if args.command == "report":
        print(render_report(cabinets))
        if args.csv:
            with open(args.csv, "w", encoding="utf-8", newline="") as handle:
                csv.writer(handle).writerows(report_rows(cabinets))
    for finding in findings:
        print(finding)
    errors = sum(1 for f in findings if f.severity == "error")
    warnings = sum(1 for f in findings if f.severity == "warning")
    print("%d error(s), %d warning(s), %d note(s)" % (
        errors, warnings, len(findings) - errors - warnings))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
