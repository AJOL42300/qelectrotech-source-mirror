# Enclosure layout (IDE enclosures and cabinets)

QElectroTech has no enclosure or mounting-panel layout feature. This adds a
minimal one, built only from existing mechanisms:

- a catalog of IDE enclosures, as editable data
  (`extra-elements/ide-enclosures/catalog.json`);
- front-view elements generated from it, drawn to scale: five enclosures,
  their doors and mounting plates, and generic footprints (DIN-rail
  modular devices, TS35 rails, Efapel wiring ducts);
- a command-line check that reads a project and reports parts that do not
  fit.

Nothing in the application changes, and existing projects are not affected.

## Install

Copy `extra-elements/ide-enclosures` into your element collection (the
`elements` folder of the QElectroTech user data folder), then restart
QElectroTech. The elements appear under "IDE enclosures and cabinets":
enclosures, doors, mounting plates and footprints.

## Scale

One pixel of the folio is 2 mm (`scale_mm_per_px` in the catalog). A
GN403020 (400 x 300 mm) is 200 x 150 px. A 2000 mm cabinet is 1000 px high,
which is taller than the default folio (800 px): enlarge the folio rows, or
use a smaller enclosure.

## Workflow

1. On a folio, place an enclosure element. Its front view shows the outer
   size and a dashed rectangle: the area where devices can be fitted.
2. Optionally place its door and its mounting plate (same front view, same
   scale) on the enclosure. Both are named after the enclosure
   (`ide_door_GN403020`, `ide_plate_GN403020`): the enclosure is their
   parent, and the cross-reference is its reference.
3. Place footprint elements inside the dashed rectangle. A modular device is
   17.5 mm per module wide. Quarter turns are supported.
4. Check the project (the file is only read, never modified):

   ```sh
   python3 misc/enclosure_layout/enclosure_layout.py check project.qet
   ```

   Folios without these elements are ignored. The exit code is 1 when at
   least one error is reported. Reported:

   | Severity | Meaning |
   |----------|---------|
   | error    | a footprint leaves the fit area |
   | error    | a footprint is deeper than the enclosure allows |
   | error    | two footprints overlap (touching is accepted) |
   | error    | a door or a plate leaves the outline of its enclosure |
   | warning  | a footprint, a door or a plate is not inside any enclosure |
   | warning  | a door or a plate is placed on an enclosure it does not belong to |
   | warning  | footprints but no enclosure on the folio |
   | warning  | the catalog has no usable size for the enclosure: the outer size is used |

A footprint, a door or a plate belongs to the enclosure that contains its
centre.

### Doors and mounting plates

Each enclosure of the catalog has one door and one mounting plate element.
The plate carries the manufacturer reference of the plate (`PL4030` for a
GN403020) and the plate size of its datasheet; the door carries the
reference of the enclosure, because the manufacturer sells no separate
door for these references. The door is drawn at the outer front size
(the sheets publish the door thickness and the opaque, reversible door, but
no door size). In the project, a door or a plate whose label (or location)
names a cabinet is listed in the `report` of that cabinet, and `check`
warns when its enclosure differs from the cabinet's.

### Rails and ducts at a chosen length

QElectroTech cannot stretch an element: its drawing is fixed. Ducts and
rails are therefore delivered at a default length (2000 mm, 1000 px wide),
and any other length is generated on demand:

```sh
python3 misc/enclosure_layout/enclosure_layout.py length duct_efapel_14040 300 450
```

writes `ide_fp_duct_efapel_14040_L300.elmt` and `..._L450.elmt` in the
`custom` folder next to the catalog (`--out DIR` to change it). Copy them
into your element collection. `check` and `report` recognise a name ending
in `_L` and a length, so a custom length is checked like any footprint.
`generate` never touches the `custom` folder.

Ducts are drawn in top view as a rectangle of length x width (the duct
height is the depth for the check). The footprints are the TS35 rails (35 x
7.5 and 35 x 15, EN 60715) and ten Efapel ETS 14 slotted panel ducts
(heights 40 and 60 mm, widths 25 to 100 mm, grey PVC "CCZ" references; the
halogen-free variant has the suffix "HCZ").

## Cabinets in the schema

Components are assigned to a cabinet with fields QElectroTech already has;
nothing is added to the project format.

- **Folio**: the "Localisation" field of the folio (`locmach` in the
  project file) names the cabinet the folio belongs to.
- **Component**: its own "location" information, when filled, overrides the
  cabinet of the folio (a field sensor drawn on a cabinet folio, for
  instance).
- **Enclosure**: an enclosure element stands for the cabinet named like its
  label (its "label" information; failing that its "location", then the
  localisation of its folio).

A cabinet name with no enclosure element is still listed, but its space is
not checked. Components with neither a label nor a manufacturer reference
are ignored; those without a reference are only counted.

### Link between a symbol and its footprint

A symbol finds its footprint through its **manufacturer reference**, so
nothing is entered twice. `extra-elements/ide-enclosures/devices.json` maps
references to a footprint of the catalog, or to dimensions in mm:

```json
{"devices": [
	{"reference": "KM1-ABC", "manufacturer": "Acme", "footprint": "din_3m"},
	{"reference": "XYZ-100", "width": 70, "height": 90, "depth": 62}
]}
```

