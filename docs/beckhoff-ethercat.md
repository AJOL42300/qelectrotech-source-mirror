# Beckhoff EtherCAT islands

A small set of symbols and tools for drawing a Beckhoff EtherCAT
architecture: a coupler with a stack of I/O terminals, EtherCAT boxes in a
daisy chain, and AF1000 drives. The symbols are in
`extra-elements/15_beckhoff_ethercat` and are installed into the common
collection, as the category **Beckhoff EtherCAT (generic)**.

## What is there

| Symbol | Manufacturer | Designation | Terminals |
| --- | --- | --- | --- |
| `el1xxx_generic.elmt` | Beckhoff | EL1xxx | contacts 1 to 8 |
| `el2xxx_generic.elmt` | Beckhoff | EL2xxx | contacts 1 to 8 |
| `el3xxx_generic.elmt` | Beckhoff | EL3xxx | contacts 1 to 8 |
| `el4xxx_generic.elmt` | Beckhoff | EL4xxx | contacts 1 to 8 |
| `el1008.elmt` | Beckhoff | EL1008 | contacts 1 to 8 |
| `el2008.elmt` | Beckhoff | EL2008 | contacts 1 to 8 |
| `el2652.elmt` | Beckhoff | EL2652 | contacts 1 to 4 (relay output, generic) |
| `el5152.elmt` | Beckhoff | EL5152 | contacts 1 to 8 (generic layout) |
| `*_realistic.elmt` | as the symbol | as the symbol | terminals on the circles of the drawing (EL1008, EL2008, EL5152, EK1914) |
| `*_table.elmt` | as the symbol | as the symbol | none: the I/O table only (EL1008, EL2008, EL2652, EL5152, EK1914, EPP2338) |
| `ek1100.elmt` | Beckhoff | EK1100 | EtherCAT IN, EtherCAT OUT |
| `ek1322.elmt` | Beckhoff | EK1322 | IN X50, OUT X51, OUT X52 |
| `ek1914.elmt` | Beckhoff | EK1914 | EtherCAT IN/OUT, I1 to I4, SI1, SI2, O1 to O4, SO1, SO2 |
| `ax5203.elmt` | Beckhoff | AX5203 | EtherCAT IN/OUT, L1, L2, L3, PE, M1-U/V/W/PE, M2-U/V/W/PE |
| `ax5206.elmt` | Beckhoff | AX5206 | same layout as the AX5203 |
| `el9011.elmt` | Beckhoff | EL9011 | none (end cap) |
| `epp2338.elmt` | Beckhoff | EPP2338-0001 | IN X50, OUT X51, X01 to X04 |
| `ethercat_box_generic.elmt` | (empty) | (empty) | EtherCAT IN, EtherCAT OUT |
| `af1000_2axes_generic.elmt` | Beckhoff | AF1000 | EtherCAT IN/OUT, A1-U/V/W, A2-U/V/W, L1, L2, L3, PE, RB+, RB- |

Other terminals are already in the standard collection, under
`10_electric/20_manufacturers_articles/beckhoff`.

The cards, the EPP2338 and the EK1914 are *PLC master* elements with an
**I/O table** (one entry per channel). Everything on the table is generic and
can be edited in the element properties.

### References of a typical schematic

The references found in the sample schematic and what covers them:

| Reference | Symbol |
| --- | --- |
| EL1008, EL2008, EL2652 | their own symbol |
| EK1100, EK1322, EK1914 | their own symbol |
| EPP2338 | `epp2338.elmt` (the `-0002` variant is not told apart: the difference is not known, edit the reference) |
| AX5203, AX5206 | their own symbol |
| AF1103, AF1107, AF1115, AF1130, AF1155, AF1215, AF1222 | `af1000_2axes_generic.elmt`: set the reference and the number of axes yourself |
| AX5805, KL219 | none |

The designations are the ones of the Beckhoff product names; the EL2652
relay output and all the layouts are generic.

## What is generic

Only the EPP2338 reference and designation come from a real project; nothing
else is copied from a datasheet.

- EL1xxx to EL4xxx are drawn the same way for every type: eight numbered
  contacts. Rename the terminals and set the exact designation (for example
  EL3004) after the datasheet of the terminal you use.
- The EPP2338 ports (X01 to X04, two channels each) are listed as I/O of
  type "universal"; the actual use of each channel is yours to define.
- The AF1000 symbol puts its terminals where the connectors are on the front
  of a two-axis drive, as a layout aid, not measured. The exact type
  (AF1215...), its power and its current are left for the user.
- The box has no manufacturer and no reference.

## Island numbering

An island is a coupler or a box (the **head**, `A1.00`) with its **cards**
(`A1.10`, `A1.20`...). The prefix `A1` is the island number: the next island
is `A2`. The cards are numbered in steps of 10, so that a card can be added
between two others without renumbering:

```
A1.00     the head of island A1 (coupler or box)
A1.10     first card
A1.20     second card
A1.15     card inserted between .10 and .20
A2.00     the head of the next island
```

