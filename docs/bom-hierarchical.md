# Hierarchical bill of materials

The hierarchical list shows each device, then its accessories indented
below it. It is an option of the CSV export (*Hierarchical list*) and of the
folio table (*Edit the query*). It is off by default: nothing changes for
existing projects, exports or tables.

## Which elements belong to a device

1. **Auxiliary articles** of an element (the blocks 1 to 4 of the articles
   tab) are listed under its main article, one level deeper.
2. **Accessory elements**: an element whose label is the label of a device,
   followed by `.`, `-` or `_` and something more.

| Device | Accessories |
| --- | --- |
| `-QS1` | `-QS1.1`, `-QS1-MC`, `-QS1_OF` |
| `-QS1.1` | `-QS1.1.2` (the longest matching label is the parent) |

Rules:

- Labels are compared as typed: case counts, and `-QS10` is no accessory of
  `-QS1` (the next character must be a separator).
- The parent is the **longest** other label that fits.
- A device with no accessory is a plain line at level 0.
- An accessory with no parent on the list (`-QS2.1` but no `-QS2`) stays at
  level 0 and its `orphan` column is 1. The export warns about it once the
  file is saved.
- Lines are never merged: two identical parts stay on two lines. Use the
  grouped list to add parts up.
- Order inside a level: the order of the project (folio, then position).

## Columns

| Column | Meaning |
| --- | --- |
| `level` | 0 for a device, 1 for its accessories and auxiliary articles, and so on |
| `label` | label of the element |
| `quantity` | quantity of the line itself |
| `total_quantity` | quantity of the line multiplied by those of its parents |
| `name` | description or designation, indented by two spaces per level |
| `part_number`, `manufacturer`, `unit` | as in the grouped list |
| `orphan` | 1 for an accessory without a parent |

`total_quantity` of an auxiliary article is its quantity multiplied by the
quantity of the main article of the same element and by those of the parents.
A blank quantity counts as 1.

## Quantity 0

- In the hierarchical list, a line with quantity 0 stays visible (an
  accessory only listed as compatible) and its `total_quantity` is 0.
- In the grouped list (`BomExport::groupedQuery`), quantity 0 is now left out
  of the sum **and** of the count (`GroupedOptions::skip_zero_quantity`, true
  by default). Before, with the *count* mode, a quantity of 0 counted for 1.
  Only a text made of zeros (`0`, `0,0`) is a zero: a blank quantity is 1 and
  a non-numeric text is not touched.

## More than four auxiliary articles

The four blocks (`BomExport::AuxiliaryBlocks`) are hard-coded in the project
database view (`element_nomenclature_view`), the articles tab and the
information keys. Changing that means changing the saved database and the
.qet keys, so it was not done.

Instead, as many accessories as wanted can be added as **elements** whose
label starts with the device label (`-QS1.5`, `-QS1.6`...): the hierarchical
list has no limit on them. An element dedicated to an accessory can be an
element with only the articles tab filled in.

## Example project

`examples/nomenclature_example.qet` is a small cabinet that shows each rule.
Open it, then export the parts list (hierarchical option) or look at the
folio "Hierarchical bill of materials". `docs/bom-hierarchical-example.csv` is
what the hierarchical export gives for it (same format as the program:
fields quoted, `;` between them).

| Label | What it shows |
|-------|---------------|
| `-QS1`, `-QS1.1` | a switch disconnector and its accessory at quantity 0 (listed, total 0) |
| `-QF1` | a circuit breaker with two auxiliary articles stored in the element (one at quantity 0) |
| `-QF1.1`, `-QF1-VG`, `-QF1_AL`, `-QF1.2` | accessories attached by label prefix with the three separators: with the two auxiliary articles, `-QF1` has seven articles in all, more than the main article plus four auxiliary blocks |
| `-QF9.1` | deliberate orphan: no `-QF9` exists, so the line is flagged "No parent" |
| `-KM1`, `-KM1.1` | a contactor with one auxiliary block and one accessory element |
| `-GS1` | a power supply, no accessory |
| `-A1` and `-A1.1` to `-A1.9` | a Beckhoff coupler with its terminals as indented lines |

The folios hold the symbols only, with no wiring: the example is about the
parts list. A last folio holds the flat grouped list, to compare: the
quantity 0 articles are not counted there and `-QF1.2`, `-QF9.1` and the
auxiliary article of `-QF1` add up as one line of 3 contact blocks.

The symbols come from the collections already in the repository: the
definitions are embedded in the project, so it opens without the elements
folder. The part numbers are real Schneider Electric and Beckhoff references,
with a short generic designation of our own; no manufacturer text or picture
is copied. Which accessory fits which device is not asserted by the file
(for instance, the accessories are those of the iC60 range, used here on a
switch disconnector only to show the list): check a reference against the
manufacturer's catalogue before ordering.

The file was written by a script, not saved from the program. The query
results above were checked by running the SQL of `hierarchicalQuery()` on the
element data of the file. Opening the project in the program has not been
tested yet.

## Where it lives

- `BomExport::hierarchicalQuery()`: the SQL, one `WITH RECURSIVE` query over
  `element_nomenclature_view`, so that the export and the folio table (which
  stores a SQL string) share it. `HierarchicalOptions` switches the
  auxiliary articles and `total_quantity` off.
- `BomExport::isHierarchicalQuery()` and `hierarchicalHeader()`: detect such a
  query and translate its column names.
- Tests: `tests/qttest/tst_groupedbom.cpp`.
- Example: `examples/nomenclature_example.qet` and
  `docs/bom-hierarchical-example.csv`.

## Compatibility

- Old projects open and look the same. No new XML attribute is written for
  the export.
- A folio table set to the hierarchical list stores its SQL in the project
  file. An older QElectroTech has no option for it and cannot edit it with
  its fields (not tested).
- The Schneider work in progress stores its compatible accessories as
  auxiliary articles with quantity 0 on the device itself: they are listed one
  level under the device. No label-prefix convention was found there; this
  rule is the one to share with it.
