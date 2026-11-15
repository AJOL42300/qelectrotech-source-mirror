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
"""Enclosure layout helper: IDE cabinets as elements, and a fit check.

Two commands, both driven by extra-elements/ide-enclosures/catalog.json:

    python3 misc/enclosure_layout/enclosure_layout.py generate
    python3 misc/enclosure_layout/enclosure_layout.py check project.qet

"generate" writes the front-view enclosure elements and the generic
footprint elements (DIN modules, DIN rails) next to the catalog. "check"
reads a project, finds the enclosure and footprint elements placed on each
folio and reports the footprints that leave the usable area of their
enclosure, are deeper than it, or overlap one another. See
docs/enclosure_layout.md.
"""

import argparse
import json
import math
import sys
import uuid
import xml.etree.ElementTree as ET
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_DIR = REPO_ROOT / "extra-elements" / "ide-enclosures"
ENCLOSURE_PREFIX = "ide_enc_"
FOOTPRINT_PREFIX = "ide_fp_"
# Millimetres of tolerance when comparing positions and sizes.
EPSILON = 0.01
DEFINITION_VERSION = "0.100.0"
LICENSE_TEXT = ("License: see "
                "http://qelectrotech.org/wiki/doc/elements_license")
STYLE_BASE = "line-style:%s;line-weight:%s;filling:none;color:black"
FONT = "Liberation Sans,%d,-1,5,50,0,0,0,0,0,Regular"


def load_catalog(path=None):
    """Return the catalog read from path (default: the shipped one)."""
    path = Path(path) if path else DEFAULT_DIR / "catalog.json"
    with open(path, encoding="utf-8") as handle:
        catalog = json.load(handle)
    catalog["_by_stem"] = {}
    for enclosure in catalog["enclosures"]:
        stem = ENCLOSURE_PREFIX + enclosure["reference"]
        catalog["_by_stem"][stem] = ("enclosure", enclosure)
    for footprint in catalog["footprints"]:
        stem = FOOTPRINT_PREFIX + footprint["id"]
        catalog["_by_stem"][stem] = ("footprint", footprint)
    return catalog


def enclosure_stem(enclosure):
    return ENCLOSURE_PREFIX + enclosure["reference"]


def footprint_stem(footprint):
    return FOOTPRINT_PREFIX + footprint["id"]


def fit_area(enclosure):
    """The area, in millimetres, where footprints must stay.

    Returns (x0, y0, x1, y1, depth) relative to the top left corner of the
    front view, or None when the catalog does not give the usable space.
    A known mounting plate wins over the usable space. The area is
    centred in the front view.
    """
    plate = enclosure.get("plate")
    usable = enclosure.get("usable")
    if plate:
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