References are compared without regard to case or extra spaces. The
`manufacturer` entry is optional: when given, it must match the
"manufacturer" information of the symbol. Add or replace an entry with:

```sh
python3 misc/enclosure_layout/enclosure_layout.py link KM1-ABC --footprint din_3m
python3 misc/enclosure_layout/enclosure_layout.py link XYZ-100 --width 70 --height 90 --depth 62
```

`--devices file.json` uses another file. The shipped file is empty: only
references you have checked against a datasheet should be added.

### Contents and space of each cabinet

```sh
python3 misc/enclosure_layout/enclosure_layout.py report project.qet [--csv contents.csv]
```

lists, for each cabinet, its enclosure, its devices grouped by reference
with the quantity and the footprint, and the share of the usable area used.
A component whose label appears several times in a cabinet counts once.
`check` also reports, per cabinet with an enclosure element:

| Severity | Meaning |
|----------|---------|
| error    | a device does not fit in the usable area (a quarter turn is allowed) |
| error    | a device is deeper than the enclosure allows |
| error    | the sum of the device footprints exceeds the usable area |
| warning  | the sum exceeds 80% of the usable area (ducts and spacing are not counted) |
| note     | a reference has no linked footprint and is not counted |
| note     | a cabinet has no enclosure element |

### Component in two cabinets

`check` warns when the same label is found in two cabinets, whether
through its folios or through its "location" information. Terminals (a
`X` prefix or a terminal symbol) are expected to be spread over several
cabinets and only give a note. Slave symbols (contacts, cross references)
carry no label of their own and are not reported. Cables and conductors
between cabinets are not components and are not reported.

### Limits

- The space check is the sum of areas, not a placement: it cannot tell
  whether the devices can be arranged on the rails. Use the placed
  footprints of the Workflow section for that.
- No folio listing the contents is generated: use `report --csv` and import
  the result, or the material list of QElectroTech.
- The checks are not in the application interface.
- The terminal prefixes (`X`) and the 80% limit can be changed with
  `shared_prefixes` and `fill_warning` in `catalog.json`.

## Catalog

`catalog.json` holds:

- `scale_mm_per_px`;
- `enclosures`: `reference`, `designation`, `family`, `mounting`,
  `material`, `ip`, `height`, `width`, `depth` and `thickness` (mm),
  `usable` (`height`, `width`, `depth`, or `null`), `plate` (`reference`,
  `height`, `width`, `thickness`, `material`, or `null`), `door` (`type`,
  `count`, `reversible`, `thickness`, `height`, `width`, `size_published`),
  `source` (product page), `datasheet`, `verified` (date), `notes`;
- `footprints`: `id`, `kind`, names, `width`, `height`, `depth` (mm); for
  ducts and rails also `stretch` (the width is the default length),
  `manufacturer`, `reference`, `source`, `verified`.

The fit area is the plate or the usable size, whichever is known; when both
are known, the smaller one on each side (the plate of the GN cabinets is
taller than their usable space). It is assumed centred in the front view.

To add an enclosure, or a real device (a contactor with its datasheet
dimensions, for instance), edit `catalog.json` and run:

```sh
python3 misc/enclosure_layout/enclosure_layout.py generate
```

The unit test fails when the generated `.elmt` files are out of date. Check
the result with `qelectrotech --check-elements`.

## Data

Only facts are recorded: reference and dimensions. No image, text or PDF of
the manufacturer is included. The IDE values were checked on 2026-11-18
against the product page and the datasheet of each reference on
`ideelectric.com` (URLs in the `source` and `datasheet` fields); the plate
sizes come from the datasheets of the plates PL4030, PL6050, PL3030, PL6040
and PL20080. The Efapel ducts come from the technical sheet of the ETS 14
slotted panel ducts (`efapel.pt`). Check them against the current
datasheets before using them for a real mounting. The IDE reference encodes
the size in centimetres (GN403020: height 40, width 30, depth 20).

Inconsistencies found: the datasheet of PL6050 prints "450 x 570" under
"H x W" (read as 570 high, 450 wide, as the cabinet datasheet implies); the
plates of the GN cabinets are 20 mm taller than the usable space of the
cabinet (370 against 350 mm for GN403020).

## Limits

- Not integrated in the application: the check is a separate script
  (Python 3.9 or later).
- Front view only. The depth check compares each device with the usable
  depth; it does not model what sits behind or in front of the plate.
- The door size is not published: it is assumed to be the outer front size.
  The opening side is reversible, and is not drawn.
- The default length of the rails (2000 mm) and their dimensions come from
  EN 60715, not from a manufacturer sheet.
- Generic modular footprints use nominal values (17.5 mm per module, 90 mm
  high, 60 mm deep), not those of a particular device.
- No DIN-rail snapping, or automatic placement.
- Stretching is not a feature of QElectroTech (see below).
- The generated elements were checked by the unit tests (well-formed, on the
  grid), not opened in QElectroTech.

### Proposal: a length property in the application

An element is a cached drawing with fixed terminals and texts, and the only
transformation the application offers is the mirror. The smallest change
that would let a duct or a rail follow a length typed by the user is a
numeric "length" property on the element (saved as an optional attribute,
so that old projects and old versions load unchanged), with the drawing
rebuilt from it and a numeric field in the element properties dialog. It is
not done here: it touches the element cache and the undo commands, and it
needs a C++ build and its own review.
