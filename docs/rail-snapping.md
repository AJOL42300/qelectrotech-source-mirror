# Rail snapping (magnétisme sur rail)

In a layout (implantation) folio, a symbol that declares itself a
**component of a rail** is put on the axis of the nearest **rail** and
against its nearest neighbour, 2 mm away, when it is placed or moved.
Nothing is hard-wired to a manufacturer: everything comes from a
`<railSnap>` element written in the symbol (`.elmt`).

Out of scope: snapping to the edges of a cabinet, mounting plate or duct,
and collision handling beyond "do not overlap another neighbour".

## Behaviour

- **Across the rail**: the *fixing line* of the component is put on the axis
  of the rail. Components of different heights on the same rail therefore
  share the same rail line. For a vertical rail the same rule applies in
  horizontal.
- **Along the rail**: the component is stuck to its nearest left or right
  neighbour (a component whose fixing line is on the same rail), leaving the
  gap of the component (`gap_mm`, 2 mm by default). A component without a
  neighbour within reach is only put on the rail.
- **Reach**: nothing happens when the rail (or the neighbour) is farther than
  the threshold. Default 10 mm, *Settings > General > Rail snapping reach*.
  The threshold is converted with the scale of the component.
- **Switch**: *Settings > General > Snap components to mounting rails and
  their neighbours* (off by default). Holding **Alt** or **Shift** while
  moving or placing suspends the snapping for that move.
- **Undo**: the snap changes only the position that the move would have had
  anyway, so one move is one undo step, as before.
- **Several selected symbols**: only the dragged symbol (the pilot) is
  computed. The others follow by the same vector, so relative positions are
  kept. Selected symbols are never used as neighbours or rails.
- **Feedback**: a thin red cross marks the fixing point and a guide line
  shows the rail and the neighbour while moving. They are drawn on screen
  only; they are never printed nor exported.
- A symbol turned by anything but a multiple of 90° is left alone.
- A symbol without `<railSnap>` is never touched.

## Data format

`<railSnap>` is a child of `<definition>`. Old versions of QElectroTech
ignore it, old symbols do not have it, and the `.qet` format is unchanged.
Lengths of the body are in symbol pixels, in the coordinates of the symbol
(the origin is the hotspot).

```xml
<railSnap role="component" axis="horizontal"
          x="0" y="0" width="17.5" height="45"
          mount="center" gap_mm="2" mm_per_px="2"
          basis="datasheet" source="Fabricant ref. 123, p. 4"
          verified="2026-05-01"/>
```

| Attribute | Applies to | Meaning | Default |
|---|---|---|---|
| `role` | both | `rail` or `component` (required) | - |
| `axis` | both | direction of the rail: `horizontal` or `vertical` | `horizontal` |
| `x`, `y`, `width`, `height` | both | body of the symbol (its footprint) | required |
| `mount` | component | which line is the fixing line: `origin`, `start`, `center`, `end` | `origin` |
| `mount_offset_mm` | component | shift of the fixing line, in mm, positive towards the end | 0 |
| `gap_mm` | component | spacing to the neighbours | 2 |
| `mm_per_px` | both | scale: millimetres of one symbol pixel (IDE: 2) | 1 |
| `basis` | both | where the fixing position comes from: `datasheet`, `image`, `standard`, `assumed` | `assumed` |
| `source` | both | reference (data sheet, page, image URL) | empty |
| `verified` | both | date when someone checked the position | empty |

For a rail, `y` (or `x`) of the body is the rail line: its fixing line is the
middle of the body, i.e. the axis of the profile.

### Origin as fixing point (recommended)

The simplest case: put the **origin (hotspot) of the symbol on the fixing
point** and write `mount="origin"` (or nothing). The position of the symbol
in the folio then *is* the fixing point on the rail axis; no offset is
needed.

`start`, `center`, `end` with `mount_offset_mm` exist for symbols whose
origin cannot be on the fixing point (for example the IDE footprints, whose
origin is the top-left corner of the body: they use `mount="center"` and keep
their geometry unchanged).

### Where the fixing position comes from

The position of the fixing line must be taken **per component** from the data
sheet (fixing dimensions, height of the DIN clip, position relative to the
35 mm rail) or from public images, not from a generic value:

- `basis="datasheet"` or `"image"` with a `source`, and `verified` set once a
  person has checked it;
- `basis="standard"` for dimensions fixed by a standard (a TS35 rail);
- `basis="assumed"` (the default) when nobody checked.

The check of the generators (`enclosure_layout.py check`) gives a
**warning** for every component whose position is `assumed`, or which comes
from a data sheet/image without a `verified` date.

## Generators and catalog

`misc/enclosure_layout/enclosure_layout.py` writes `<railSnap>` in the
footprints: `role="rail"` for TS35 rails (length follows the stretched rail,
`basis="standard"`), `role="component"` for DIN modules. A value per
footprint can be given in `extra-elements/ide-enclosures/catalog.json`:

```json
"rail_snap": {
  "mount": "center", "mount_offset_mm": 0, "gap_mm": 2,
  "basis": "datasheet", "source": "ref. 123 p. 4", "verified": "2026-05-01"
}
```

Ducts are not rails nor components. See `docs/enclosure_layout.md`.

## Adoption by other libraries

Add one `<railSnap>` to the `<definition>` of each rail and each component,
with the scale of the library in `mm_per_px`, the real dimensions in the
body, and the position of the fixing line with its `basis`/`source`. No code
change is needed; the symbols of the Beckhoff and Mean Well libraries can
adopt it independently of this branch.

## Uncertain points

- The red cross is drawn at run time from the data, on screen only, and not
  stored as a shape in the symbol.
- No folio type "layout" exists in QElectroTech: the snapping applies to any
  folio holding symbols that declare `<railSnap>`.
- Collisions: only "do not overlap another neighbour of the same rail".
