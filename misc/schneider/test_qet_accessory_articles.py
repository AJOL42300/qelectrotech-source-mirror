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
"""Tests of qet_accessory_articles.py."""
import os
import sys
import unittest
import xml.etree.ElementTree as ET

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import qet_accessory_articles as tool  # noqa: E402


def element(label, ref="", extra=()):
    infos = '<elementInformation name="label">%s</elementInformation>' % label
    if ref:
        infos += ('<elementInformation name="manufacturer_reference">%s'
                  "</elementInformation>" % ref)
    for name, value in extra:
        infos += ('<elementInformation name="%s">%s</elementInformation>'
                  % (name, value))
    return ("<element><elementInformations>%s</elementInformations>"
            "</element>" % infos)


def project(*elements):
    return ET.fromstring("<project><diagram title=\"f\"><elements>%s"
                         "</elements></diagram></project>"
                         % "".join(elements))


def blocks(root, label):
    """ref -> quantity of the blocks of the first element with the label"""
    for el in root.iter("element"):
        _, v = tool.infos(el)
        if v.get("label") == label:
            return {v[tool.key("manufacturer_reference", n)]:
                    v[tool.key("quantity", n)] for n in tool.BLOCKS
                    if tool.key("manufacturer_reference", n) in v}
    raise KeyError(label)


NSX = "C25N3TM250C"


class AccessoryArticles(unittest.TestCase):
    def test_seed_and_link(self):
        root = project(element("-QS1", NSX), element("-QS1.1", "LV431541"),
                       element("-QS1-OF", "29450"))
        res = tool.process(root)
        self.assertEqual(res.warnings, [])
        self.assertEqual(blocks(root, "-QS1"),
                         {"LV431541": "1", "29450": "1", "LV429410": "0",
                          "LV429517": "0"})

    def test_idempotent_and_rename(self):
        root = project(element("-QS1", NSX), element("-QS1.1", "LV431541"))
        tool.process(root)
        self.assertEqual(tool.process(root).changes, 0)
        for el in root.iter("element"):
            _, v = tool.infos(el)
            if v["label"] == "-QS1.1":
                tool.set_info(el, "label", "-QS9.1")
        res = tool.process(root)
        self.assertEqual(blocks(root, "-QS1")["LV431541"], "0")
        self.assertTrue(any("orphan" in w for w in res.warnings))

    def test_outside_the_list(self):
        root = project(element("-QS1", NSX), element("-QS1.1", "29450"),
                       element("-QS1.2", "A9A26924"))
        res = tool.process(root)
        self.assertEqual(len(res.warnings), 1)
        self.assertIn("outside the list", res.warnings[0])
        self.assertEqual(blocks(root, "-QS1")["29450"], "1")
        self.assertNotIn("A9A26924", blocks(root, "-QS1"))
        self.assertTrue(tool.process(root, strict=True).errors)

    def test_quantity_counts_and_group(self):
        root = project(element("-QS1", NSX), element("-QS1.1", "29450"),
                       element("-QS1.2", "29450"),
                       element("-QS1-MC", "LV431543"))
        tool.process(root)
        got = blocks(root, "-QS1")
        self.assertEqual(got["29450"], "2")
        self.assertEqual(got["LV431543"], "1")
        self.assertNotIn("LV431541", got)

    def test_manual_block_kept_and_prune(self):
        extra = [("auxiliary1", "Spare"), ("manufacturer_reference_"
                                           "auxiliary1", "X-1"),
                 ("quantity_auxiliary1", "3")]
        root = project(element("-QS1", NSX, extra))
        tool.process(root, prune_zero=True)
        self.assertEqual(blocks(root, "-QS1"), {"X-1": "3"})

    def test_same_label_on_two_folios(self):
        root = project(element("-QS1", NSX), element("-QS1", NSX),
                       element("-QS1.1", "29450"))
        tool.process(root)
        for el in root.iter("element"):
            _, v = tool.infos(el)
            if v["label"] == "-QS1":
                self.assertEqual(v[tool.key("quantity", 2)], "1")

    def test_no_list(self):
        root = project(element("-QF1", "OTHER"),
                       element("-QF1.1", "29450"))
        res = tool.process(root)
        self.assertIn("no list", res.warnings[0])


    def test_contactor_blocks(self):
        root = project(element("-KM1", "LC1D09P7"),
                       element("-KM1.1", "LADN22"),
                       element("-KM1-2", "LADN11"),
                       element("-KM1.3", "LV431541"))
        res = tool.process(root)
        self.assertEqual(blocks(root, "-KM1"),
                         {"LADN22": "1", "LADN11": "1", "LADN20": "0",
                          "LADN02": "0"})
        self.assertEqual(len(res.warnings), 1)
        self.assertIn("outside the list", res.warnings[0])


if __name__ == "__main__":
    unittest.main()
