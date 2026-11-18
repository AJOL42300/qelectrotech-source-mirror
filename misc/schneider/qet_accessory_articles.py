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
"""Give the accessories of a Schneider device to it, as articles.

An accessory whose label is the label of a device followed by a separator
and something (-QS1.1, -QS1-OF, -QS1_MC for the device -QS1) is linked to
that device.  The device carries, in the auxiliary article blocks 1 to 4 of
its Articles tab, its compatible accessories at quantity 0; the quantity of
the block of an accessory is the number of accessories linked to it.

Rules
  * The parent is found by cutting the label of the accessory at its
    separators (. - _ , never the first character), longest prefix first,
    and taking the first one that is the label of an element.  Every
    element carrying the parent label (the symbol and the front view of
    it, on several folios) gets the same blocks, so the synchronisation of
    the fields of elements with the same label has nothing to arbitrate.
  * Only the accessories of the compatibility lists of gen_schneider.py
    (public data of the manufacturer) can be linked.  A reference outside
    the list of the parent is reported and NOT written ("outside the
    list"); with --strict it is an error and nothing is written.  A parent
    that has no list is the same case ("no list").
  * An accessory with no parent is a warning.
  * A block whose reference belongs to the list of its device is managed
    by this tool: it is rewritten at every run (idempotent: no duplicate,
    quantity back to 0 when the accessory is deleted or renamed).  Any
    other block of the device is left as it is, and takes one of the four
    places.
  * A group of exclusive accessories (the two motor mechanisms) takes one
    block: the one that is linked, else the first of the list.
  * --prune-zero also removes the blocks left at quantity 0, to export a
    parts list: the parts list of QElectroTech does not leave a quantity 0
    out (see docs/schneider.md).

Nothing is written without -o or --in-place.

Usage: qet_accessory_articles.py project.qet [-o out.qet | --in-place]
                                 [--strict] [--prune-zero]
Exit status: 0 done, 1 unreadable file, 2 strict errors, 3 changes needed
(--check).
"""
import argparse
import os
import sys
import xml.etree.ElementTree as ET

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_schneider as gs  # noqa: E402

SEPARATORS = ".-_"
FIELDS = ("description", "designation", "manufacturer",
          "manufacturer_reference", "quantity")
BLOCKS = range(1, gs.MAX_BLOCKS + 1)
MARK = "Compatible accessory"
LINKABLE = {"A9V11325"}   # a device that is also an accessory of another


def key(field, n):
    return "%s_auxiliary%d" % (field, n)


class Result:
    def __init__(self):
        self.warnings = []
        self.errors = []
        self.changes = 0


def infos(el):
    node = el.find("elementInformations")
    if node is None:
        return None, {}
    return node, {i.get("name"): (i.text or "")
                  for i in node.findall("elementInformation")}


def set_info(el, name, value):
    node = el.find("elementInformations")
    if node is None:
        node = ET.Element("elementInformations")
        at = 0
        for i, child in enumerate(el):
            if child.tag in ("terminals", "inputs"):
                at = i + 1
        el.insert(at, node)
    for item in node.findall("elementInformation"):
        if item.get("name") == name:
            if (item.text or "") == value:
                return False
            item.text = value
            return True
    item = ET.SubElement(node, "elementInformation",
                         {"name": name, "show": "0"})
    item.text = value
    return True


def del_info(el, name):
    node = el.find("elementInformations")
    changed = False
    if node is not None:
        for item in node.findall("elementInformation"):
            if item.get("name") == name:
                node.remove(item)
                changed = True
    return changed


def stem_of(path):
    base = os.path.basename((path or "").replace("\\", "/"))
    base = os.path.splitext(base)[0]
    for suffix in ("_realistic", "_accessories"):
        if base.endswith(suffix):
            base = base[:-len(suffix)]
    return base


def reference(el, values, products_by_stem):
    ref = values.get("manufacturer_reference", "").strip()
    if ref:
        return ref
    p = products_by_stem.get(stem_of(el.get("type")))
    return p.reference if p else ""


def parent_prefixes(label):
    """Candidate parent labels, longest first."""
    out = []
    for i in range(len(label) - 1, 0, -1):
        if label[i] in SEPARATORS:
            out.append(label[:i])
    return out


