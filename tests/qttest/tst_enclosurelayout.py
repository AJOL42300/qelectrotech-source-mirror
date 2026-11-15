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

    def test_generated_files_are_up_to_date(self):
        files = el.render_all(self.catalog)
        for name, text in files.items():
            on_disk = ((el.DEFAULT_DIR / name).read_bytes().decode("utf-8")
                       .replace("\r\n", "\n"))
            self.assertEqual(on_disk, text, name + " is stale: run generate")
        for sub in ("enclosures", "footprints"):
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

    def test_plate_wins_over_usable_space(self):
        enc = {"height": 400, "width": 300, "depth": 200,
               "usable": {"height": 350, "width": 250, "depth": 179},
               "plate": {"height": 300, "width": 200}}
        self.assertEqual(el.fit_area(enc), (50, 50, 250, 350, 179))

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
        findings = self.check(placed(ENC % "GNG303020", 0, 0),
                              placed(FP % "din_4m", 5, 50),
                              placed(FP % "din_4m", 125, 50))
        self.assertEqual(len(messages(findings, "warning")), 1)
        self.assertEqual(len(messages(findings, "error")), 1)

    def test_other_elements_are_ignored(self):
        findings = self.check(
            placed("embed://import/10_electric/relay.elmt", 0, 0))
        self.assertEqual(findings, [])


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
