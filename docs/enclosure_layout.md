# Enclosure layout (IDE enclosures and cabinets)

QElectroTech has no enclosure or mounting-panel layout feature. This adds a
minimal one, built only from existing mechanisms:

- a catalog of IDE enclosures, as editable data
  (`extra-elements/ide-enclosures/catalog.json`);
- front-view elements generated from it, drawn to scale: five enclosures and
  twelve generic footprints (DIN-rail modular devices and TS35 rails);
- a command-line check that reads a project and reports footprints that do
  not fit.

Nothing in the application changes, and existing projects are not affected.

## Install

Copy `extra-elements/ide-enclosures` into your element collection (the
`elements` folder of the QElectroTech user data folder), then restart
QElectroTech. The elements appear under "IDE enclosures and cabinets".

## Scale

One pixel of the folio is 2 mm (`scale_mm_per_px` in the catalog). A
GN403020 (400 x 300 mm) is 200 x 150 px. A 2000 mm cabinet is 1000 px high,
which is taller than the default folio (800 px): enlarge the folio rows, or
use a smaller enclosure.

## Workflow

1. On a folio, place an enclosure element. Its front view shows the outer
   size and a dashed rectangle: the area where devices can be fitted.
2. Place footprint elements inside the dashed rectangle. A modular device is
   17.5 mm per module wide. Quarter turns are supported.
3. Check the project (the file is only read, never modified):

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
   | warning  | a footprint is not inside any enclosure |
   | warning  | footprints but no enclosure on the folio |
   | warning  | the catalog has no usable size for the enclosure: the outer size is used |

A footprint belongs to the enclosure that contains its centre.

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
- `enclosures`: `reference`, `family`, `mounting`, `height`, `width`,
  `depth` (mm), `usable` (`height`, `width`, `depth`, or `null`), `plate`
  (`height`, `width`, or `null`), `notes`;
- `footprints`: `id`, names, `width`, `height`, `depth` (mm).

The fit area is the plate when its size is known, otherwise the usable size.
It is assumed centred in the front view.

To add an enclosure, or a real device (a contactor with its datasheet
dimensions, for instance), edit `catalog.json` and run:

```sh
python3 misc/enclosure_layout/enclosure_layout.py generate
```

The unit test fails when the generated `.elmt` files are out of date. Check
the result with `qelectrotech --check-elements`.

## Data

Only facts are recorded: reference and dimensions. No image, text or PDF of
the manufacturer is included. The values were taken from the public IDE
product pages (through search result excerpts, as the site was not
reachable): **check them against the current datasheets before using them
for a real mounting**. The reference encodes the size in centimetres
(GN403020: height 40, width 30, depth 20).

## Limits

- Not integrated in the application: the check is a separate script
  (Python 3.9 or later).
- Front view only. The depth check compares each device with the usable
  depth; it does not model what sits behind or in front of the plate.
- Plate sizes are unknown: the usable size is used instead. GNG303020 and
  GNG604020 have no usable size recorded.
- Generic modular footprints use nominal values (17.5 mm per module, 90 mm
  high, 60 mm deep), not those of a particular device.
- No ducts, DIN-rail snapping, or automatic placement.
- The generated elements were checked by the unit tests (well-formed, on the
  grid), not opened in QElectroTech.
