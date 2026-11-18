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
"""Tests of misc/enclosure_layout: the catalog, the generated elements and
the fit check. Run with: python3 -m unittest tst_enclosurelayout
"""

import contextlib
import copy
import io
import re
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "misc" / "enclosure_layout"))
import enclosure_layout as el  # noqa: E402

ENC = "embed://import/ide-enclosures/enclosures/ide_enc_%s.elmt"
FP = "embed://import/ide-enclosures/footprints/ide_fp_%s.elmt"


def placed(type_, x, y, orientation=0):
    return ET.Element("element", {"type": type_, "x": str(x),
                                  "y": str(y),
                                  "orientation": str(orientation)})


def messages(findings, severity=None):
    return [f.message for f in findings
            if severity is None or f.severity == severity]


class CatalogTest(unittest.TestCase):
    def setUp(self):
        self.catalog = el.load_catalog()

    def test_references_are_unique_and_match_their_size(self):
        seen = set()
        for enc in self.catalog["enclosures"]:
            self.assertNotIn(enc["reference"], seen)
            seen.add(enc["reference"])
            digits = re.sub(r"\D", "", enc["reference"])
            size = "".join(str(enc[k] // 10)
                           for k in ("height", "width", "depth"))
            self.assertEqual(digits, size, enc["reference"])

    def test_usable_space_fits_in_the_enclosure(self):
        for enc in self.catalog["enclosures"]:
            usable = enc["usable"]
            if usable:
                for key in ("height", "width", "depth"):
                    self.assertLessEqual(usable[key], enc[key],
                                         enc["reference"])

    def test_every_enclosure_is_verified_and_has_a_door_and_a_plate(self):
        for enc in self.catalog["enclosures"]:
            ref = enc["reference"]
            self.assertTrue(enc["source"].startswith("https://"), ref)
            self.assertRegex(enc["verified"], r"^\d{4}-\d\d-\d\d$", ref)
            self.assertIn("ide_door_" + ref, self.catalog["_by_stem"])
            self.assertIn("ide_plate_" + ref, self.catalog["_by_stem"])
            self.assertRegex(enc["plate"]["reference"], r"^PL\d+$", ref)
            self.assertLessEqual(enc["plate"]["width"], enc["width"], ref)
            self.assertLessEqual(enc["plate"]["height"], enc["height"], ref)

    def test_ducts_and_rails_are_stretchable_with_a_default_length(self):
        stretch = [f for f in self.catalog["footprints"]
                   if f.get("stretch")]
        ducts = [f for f in stretch if f["kind"] == "duct"]
        self.assertEqual(len(ducts), 10)
        for duct in ducts:
            self.assertEqual(duct["manufacturer"], "Efapel")
            self.assertTrue(duct["reference"].endswith(" CCZ"))
            self.assertEqual(duct["width"], 2000)
            self.assertTrue(duct["source"].startswith("https://"))
        self.assertEqual({f["id"] for f in stretch if f["kind"] == "rail"},
                         {"rail_ts35_7_5", "rail_ts35_15"})

    def test_generated_files_are_up_to_date(self):
        files = el.render_all(self.catalog)
        for name, text in files.items():
            on_disk = ((el.DEFAULT_DIR / name).read_bytes().decode("utf-8")
                       .replace("\r\n", "\n"))
            self.assertEqual(on_disk, text, name + " is stale: run generate")
        for sub in ("enclosures", "footprints", "doors", "plates"):
            for path in (el.DEFAULT_DIR / sub).glob("*.elmt"):
                self.assertIn("%s/%s" % (sub, path.name), files)


class ElementTest(unittest.TestCase):
    def setUp(self):
        self.catalog = el.load_catalog()

    def test_frames_are_on_the_grid_and_hold_the_drawing(self):
        for name, text in el.render_all(self.catalog).items():
            if not name.endswith(".elmt"):
                continue
            root = ET.fromstring(text)
            frame = [int(root.get(k)) for k in
                     ("width", "height", "hotspot_x", "hotspot_y")]
            self.assertTrue(all(v % 10 == 0 for v in frame), name)
            rect = root.find("description/rect")
            right = int(root.get("hotspot_x")) + float(rect.get("width"))
            bottom = int(root.get("hotspot_y")) + float(rect.get("height"))
            self.assertLessEqual(right, int(root.get("width")), name)
            self.assertLessEqual(bottom, int(root.get("height")), name)

    def test_size_is_drawn_at_the_catalog_scale(self):
        enc = next(e for e in self.catalog["enclosures"]
                   if e["reference"] == "GN403020")
        root = ET.fromstring(el.enclosure_element(self.catalog, enc))
        outer, usable = root.findall("description/rect")
        self.assertEqual(float(outer.get("width")), 150)
        self.assertEqual(float(outer.get("height")), 200)
        self.assertEqual((float(usable.get("x")), float(usable.get("y"))),
                         (12.5, 12.5))
        self.assertEqual(float(usable.get("width")), 125)
        self.assertEqual(float(usable.get("height")), 175)

    def test_modular_device_is_17_5_mm_per_module(self):
        fp = next(f for f in self.catalog["footprints"]
                  if f["id"] == "din_4m")
        root = ET.fromstring(el.footprint_element(self.catalog, fp))
        rect = root.find("description/rect")
        self.assertEqual(float(rect.get("width")) * 2, 70)


class FitAreaTest(unittest.TestCase):
    def test_usable_space_is_centred(self):
        enc = {"height": 400, "width": 300, "depth": 200,
               "usable": {"height": 350, "width": 250, "depth": 179},
               "plate": None}
        self.assertEqual(el.fit_area(enc), (25, 25, 275, 375, 179))

    def test_smaller_of_plate_and_usable_space_wins(self):
        enc = {"height": 400, "width": 300, "depth": 200,
               "usable": {"height": 350, "width": 250, "depth": 179},
               "plate": {"height": 300, "width": 200}}
        self.assertEqual(el.fit_area(enc), (50, 50, 250, 350, 179))
        # A plate taller than the usable space, like PL4030 in GN403020
        enc["plate"] = {"height": 370, "width": 250}
        self.assertEqual(el.fit_area(enc), (25, 25, 275, 375, 179))

    def test_unknown_when_nothing_is_recorded(self):
        enc = {"height": 300, "width": 300, "depth": 200,
               "usable": None, "plate": None}
        self.assertIsNone(el.fit_area(enc))


class CheckTest(unittest.TestCase):
    def setUp(self):
        self.catalog = el.load_catalog()

    def check(self, *elements):
        return el.check_diagram(self.catalog, "test", list(elements))

    def test_footprints_inside_pass(self):
        # 50 px = 100 mm: well inside the 250x350 mm usable area of GN403020
        findings = self.check(placed(ENC % "GN403020", 0, 0),
                              placed(FP % "din_4m", 50, 50),
                              placed(FP % "din_4m", 90, 50))
        self.assertEqual(findings, [])

    def test_footprint_leaving_the_usable_area_is_an_error(self):
        # 10 px = 20 mm from the left: 5 mm outside the usable area
        findings = self.check(placed(ENC % "GN403020", 0, 0),
                              placed(FP % "din_4m", 10, 50))
        self.assertEqual(len(messages(findings, "error")), 1)
        self.assertIn("by 5 mm", messages(findings)[0])

    def test_rotation_is_taken_into_account(self):
        # Turned a quarter turn, the 70x90 mm device extends to the left of
        # its origin: it leaves the area where the upright one would not.
        upright = self.check(placed(ENC % "GN403020", 0, 0),
                             placed(FP % "din_4m", 50, 20))
        turned = self.check(placed(ENC % "GN403020", 0, 0),
                            placed(FP % "din_4m", 50, 20, 1))
        self.assertEqual(upright, [])
        self.assertEqual(len(messages(turned, "error")), 1)

    def test_enclosure_position_does_not_matter(self):
        findings = self.check(placed(ENC % "GN403020", 300, 200),
                              placed(FP % "din_4m", 350, 250))
        self.assertEqual(findings, [])

    def test_too_deep_is_an_error(self):
        catalog = copy.deepcopy(self.catalog)
        catalog["_by_stem"]["ide_fp_din_4m"][1]["depth"] = 180
        findings = el.check_diagram(catalog, "test", [
            placed(ENC % "GN403020", 0, 0), placed(FP % "din_4m", 50, 50)])
        self.assertEqual(len(messages(findings, "error")), 1)
        self.assertIn("deep", messages(findings)[0])

    def test_overlap_is_an_error_but_touching_is_not(self):
        touching = self.check(placed(ENC % "GN403020", 0, 0),
                              placed(FP % "din_4m", 50, 50),
                              placed(FP % "din_4m", 85, 50))
        overlapping = self.check(placed(ENC % "GN403020", 0, 0),
                                 placed(FP % "din_4m", 50, 50),
                                 placed(FP % "din_4m", 80, 50))
        self.assertEqual(touching, [])
        self.assertEqual(len(messages(overlapping, "error")), 1)

    def test_footprint_outside_every_enclosure_is_a_warning(self):
        findings = self.check(placed(ENC % "GN403020", 0, 0),
                              placed(FP % "din_4m", 500, 500))
        self.assertEqual([f.severity for f in findings], ["warning"])

    def test_unknown_usable_space_checks_the_outer_size(self):
        catalog = copy.deepcopy(self.catalog)
        item = catalog["_by_stem"]["ide_enc_GNG303020"][1]
        item["usable"] = item["plate"] = None
        findings = el.check_diagram(catalog, "test", [
            placed(ENC % "GNG303020", 0, 0),
            placed(FP % "din_4m", 5, 50),
            placed(FP % "din_4m", 125, 50)])
        self.assertEqual(len(messages(findings, "warning")), 1)
        self.assertEqual(len(messages(findings, "error")), 1)

    def test_other_elements_are_ignored(self):
        findings = self.check(
            placed("embed://import/10_electric/relay.elmt", 0, 0))
        self.assertEqual(findings, [])


def part(label="", reference="", location="", prefix="", maker="",
         type_="embed://import/10_electric/relay.elmt"):
    """A symbol of the schema with its element information."""
    node = ET.Element("element", {"type": type_, "x": "0", "y": "0",
                                  "orientation": "0", "prefix": prefix})
    infos = ET.SubElement(node, "elementInformations")
    for name, value in (("label", label), ("manufacturer_reference",
                        reference), ("location", location),
                        ("manufacturer", maker)):
        ET.SubElement(infos, "elementInformation",
                      {"name": name, "show": "1"}).text = value
    return node


def enclosure(reference, label):
    node = placed(ENC % reference, 0, 0)
    infos = ET.SubElement(node, "elementInformations")
    ET.SubElement(infos, "elementInformation",
                  {"name": "label"}).text = label
    return node


class CabinetTest(unittest.TestCase):
    def setUp(self):
        self.catalog = el.load_catalog()
        self.catalog["_devices"] = [
            {"key": "k1", "manufacturer": "", "reference": "K1",
             "width": 45, "height": 90, "depth": 60},
            {"key": "ref-big", "manufacturer": "acme", "reference": "REF-BIG",
             "width": 200, "height": 200, "depth": 100},
            {"key": "deep", "manufacturer": "", "reference": "DEEP",
             "width": 10, "height": 10, "depth": 190}]

    def analyse(self, *folios):
        return el.analyse_project(self.catalog, list(folios))

    def test_folio_location_is_inherited_and_can_be_overridden(self):
        _, cabinets = self.analyse(
            ("Power", "A1", [part("-Q1", "K1"),
                             part("-S1", "K1", location="A2")]))
        self.assertEqual(sorted(cabinets), ["A1", "A2"])
        self.assertEqual(cabinets["A1"]["items"][0]["quantity"], 1)
        self.assertEqual(cabinets["A2"]["items"][0]["quantity"], 1)

    def test_contents_group_by_reference_and_count_a_label_once(self):
        _, cabinets = self.analyse(
            ("Power", "A1", [part("-Q1", "K1"), part("-Q2", "K1"),
                             part("-Q2", "K1"), part("-Q3")]))
        items = cabinets["A1"]["items"]
        self.assertEqual([(i["reference"], i["quantity"]) for i in items],
                         [("K1", 2)])
        self.assertEqual(cabinets["A1"]["unreferenced"], 1)

    def test_components_without_cabinet_are_not_listed(self):
        _, cabinets = self.analyse(("Power", "", [part("-Q1", "K1")]))
        self.assertEqual(cabinets, {})

    def test_reference_is_linked_to_its_footprint(self):
        found = el.find_device(self.catalog, "", " k1 ")
        self.assertEqual((found["width"], found["height"]), (45, 90))
        self.assertIsNone(el.find_device(self.catalog, "", "unknown"))

    def test_device_manufacturer_must_match_when_given(self):
        self.assertIsNotNone(el.find_device(self.catalog, "ACME", "ref-big"))
        self.assertIsNone(el.find_device(self.catalog, "Other", "REF-BIG"))

    def test_devices_that_fit_give_no_warning(self):
        findings, cabinets = self.analyse(
            ("Layout", "", [enclosure("GN403020", "A1")]),
            ("Power", "A1", [part("-Q1", "K1")]))
        self.assertEqual(findings, [])
        self.assertEqual(cabinets["A1"]["occupied"], 45 * 90)

    def test_cabinet_without_enclosure_is_a_note(self):
        findings, _ = self.analyse(("Power", "A1", [part("-Q1", "K1")]))
        self.assertEqual([f.severity for f in findings], ["info"])

    def test_unlinked_reference_is_a_note(self):
        findings, _ = self.analyse(
            ("Layout", "", [enclosure("GN403020", "A1")]),
            ("Power", "A1", [part("-Q1", "NOPE")]))
        self.assertEqual([f.severity for f in findings], ["info"])
        self.assertIn("NOPE", findings[0].message)

    def test_total_area_over_the_usable_area_is_an_error(self):
        # 250 x 350 mm usable: 87500 mm2; one K1 takes 4050 mm2
        parts = [part("-Q%d" % n, "K1") for n in range(22)]
        findings, _ = self.analyse(
            ("Layout", "", [enclosure("GN403020", "A1")]),
            ("Power", "A1", parts))
        self.assertEqual(messages(findings, "error"),
                         ["A1: the devices take 102% of the usable area of "
                          "GN403020"])

    def test_crowded_cabinet_is_a_warning(self):
        parts = [part("-Q%d" % n, "K1") for n in range(19)]
        findings, _ = self.analyse(
            ("Layout", "", [enclosure("GN403020", "A1")]),
            ("Power", "A1", parts))
        self.assertEqual([f.severity for f in findings], ["warning"])
        self.assertIn("88%", findings[0].message)

    def test_deep_device_is_an_error_and_a_big_one_that_fits_is_not(self):
        findings, _ = self.analyse(
            ("Layout", "", [enclosure("GN403020", "A1")]),
            ("Power", "A1", [part("-Q1", "REF-BIG", maker="Acme"),
                             part("-Q2", "DEEP")]))
        errors = messages(findings, "error")
        self.assertEqual(len(errors), 1)
        self.assertIn("deep", errors[0])
        self.assertEqual(messages(findings, "warning"), [])

    def test_oversized_device_is_an_error(self):
        findings, _ = self.analyse(
            ("Layout", "", [enclosure("GN403020", "A1")]),
            ("Power", "A1", [part("-Q1", "REF-BIG", maker="Acme")]))
        self.assertEqual(len(messages(findings, "error")), 0)
        self.catalog["_devices"][1]["width"] = 400
        findings, _ = self.analyse(
            ("Layout", "", [enclosure("GN403020", "A1")]),
            ("Power", "A1", [part("-Q1", "REF-BIG", maker="Acme")]))
        self.assertIn("does not fit", messages(findings, "error")[0])

    def test_same_component_in_two_cabinets_is_a_warning(self):
        findings, _ = self.analyse(
            ("Power", "A1", [part("-K1", "K1")]),
            ("Field", "A2", [part("-K1", "K1")]))
        dup = [f for f in findings if "several cabinets" in f.message]
        self.assertEqual([f.severity for f in dup], ["warning"])
        self.assertIn("A1 (folio 'Power'), A2 (folio 'Field')",
                      dup[0].message)

    def test_explicit_cabinet_counts_as_well(self):
        findings, _ = self.analyse(
            ("Power", "A1", [part("-K1", "K1")]),
            ("Power 2", "A1", [part("-K1", "K1", location="A2")]))
        self.assertEqual(len(messages(findings, "warning")), 1)

    def test_terminals_in_two_cabinets_are_only_a_note(self):
        findings, _ = self.analyse(
            ("Power", "A1", [part("-X1", prefix="X")]),
            ("Field", "A2", [part("-X1", prefix="X")]))
        dup = [f for f in findings if "several cabinets" in f.message]
        self.assertEqual([f.severity for f in dup], ["info"])

    def test_report_lists_each_cabinet(self):
        _, cabinets = self.analyse(
            ("Layout", "", [enclosure("GN403020", "A1")]),
            ("Power", "A1", [part("-Q1", "K1"), part("-Q2", "K1")]))
        text = el.render_report(cabinets)
        self.assertIn("Cabinet A1", text)
        self.assertIn("2 x", text)
        self.assertIn("K1 (45x90x60 mm)", text)
        self.assertEqual(el.report_rows(cabinets)[1],
                         ["A1", "GN403020", "", "K1", "2", "45", "90", "60"])


class LinkTest(unittest.TestCase):
    def test_link_writes_the_devices_file_and_replaces_an_entry(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "devices.json"
            catalog = el.load_catalog(devices_path=path)
            el.link_device(catalog, "KM1", footprint="din_3m")
            el.link_device(catalog, "KM1", footprint="din_4m")
            el.link_device(catalog, "ABC", manufacturer="Acme",
                           size={"width": 10, "height": 20, "depth": 30})
            catalog = el.load_catalog(devices_path=path)
            self.assertEqual(
                [(d["reference"], d["width"]) for d in catalog["_devices"]],
                [("ABC", 10), ("KM1", 70)])

    def test_unknown_footprint_is_refused(self):
        with tempfile.TemporaryDirectory() as tmp:
            catalog = el.load_catalog(devices_path=Path(tmp) / "d.json")
            with self.assertRaises(ValueError):
                el.link_device(catalog, "KM1", footprint="nope")


DOOR = "embed://import/ide-enclosures/doors/ide_door_%s.elmt"
PLATE = "embed://import/ide-enclosures/plates/ide_plate_%s.elmt"
CUSTOM = "embed://import/ide-enclosures/custom/ide_fp_rail_ts35_7_5_L%d.elmt"


class PartTest(unittest.TestCase):
    def setUp(self):
        self.catalog = el.load_catalog()

    def check(self, *elements):
        return el.check_diagram(self.catalog, "test", list(elements))

    def test_door_and_plate_elements_carry_their_references(self):
        files = el.render_all(self.catalog)
        door = ET.fromstring(files["doors/ide_door_GN403020.elmt"])
        plate = ET.fromstring(files["plates/ide_plate_GN403020.elmt"])
        refs = []
        for root in (door, plate):
            infos = {i.get("name"): i.text for i in root.iter(
                "elementInformation")}
            refs.append(infos["manufacturer_reference"])
        self.assertEqual(refs, ["GN403020", "PL4030"])
        outer = plate.find("description/rect")
        # 370 x 250 mm at 1 px = 2 mm
        self.assertEqual((float(outer.get("width")),
                          float(outer.get("height"))), (125, 185))

    def test_parts_on_their_enclosure_pass(self):
        findings = self.check(placed(ENC % "GN403020", 0, 0),
                              placed(DOOR % "GN403020", 0, 0),
                              placed(PLATE % "GN403020", 12, 7))
        self.assertEqual(findings, [])

    def test_part_of_another_enclosure_is_a_warning(self):
        findings = self.check(placed(ENC % "GN605020", 0, 0),
                              placed(PLATE % "GN403020", 10, 10))
        self.assertEqual([f.severity for f in findings], ["warning"])
        self.assertIn("belongs to GN403020", findings[0].message)

    def test_part_leaving_the_enclosure_is_an_error(self):
        findings = self.check(placed(ENC % "GN403020", 0, 0),
                              placed(PLATE % "GN403020", 40, 10))
        self.assertEqual(len(messages(findings, "error")), 1)
        self.assertIn("leaves the outline", messages(findings)[0])

    def test_part_outside_every_enclosure_is_a_warning(self):
        findings = self.check(placed(ENC % "GN403020", 0, 0),
                              placed(DOOR % "GN403020", 500, 500))
        self.assertEqual([f.severity for f in findings], ["warning"])

    def test_parts_are_listed_in_their_cabinet_and_checked(self):
        door = placed(DOOR % "GN605020", 0, 0)
        infos = ET.SubElement(door, "elementInformations")
        ET.SubElement(infos, "elementInformation",
                      {"name": "label"}).text = "A1"
        findings, cabinets = el.analyse_project(
            self.catalog, [("Layout", "", [enclosure("GN403020", "A1"),
                                           door])])
        self.assertEqual([p[0] for p in cabinets["A1"]["parts"]], ["door"])
        self.assertIn("belongs to GN605020", " ".join(
            messages(findings, "warning")))
        self.assertIn("door GN605020", el.render_report(cabinets))


class LengthTest(unittest.TestCase):
    def setUp(self):
        self.catalog = el.load_catalog()

    def test_stem_with_a_length_stands_for_the_stretched_footprint(self):
        kind, item = el.lookup(self.catalog, "ide_fp_duct_efapel_14040_L1200")
        self.assertEqual((kind, item["width"], item["height"]),
                         ("footprint", 1200, 60))
        self.assertIsNone(el.lookup(self.catalog, "ide_fp_din_4m_L100"))
        self.assertIsNone(el.lookup(self.catalog, "ide_fp_nope_L100"))

    def test_length_is_drawn_at_the_catalog_scale(self):
        with tempfile.TemporaryDirectory() as tmp:
            with contextlib.redirect_stdout(io.StringIO()):
                code = el.main(["length", "rail_ts35_7_5", "500", "750",
                                "--out", tmp])
            self.assertEqual(code, 0)
            root = ET.parse(Path(tmp) / "ide_fp_rail_ts35_7_5_L750.elmt"
                            ).getroot()
            rect = root.find("description/rect")
            self.assertEqual((float(rect.get("width")),
                              float(rect.get("height"))), (375, 17.5))
            self.assertTrue((Path(tmp) / "qet_directory").exists())

    def test_fixed_or_unknown_footprint_is_refused(self):
        with tempfile.TemporaryDirectory() as tmp:
            for footprint in ("din_4m", "nope"):
                with self.assertRaises(ValueError):
                    el.write_lengths(self.catalog, footprint, [100], tmp)
            with self.assertRaises(ValueError):
                el.write_lengths(self.catalog, "rail_ts35_15", [0], tmp)

    def test_custom_length_is_checked_like_any_footprint(self):
        # 30 px = 60 mm from the left: 200 mm fits in the 250 mm width of
        # GN403020 but 300 mm does not
        def check(length):
            return el.check_diagram(self.catalog, "test", [
                placed(ENC % "GN403020", 0, 0),
                placed(CUSTOM % length, 30, 50)])
        self.assertEqual(check(200), [])
        self.assertEqual(len(messages(check(300), "error")), 1)


PROJECT = """<project version="0.100.0" title="t">
    <diagram title="Layout">
        <elements>
            <element type="%s" x="0" y="0" orientation="0"/>
            <element type="%s" x="10" y="50" orientation="0"/>
        </elements>
    </diagram>
    <diagram title="Wiring">
        <elements>
            <element type="embed://import/10_electric/relay.elmt"
                     x="0" y="0" orientation="0"/>
        </elements>
    </diagram>
</project>
"""


class ProjectTest(unittest.TestCase):
    def run_main(self, project_text):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "project.qet"
            path.write_text(project_text, encoding="utf-8")
            out = io.StringIO()
            with contextlib.redirect_stdout(out):
                code = el.main(["check", str(path)])
            return code, out.getvalue()

    def test_failing_project_exits_with_one(self):
        code, out = self.run_main(PROJECT % (ENC % "GN403020",
                                             FP % "din_4m"))
        self.assertEqual(code, 1)
        self.assertIn("folio 'Layout'", out)
        self.assertNotIn("Wiring", out)

    def test_project_without_problem_exits_with_zero(self):
        code, out = self.run_main(
            (PROJECT % (ENC % "GN403020", FP % "din_4m"))
            .replace('x="10"', 'x="50"'))
        self.assertEqual(code, 0)
        self.assertIn("0 error(s)", out)


if __name__ == "__main__":
    unittest.main()
