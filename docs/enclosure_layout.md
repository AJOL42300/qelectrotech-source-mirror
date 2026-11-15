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