Use **Numbering > Number the cards of an island...**. Select the cards on the
folio, give the island label (`A1`; the existing islands are proposed), choose
where to place them (end, beginning, after card N) and the reading order,
then validate. When there is room, only the new cards are numbered (`.15` is
the middle of `.10` and `.20`). Otherwise, or on request, all the cards of
the island are renumbered by steps of 10. The whole operation is one undo
step. Only the direct cards of the island are counted: `A1.10.5`
belongs to `A1.10`. A head written `A1` (without `.00`) is still recognised.

### Parent, card, appearances

A card can be drawn up to three times, as three elements: **symbol**
(`el1008.elmt`), **realistic** view (`el1008_realistic.elmt`) and **I/O
table** (`el1008_table.elmt`). The link between them is the label, not a
reference to another object:

```
island A1
 ├─ A1.00  head (ek1100)             symbol
 ├─ A1.10  card (el1008)             symbol  +  realistic  +  I/O table
 ├─ A1.20  card (el2008)             symbol  +  realistic  +  I/O table
 └─ A1.30  card (el5152)             symbol  +  realistic  +  I/O table
```

- parent to card: the card label starts with the label of the head
  (`A1.10` belongs to `A1`), with a step of 10 inherited from the island;
- card to appearances: the three elements have the same label (`A1.10`), the
  same manufacturer, reference and designation, and the same I/O table, so
  that the used pins and the tooltip see them as one card.

**Numbering > Create the other appearances...** (also in the context menu of
a card) adds the missing appearances of the selected card, on the current
folio (placed to its right), on another folio, or on a new folio, in one
undo step, with the label, the information and the I/O table copied. It
needs the symbols of the three appearances to be side by side in the same
collection folder (`X.elmt`, `X_realistic.elmt`, `X_table.elmt`), as the
Beckhoff ones are. Renumbering and duplicating work on the label, so they
apply to every appearance at once.

The nomenclature of the project counts each appearance, so a card drawn three
times is counted three times by the core bill of materials. The I/O lists
of the island and `qet_plc_io` list a card once.

### Realistic appearances and the grid

The realistic views are made from the user's own drawings of the "Entreprise"
collection with `misc/beckhoff/realistic_from_model.py`: each terminal is
placed on the centre of the circle it belongs to, and the drawing is scaled
and shifted so that these centres fall on the grid of QElectroTech. The
manufacturer drawings and texts are not part of the repository.

| Element | Scale | Offset (px) | Grid | Largest remaining gap |
| --- | --- | --- | --- | --- |
| `el1008_realistic` | 1.00 | 4.5, 0 | 10 px | 0.5 px |
| `el2008_realistic` | 1.00 | 4.5, 0 | 10 px | 0.5 px |
| `el5152_realistic` | 1.13 | 1.8, 5.5 | 10 px | 1.2 px |
| `ek1914_realistic` | 1.09 | 2.5, 3.8 | 5 px | 1.5 px |

A single scale cannot bring all the circles of a drawing to the grid: the
gap above is what is left, and the terminals are placed on the grid nodes.
The EK1914 spacing only allows a 5 px grid. The grid used is written in the
`informations` of each element and checked by the tests. The hotspot is the
origin of the drawing after the offset.

## Folios generated for an island

- **Lists > Add a folio listing the cards of an island...** adds a folio after
  the current one, with a table of the cards of the island (label,
  designation, manufacturer, reference) ordered by their number.
- **Lists > Add a folio listing the inputs and outputs of an island...** adds
  a folio with one line per channel of the island and of its cards (card,
  channel, address, type, function, comment, folio), ordered by card number.
  The card label, with the folio, is the link to the card.

Both are ordinary tables: they are refreshed when the project changes
(adding or renumbering a card), and a table too long for a folio continues
on a new folio. Adding the folio is one undo step.

### Input and output lists, as in a typical schematic

For a line like `X01(1) C.PC3 ( Cellule presence produits crochet 3 )`,
fill the entry of the I/O table of the card as follows:

- **address**: the connector and channel, `X01(1)`;
- **function**: the tag of the equipment, `C.PC3`;
- **comment**: the designation, `Cellule presence produits crochet 3`.

The bill of materials of the project (folio of references, quantity and
total) is the existing nomenclature: every card with a manufacturer and a
reference appears in it, no extra step.

## Duplicating an island

Select the head of an island (or one of its cards) and use
**Numbering > Duplicate the island...** or the context menu. One undo step
copies the head and all its cards, in the same relative layout.

- **New copy**: an independent island. The head takes the next free
  identifier (`A1` gives `A2`, editable; any text ending with a number works,
  `BOX 03` gives `BOX 04`) and the cards take it by steps of 10 in the same
  order (`A2.00` for the head, `A2.10`, `A2.20`...). A label already used by
  an island or a card is refused. The formula, comment and location are
  emptied, and so are the function, comment and cross reference of the
  channels, unless *Keep the function and the comment of the channels* is
  ticked. The channel addresses and types stay: they describe the module.