def _definition(stem, names, infos, frame, parts):
    """Build the definition of one element.

    frame is (left, top, drawing width, drawing height, right, bottom):
    the drawing starts at the origin of the element, which lies left and
    top pixels into the frame. The frame is on the grid of 10 like the
    other elements.
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


def footprint_element(catalog, footprint):
    """A footprint: the front view of a part, at the scale of the catalog."""
    scale = float(catalog["scale_mm_per_px"])
    stem = footprint_stem(footprint)
    width = footprint["width"] / scale
    height = footprint["height"] / scale
    parts = [_rect(0, 0, width, height, STYLE_BASE % ("normal", "normal"))]
    if width >= 24:
        parts.append(_static_text(stem, "name", 2, height / 2.0 - 3,
                                  footprint["label"], 5))
    parts.append(_label_text(stem, 1, 1, 5))
    names = [("en", footprint["name_en"]), ("fr", footprint["name_fr"])]
    infos = [("description", "%s, %sx%sx%s mm (width x height x depth)" % (
        footprint["name_en"], _num(footprint["width"]),
        _num(footprint["height"]), _num(footprint["depth"])))]
    return _serialize(_definition(stem, names, infos,
                                  (10, 10, width, height, 10, 10), parts))


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
            ("en", "Footprints (DIN rails and modular devices)"),
            ("fr", "Empreintes (rails DIN et appareils modulaires)")]),
    }
    for enclosure in catalog["enclosures"]:
        name = "enclosures/%s.elmt" % enclosure_stem(enclosure)
        files[name] = enclosure_element(catalog, enclosure)
    for footprint in catalog["footprints"]:
        name = "footprints/%s.elmt" % footprint_stem(footprint)
        files[name] = footprint_element(catalog, footprint)
    return files


def generate(catalog, out_dir):
    """Write the elements; remove the ones the catalog no longer lists."""
    out_dir = Path(out_dir)
    files = render_all(catalog)
    for name, text in files.items():
        target = out_dir / name
        target.parent.mkdir(parents=True, exist_ok=True)
        with open(target, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(text)
    for sub in ("enclosures", "footprints"):
        for old in (out_dir / sub).glob("*.elmt"):
            if "%s/%s" % (sub, old.name) not in files:
                old.unlink()
    return sorted(files)


class Finding:
    def __init__(self, folio, severity, message):
        self.folio = folio
        self.severity = severity
        self.message = message

    def __str__(self):
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
        return self.item.get("reference") or self.item["id"]


def _bounds(points):
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    return (min(xs), min(ys), max(xs), max(ys))


def _stem_of(type_attribute):
    name = type_attribute.replace("\\", "/").rsplit("/", 1)[-1]
    return name[:-5] if name.endswith(".elmt") else name


def check_diagram(catalog, title, elements):
    """Findings for one folio; elements are its <element> nodes."""
    scale = float(catalog["scale_mm_per_px"])
    enclosures, footprints = [], []
    for element in elements:
        stem = _stem_of(element.get("type", ""))
        found = catalog["_by_stem"].get(stem)
        if not found:
            continue
        kind, item = found
        placed = _Placed(stem, kind, item, element, scale)
        (enclosures if kind == "enclosure" else footprints).append(placed)

    findings = []
    members = {id(e): [] for e in enclosures}
    for footprint in footprints:
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
        else:
            members[id(owner)].append(footprint)

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
                if (min(a[2], b[2]) - max(a[0], b[0]) > EPSILON
                        and min(a[3], b[3]) - max(a[1], b[1]) > EPSILON):
                    findings.append(Finding(title, "error",
                        "%s overlaps %s in %s" % (first.label(),
                                                  second.label(), reference)))
    if footprints and not enclosures:
        findings.append(Finding(title, "warning",
            "footprints placed but no enclosure on this folio"))
    return findings


def check_project(catalog, project_path):
    """Check every folio of a .qet project; returns the findings."""
    tree = ET.parse(str(project_path))
    findings = []
    for index, diagram in enumerate(tree.getroot().iter("diagram"), 1):
        title = diagram.get("title") or "folio %d" % index
        elements = [e for e in diagram.iter("element") if e.get("type")]
        findings += check_diagram(catalog, title, elements)
    return findings


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--catalog", help="catalog.json to use")
    sub = parser.add_subparsers(dest="command", required=True)
    gen = sub.add_parser("generate", help="write the .elmt files")
    gen.add_argument("--out", help="output folder (default: next to the "
                     "catalog)")
    chk = sub.add_parser("check", help="check the footprints of a project")
    chk.add_argument("project", help=".qet file to read (never modified)")
    args = parser.parse_args(argv)

    catalog_path = Path(args.catalog) if args.catalog else None
    catalog = load_catalog(catalog_path)
    if args.command == "generate":
        out = Path(args.out) if args.out else (
            catalog_path.parent if catalog_path else DEFAULT_DIR)
        written = generate(catalog, out)
        print("%d files written in %s" % (len(written), out))
        return 0
    findings = check_project(catalog, args.project)
    for finding in findings:
        print(finding)
    errors = sum(1 for f in findings if f.severity == "error")
    print("%d error(s), %d warning(s)" % (errors, len(findings) - errors))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