def process(root, strict=False, prune_zero=False):
    res = Result()
    by_ref = gs.product_by_reference()
    by_stem = {p.stem: p for p in gs.PRODUCTS}
    accessory_refs = {p.reference for p in gs.PRODUCTS
                      if p.folder in gs.ACCESSORY_FOLDERS} | LINKABLE
    elements = []
    for diagram in root.iter("diagram"):
        title = diagram.get("title", "")
        for el in diagram.iter("element"):
            _, values = infos(el)
            label = values.get("label", "").strip()
            if label:
                elements.append((el, values, label, title))
    labels = {}
    for el, values, label, _ in elements:
        labels.setdefault(label, []).append(el)

    # accessories linked to each parent label: label -> ref -> {labels}
    linked = {}
    for el, values, label, title in elements:
        ref = reference(el, values, by_stem)
        if ref not in accessory_refs:
            continue
        parent = next((c for c in parent_prefixes(label) if c in labels),
                      None)
        if parent is None:
            if by_ref.get(ref) and by_ref[ref].folder in gs.ACCESSORY_FOLDERS:
                res.warnings.append(
                    "orphan accessory %s (%s) on %s: no device whose "
                    "label is a prefix" % (label, ref, title))
            continue
        parent_ref = ""
        for pe in labels[parent]:
            _, pv = infos(pe)
            parent_ref = reference(pe, pv, by_stem) or parent_ref
        product = by_ref.get(parent_ref)
        allowed = gs.COMPAT.get(product.stem, []) if product else []
        if ref not in allowed:
            msg = ("%s (%s) is not in the compatibility list of %s (%s)"
                   % (label, ref, parent, parent_ref or "no reference")
                   if allowed else
                   "%s (%s): %s (%s) has no compatibility list"
                   % (label, ref, parent, parent_ref or "no reference"))
            (res.errors if strict else res.warnings).append(
                ("outside the list: " if allowed else "no list: ") + msg)
            continue
        linked.setdefault(parent, {}).setdefault(ref, set()).add(label)

    for parent, instances in labels.items():
        for el in instances:
            _, values = infos(el)
            product = by_ref.get(reference(el, values, by_stem))
            if product is None or product.stem not in gs.COMPAT:
                continue
            res.changes += apply_blocks(
                el, values, gs.COMPAT[product.stem],
                linked.get(parent, {}), by_ref, prune_zero, res, parent)
    return res


def apply_blocks(el, values, allowed, linked, by_ref, prune_zero, res,
                 parent):
    managed_refs = set(allowed)
    old = {n: values.get(key("manufacturer_reference", n), "").strip()
           for n in BLOCKS}
    def used(n):
        return any(values.get(key(f, n), "").strip() for f in FIELDS) or \
            values.get("auxiliary%d" % n, "").strip() != ""

    keep = [n for n in BLOCKS
            if used(n) and not (old[n] in managed_refs or (
                old[n] == "" and values.get("auxiliary%d" % n) == MARK))]
    free = [n for n in BLOCKS if n not in keep]

    wanted = []     # (ref, quantity), in the order of the list
    linked_groups = {gs.group(r) for r in linked}
    for ref in allowed:
        if ref in linked:
            wanted.append((ref, len(linked[ref])))
        elif (not prune_zero and gs.group(ref) == ref
              and ref not in linked_groups):
            wanted.append((ref, 0))
    while len(wanted) > len(free):           # quantity 0 goes first
        zero = [w for w in wanted if w[1] == 0]
        drop = zero[-1] if zero else wanted[-1]
        wanted.remove(drop)
        if drop[1]:
            res.warnings.append(
                "%s: no free article block for %s" % (parent, drop[0]))

    changes = 0
    for n in BLOCKS:
        if n in keep:
            continue
        if wanted:
            ref, qty = wanted.pop(0)
            acc = by_ref[ref]
            data = {"auxiliary%d" % n: MARK,
                    key("description", n): acc.description,
                    key("designation", n): acc.designation,
                    key("manufacturer", n): gs.MANUFACTURER,
                    key("manufacturer_reference", n): acc.reference,
                    key("quantity", n): str(qty)}
            for name, value in data.items():
                changes += set_info(el, name, value)
        else:
            for name in ["auxiliary%d" % n] + [key(f, n) for f in FIELDS]:
                changes += del_info(el, name)
    return changes


def indent_new(root):
    """Give the new nodes a line of their own."""
    for node in root.iter("elementInformations"):
        items = list(node)
        for i, item in enumerate(items):
            if not item.tail or not item.tail.strip() == "":
                item.tail = "\n" + "\t" * 6 if i < len(items) - 1 \
                    else "\n" + "\t" * 5


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("project")
    ap.add_argument("-o", "--output")
    ap.add_argument("--in-place", action="store_true")
    ap.add_argument("--check", action="store_true",
                    help="write nothing, exit 3 if the file would change")
    ap.add_argument("--strict", action="store_true",
                    help="an accessory outside the list is an error")
    ap.add_argument("--prune-zero", action="store_true",
                    help="also remove the blocks left at quantity 0")
    args = ap.parse_args(argv)
    try:
        tree = ET.parse(args.project)
    except (OSError, ET.ParseError) as e:
        print("cannot read %s: %s" % (args.project, e), file=sys.stderr)
        return 1
    res = process(tree.getroot(), args.strict, args.prune_zero)
    for w in res.warnings:
        print("WARNING", w)
    for e in res.errors:
        print("ERROR", e)
    print("%d field(s) to change" % res.changes)
    if res.errors:
        return 2
    if args.check:
        return 3 if res.changes else 0
    target = args.project if args.in_place else args.output
    if target and (res.changes or target != args.project):
        indent_new(tree.getroot())
        tree.write(target, encoding="utf-8", xml_declaration=False)
        print("wrote", target)
    return 0


if __name__ == "__main__":
    sys.exit(main())
