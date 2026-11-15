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
| `el9011.elmt` | Beckhoff | EL9011 | none (end cap) |
| `epp2338.elmt` | Beckhoff | EPP2338-0001 | IN X50, OUT X51, X01 to X04 |
| `ethercat_box_generic.elmt` | (empty) | (empty) | EtherCAT IN, EtherCAT OUT |
| `af1000_2axes_generic.elmt` | Beckhoff | AF1000 | EtherCAT IN/OUT, A1-U/V/W, A2-U/V/W, L1, L2, L3, PE, RB+, RB- |

The EK1100 coupler, and EL1008, EL2008, EL2564 and other terminals, are
already in the standard collection, under
`10_electric/20_manufacturers_articles/beckhoff`.

The EL1xxx to EL4xxx cards and the EPP2338 are *PLC master* elements with an
**I/O table** (one entry per channel). Everything on the table is generic and
can be edited in the element properties.

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

An island is a coupler or a box with its cards. The cards take the label of
the island followed by a number, in steps of 10, so that a card can be added
between two others without renumbering:

```
BOX 03        the island (coupler or box)
BOX 03.10     first card
BOX 03.20     second card
BOX 03.15     card inserted between .10 and .20
```

Use **Numbering > Number the cards of an island...**. Select the cards on the
folio, give the island label (the existing islands are proposed), choose where
to place them (end, beginning, after card N) and the reading order, then
validate. When there is room, only the new cards are numbered (`.15` is
the middle of `.10` and `.20`). Otherwise, or on request, all the cards of
the island are renumbered by steps of 10. The whole operation is one undo
step. Only the direct cards of the island are counted: `BOX 03.10.5`
belongs to `BOX 03.10`.

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

- **New copy**: an independent island. The head takes the next free label
  (`BOX 03` gives `BOX 04`, editable) and the cards take it by steps of 10 in
  the same order (`BOX 04.10`, `BOX 04.20`...). A label already used by an
  island or a card is refused. The formula, comment and location are
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

## Limits

- No E-bus or power contact terminals are drawn between the I/O terminals.
- A SVG conversion only keeps the colours of the symbol palette, no text,
  clipping or filter, and does not detect the terminals.
- The I/O folios use the "island" table: it is not editable with the
  nomenclature query editor.
- Duplicating an island does not copy the slaves of its cards which are on
  other folios, nor any conductor leaving the island.