- **Linked representation**: the same island shown again on another folio,
  with the same labels and the same information, so that the used pins are
  greyed and the tooltip lists the channels of both folios.
- The folio of the copy is chosen in the dialog. If the island is on
  several folios, each part is copied side by side on the chosen folio.
- *Keep the wires between the cards of the island* copies the conductors
  which join two copied elements. Conductors leaving the island are never
  copied.

## Wiring tooltip, used and free pins

Both are settings of the general configuration, off by default.

- **Wiring tooltip**: hovering an element or a terminal shows, per terminal,
  its name, signal, I/O address, the wires connected (with their folio) and
  its internal links. An internal link is the `potential` attribute of the
  terminals (terminals with the same value are linked inside the module: 24 V,
  0 V, PE, feed-through). The I/O entries of the table belong to the terminal
  whose name is their address (`X01(1)` belongs to `X01`).
- **Grey used pins**: a terminal whose pin is wired on another placement of the
  same element (same label, on any folio) is drawn in light grey, and the
  tooltip adds the used channels (with their folios) and the free ones.
  Several placements of one element are made by giving them the same label;
  the *Linked representation* above does it.

The tooltip is plain text and cannot be clicked: to jump to another folio,
use the cross references of the folios (double click).

## Importing a drawing as a symbol

The element editor imports a SVG file (**Import a SVG drawing**), and the
command line converts one with `--svg-to-elmt`. See
[svg_to_elmt.md](svg_to_elmt.md). Terminals are placed on the drawing
afterwards (the editor moves them with the mouse). The editor also imports a
PNG or JPEG image (vectorised) and pastes an image of the clipboard with
Ctrl+V, with the same scale and threshold choices.

## Using the symbols

- Daisy chain: a connection between two boxes is a single molded cable
  (reference and length), not a wire per conductor: use one conductor
  between the EtherCAT OUT of a box and the IN of the next one.
- Showing a picture of the real product: the symbol editor has no picture
  part, so a raster photo cannot be a symbol. Convert a vector drawing (SVG)
  instead, or add the picture to the folio with the image tool and place the
  symbol over it. Only use pictures you may use.

### Fetching a product picture on your machine

No Beckhoff picture is in the repository, and none must be added to it.
`--svg-to-elmt` also accepts one address (https) instead of a file: the
address of a product page, or of a picture, that you copied from your browser:

```
qelectrotech --svg-to-elmt https://www.beckhoff.com/en-en/products/i-o/ethercat-terminals/el-ed1xxx-digital-input/el1008.html el1008_picture.elmt --cells 10
```

It downloads that one address when you run the command (no search, no loop
over references; a product page gives its preview picture), keeps the picture
in the cache folder of the user, traces it, and writes the element where you
say. Write it outside the repository, for example in your personal element
collection: a warning is printed when the output is in a git repository.

I did not find clear terms of use for the product pictures of the Beckhoff
website, so whether you may use a picture is up to you. A photograph traces
poorly: this works for line drawings and logos.

## Example project

`examples/beckhoff_islands_example.qet` shows island `A1`: its head `A1.00`
(EK1100) and the cards `A1.10` (EL1008), `A1.20` (EL2008) and `A1.30`
(EL5152). Folio 1 has the symbols, with two sensors wired on pins 1 and 2 of
`A1.10` and a lamp on pin 1 of `A1.20` (these pins show as used); folio 2 the
realistic views; folio 3 the I/O tables, where the used channels carry their
function. All the generic Beckhoff elements are embedded in the project. It is
written by `python misc/beckhoff/gen_example.py` (the uuids are fixed, so the
file is always the same). Open it with the QElectroTech built from this
branch (`build\qelectrotech.exe examples\beckhoff_islands_example.qet`).
Checked: `--info`, `--resave` (a second save is identical to the first) and
`--export-png`. The three appearances are tied by their label; there is no
cross reference between them, and the head has no I/O table.

## Limits

- The ios of the EK1914 are named `I1`..`SO2` while the terminals of its
  realistic view are numbered `1`..`16`: the tooltip and the grey pins do not
  match them there.
- The designation of the EL5152 is generic and was not checked against a
  datasheet.
- No table is added automatically on the nomenclature folio: choose the folio
  in *Create the other appearances*.
- The drives (AF1000, AX5203, AX5206), the AX5805 and the EPP2338-0002 have no
  realistic view yet.
- The realistic views were checked by drawing them with their terminals
  (every terminal is inside its circle), and by the tests; the interface
  (menus, dialogs, tooltip) was not tried by hand.

- No E-bus or power contact terminals are drawn between the I/O terminals.
- A SVG conversion only keeps the colours of the symbol palette, no text,
  clipping or filter, and does not detect the terminals.
- The I/O folios use the "island" table: it is not editable with the
  nomenclature query editor.
- Duplicating an island does not copy the slaves of its cards which are on
  other folios, nor any conductor leaving the island.
