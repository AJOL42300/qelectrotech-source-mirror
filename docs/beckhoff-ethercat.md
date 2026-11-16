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
| `el1004.elmt`, `el2004.elmt` | Beckhoff | EL1004, EL2004 | pins 1 to 8 (channels on 1, 5, 3, 7; 0 V on 2, 6, 4, 8) |
| `el2522.elmt`, `el2574.elmt` | Beckhoff | EL2522, EL2574 | pins 1 to 8 (channels as printed on the drawing) |
| `el2911.elmt`, `el9100.elmt` | Beckhoff | EL2911, EL9100 | 16 pins (doubled drawing) / 8 pins |
| `el2652.elmt` | Beckhoff | EL2652 | contacts 1 to 4 (relay output, generic) |
| `el5152.elmt` | Beckhoff | EL5152 | contacts 1 to 8 (generic layout) |
| `*_realistic.elmt` | as the symbol | as the symbol | terminals on the screw circles of the drawing (EL1004, EL1008, EL1904, EL1918, EL2004, EL2008, EL2522, EL2574, EL2904, EL2911, EL5152, EL9100, EK1914) |
| `*_generic.elmt` | as the symbol | as the symbol | generic vector view drawn here (drives, EL6910, EK1960 and the yellow cards EL1904, EL1918, EL2904) |
| `*_table.elmt` | as the symbol | as the symbol | none: the I/O table only (EL1004, EL1008, EL2004, EL2008, EL2522, EL2574, EL2652, EL2911, EL5152, EK1914, EPP2338) |
| `ek1100.elmt` | Beckhoff | EK1100 | EtherCAT IN, EtherCAT OUT |
| `ek1322.elmt` | Beckhoff | EK1322 | IN X50, OUT X51, OUT X52 |
| `ek1914.elmt` | Beckhoff | EK1914 | EtherCAT IN/OUT, I1 to I4, SI1, SI2, O1 to O4, SO1, SO2 |
| `el9011.elmt` | Beckhoff | EL9011 | none (end cap) |
| `epp2338.elmt` | Beckhoff | EPP2338-0001 | IN X50, OUT X51, X01 to X04 |
| `ethercat_box_generic.elmt` | (empty) | (empty) | EtherCAT IN, EtherCAT OUT |
| `drives_af1000/af11xx.elmt`, `af12xx.elmt` | Beckhoff | AF1103, AF1107, AF1115, AF1130, AF1155 (1 axis); AF1215, AF1222 (2 axes) | see [Drives](#drives-af1000-and-ax5000) |
| `drives_ax5000/ax51xx.elmt`, `ax52xx.elmt` | Beckhoff | AX5101 to AX5125 (1 axis); AX5201 to AX5225 (2 axes) | see [Drives](#drives-af1000-and-ax5000) |
| `twinsafe/*.elmt` | Beckhoff | EL1904, EL1918, EL2904, EL6910, EK1960 | yellow safety cards, see [Safety cards](#safety-cards-twinsafe-yellow) |

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
| AX5203, AX5206 | `drives_ax5000/ax5203.elmt`, `ax5206.elmt` (2 axes) |
| AF1103, AF1107, AF1115, AF1130, AF1155 | `drives_af1000/`, 1 axis |
| AF1215, AF1222 | `drives_af1000/`, 2 axes |
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
- The drive symbols put their terminals where the connectors are on the
  pictures of the sample schematic (see below), not measured on a product.
  The exact power, current and mains type are to be checked by the user.
- The box has no manufacturer and no reference.

## Pin and channel are two things

On a Beckhoff card the number of the **pin** (the terminal point of the
housing) is not always the number of the **channel**. The EL1008 has its
channels I1 to I8 on pins 1, 5, 2, 6, 3, 7, 4, 8: the pins 1 to 4 are on one
side, 5 to 8 on the other. So the terminals of an element are named with the
Beckhoff pin, and each channel of the I/O table lists its pin(s) in the entry
(`address` = signal, `terminal` = pin; the file keeps `<plcIO ...>
<terminal>5</terminal></plcIO>`). Nothing is renumbered by channel.

| Card | Pin to channel | Source |
| --- | --- | --- |
| EL1008 | I1 1, I2 5, I3 2, I4 6, I5 3, I6 7, I7 4, I8 8 | the user's drawing |
| EL2008 | O1 1, O2 5, O3 2, O4 6, O5 3, O6 7, O7 4, O8 8 | the user's drawing |
| EL5152 | pins 1 A1, 2 +, 3 -, 4 A2, 5 B1, 6 +, 7 -, 8 B2; 2 and 6 are 24 V, 3 and 7 are 0 V | the user's drawing, the supply is inferred from the labels |
| EL1004, EL2004, EL1904, EL2904 | channels 1 to 4 on pins 1, 5, 3, 7; 0 V on 2, 6, 4, 8 | the user's drawing (the yellow cards print S1..S4; the I/O table calls them SI1..SI4 / SO1..SO4) |
| EL1918 | SI1..SI8 on pins 1, 5, 2, 6, 3, 7, 4, 8 | the user's drawing: **8 terminals** (the earlier 16-pin assumption was wrong) |
| other cards | pin = channel | generic |

The tooltip writes `5 → I2`, the I/O table has an optional **Terminal**
column (in the table properties, hidden by default; the `_table` elements show
it), and the greyed pins use the pin. When the pin-out is unknown (EK1914:
the drawing names no channel, so the pin of each channel is not listed; the
realistic view then has numbered pins and no channel) it is flagged in the
description of the element.

## Drives (AF1000 and AX5000)

The drives were redrawn in vector (rectangles, circles, text; no bitmap) from
the pictures of the sample schematic, which are kept outside the repository.
`misc/beckhoff/gen_drives.py` writes them (`--report` prints the residuals).
Each reference has three elements: the symbol, `_generic` and `_table`,
with the same label and information. Terminals are on the 10 px grid. The
generic view is coloured like a real front (grey body, black cover, red
band, status LEDs, grey RJ45 sockets, black connector housings, orange motor
block); it is plain vector drawing, not a picture of a product.

**Number of axes**: the second digit of the number in the reference. AF1103,
AF1107, AF1115, AF1130, AF1155, AX5101 to AX5125 are 1 axis; AF1215, AF1222,
AX5201 to AX5225 are 2 axes (a 2-axis element adds `M2-U/V/W/PE` and X21/X22
on the AX, the second power rows on the AF). The rule is the user's; no
reference breaks it. Only AX5203, AX5206 and the AF references named in the
schematic come from it; the other AX5100/AX5200 references (5101, 5103, 5106,
5112, 5118, 5125, 5201, 5212, 5218, 5225) share the drawing and exist from the
rule only, their existence is not checked against a datasheet. The rating of
the AF1155 (18 A) looks suspect.

| Family | Terminals | Scale | Largest gap to the picture |
| --- | --- | --- | --- |
| AF1000 | EtherCAT IN/OUT (X04, X05); X03 control (IN1.., GND1, TH1, TH2, OUT, FGND, 24V_A..C); M1-U/V/W/PE (+ M2- on 2 axes); L1 L2 L3 PE RB+ RB- | 0.95 | 7.6 px |
| AX5000 | EtherCAT IN/OUT; X11, X12 (+ X21, X22 on 2 axes); X06 (24, 0 to 7, 0V); DC+, DC-; L1 L2 L3N PE; Up, Us, GND; M1-U/V/W/PE (+ M2-) | 0.313 | 16.7 px |

The I/O table of an AF lists IN* (digital inputs), TH1/TH2 (analog) and OUT;
the AX lists X06-0 to X06-7 as universal channels.

Not known or not checked: 1-phase or 3-phase mains of the AF (the 3-phase
layout is drawn); the "25 Vdc" of a blurry picture is read as 24 V; the
function names of X03 and X06 come from blurry pictures; the motor and brake
connectors of the AX are not on the pictures, so `M1-*`/`M2-*` are generic
and the brake is not modelled; the AX5805 (TwinSAFE option) is a rectangle on
the picture and has no element.

## Safety cards (TwinSAFE, yellow)

`twinsafe/` has EL1904, EL1918, EL2904 (symbol, generic, realistic, table),
EL6910 and EK1960 (symbol and generic only), drawn with a yellow body. EL1904,
EL1918 and EL2904 have a **realistic** view taken from the user's drawings
(yellow). The **EL1918 has 8 terminals**, not 16: the drawing shows SI1..SI8
on pins 1, 5, 2, 6, 3, 7, 4, 8. The EL1904 and EL2904 have the channels on
pins 1, 5, 3, 7 and 0 V on 2, 6, 4, 8. The EL6910 and EK1960 are not
verified and flagged so in the description. `misc/beckhoff/gen_safety.py`
writes them.

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

A card can be drawn up to four times, as four elements: **symbol**
(`el1008.elmt`), **generic** view (`el1008_generic.elmt`), **realistic**
view (`el1008_realistic.elmt`) and **I/O table** (`el1008_table.elmt`);
the ones that do not exist for a card are skipped. The link between them is the label, not a
reference to another object:

```
island A1
 ├─ A1.00  head (ek1100)             symbol
 ├─ A1.10  card (el1008)             symbol + realistic + table
 ├─ A1.20  card (el2008)             symbol + realistic + table
 └─ A1.30  card (el5152)             symbol + realistic + table
```

- parent to card: the card label starts with the label of the head
  (`A1.10` belongs to `A1`), with a step of 10 inherited from the island;
- card to appearances: the elements have the same label (`A1.10`), the
  same manufacturer, reference and designation, and the same I/O table, so
  that the used pins and the tooltip see them as one card.

**Numbering > Create the other appearances...** (also in the context menu of
a card) adds the missing appearances of the selected card, on the current
folio (placed to its right), on another folio, or on a new folio, in one
undo step, with the label, the information and the I/O table copied. It
needs the appearances to be side by side in the same collection folder
(`X.elmt`, `X_generic.elmt`, `X_realistic.elmt`, `X_table.elmt`), as the
Beckhoff ones are. Renumbering and duplicating work on the label, so they
apply to every appearance at once.

The nomenclature of the project counts each appearance, so a card drawn three
times is counted three times by the core bill of materials. The I/O lists
of the island and `qet_plc_io` list a card once.

### Realistic appearances and the grid

**Generic** = the views drawn here (vector, no manufacturer picture).
**Realistic** = the user's own drawings of the cards, with their geometry and
colours (the yellow of the safety cards). They are rebuilt by
`misc/beckhoff/realistic_from_model.py <folder of the drawings> <elements>`:
each terminal is placed on the centre of the screw circle it belongs to, and
the drawing is scaled and shifted so that these centres fall on the grid of
QElectroTech. The drawings are read from a folder outside the repository (a
local copy of the user's own company collection); no generated file, example
or test refers to that collection at run time, and the example project does
not embed it.

| Element | Scale | Grid | Largest remaining gap | Size, hotspot |
| --- | --- | --- | --- | --- |
| single-width cards (EL1004, EL1008, EL1904, EL1918, EL2004, EL2008, EL2522, EL2574, EL2904, EL5152, EL9100) | 1.26 | 10 px | 0.55 px | 80x400, (30,190) |
| `ek1914_realistic` | 1.00 | 5 px | 1.35 px | 130x230, (60,110) |
| `el2911_realistic` | 1.01 | 5 px | 0.80 px | 120x330, (30,160) |

The row pitch of the drawings is not uniform, so a single scale cannot bring
all the circles to the grid: the gap above is what is left, and the circles
(with the small parts inside them) are moved onto the grid nodes, so the
terminals are exactly at the centres. The EK1914 and EL2911 spacing only
allows a 5 px grid. The grid used is in the `informations` of each element
and checked by the tests. The hotspot is the origin of the drawing after the
offset. The RJ45 sockets of the EK1914 are not circles: their terminals
(EtherCAT IN/OUT) are at a position estimated on the drawing.

The pin names of the realistic views follow the position (left column 1 to 4,
right column 5 to 8, second group 9 to 16 on the EL2911); the signal printed
in the drawing (I1, S1, CH1A, P1+...) is the channel of the I/O table.
Uncertain: the channels of the EL2522, EL2574 and EL9100 (the labels of the
drawing are used as they are), the EL2911 (the drawing is doubled, channels
only for the first group) and the numbering of the EK1914 terminals (row by
row, 1 to 16).

### I/O table appearance

The `_table` elements draw the I/O table in **Roboto Condensed**, size 7,
with rows of 12 px (about 1.5 times the default), a bold header on a grey
band, and the columns Type, Address, Function, Comment, Cross reference and
Terminal wide enough for their text (the widths are in the element). Roboto
Condensed is installed on the machine where this was checked; the font file
is **not** in the repository. If it is missing, Arial Narrow or Liberation
Sans Narrow is used, then the system sans-serif.

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
  0 V, PE, feed-through). A channel whose entry lists pins (`5 → I2`) belongs
  to those pins; otherwise it belongs to the terminal whose name is its
  address (`X01(1)` belongs to `X01`).
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

`examples/beckhoff_islands_example.qet` has three islands on 10 folios (symbols,
generic and/or realistic views, I/O tables for each island):

- `A1`: head `A1.00` (EK1100), cards `A1.10` (EL1008), `A1.20` (EL2008),
  `A1.30` (EL5152). Two sensors are wired on pins 1 and 2 of `A1.10` (pin 2 is
  channel I3) and a lamp on pin 1 of `A1.20`; these pins show as used.
- `A2`: head `A2.00`, the yellow card `A2.10` (EL1918, 8 terminals), the
  1-axis drive `A2.20` (AF1103) and the 2-axis drive `A2.30` (AF1215). A
  sensor is wired on the pins 1 and 5 (SI1 and SI2) of `A2.10` and one on IN1 of
  `A2.20`.
- `A3`: head `A3.00`, the 1-axis drive `A3.10` (AX5103) and the 2-axis drive
  `A3.20` (AX5203).

The used channels carry their function in the I/O tables. The Beckhoff
elements used are embedded in the project. It is
written by `python misc/beckhoff/gen_example.py` (the uuids are fixed, so the
file is always the same). Open it with the QElectroTech built from this
branch (`build\qelectrotech.exe examples\beckhoff_islands_example.qet`).
Checked: `--info`, `--resave` (a second save is identical to the first) and
`--export-png`. The three appearances are tied by their label; there is no
cross reference between them, and the heads have no I/O table.

## Limits

- The ios of the EK1914 name the channels `I1`..`SO2`, but the pin of each
  channel is unknown (the user's drawing has no channel names), so they are
  not listed and the tooltip of its realistic view, with numbered pins,
  cannot tell the channels.
- The designation of the EL5152 is generic and was not checked against a
  datasheet.
- No table is added automatically on the nomenclature folio: choose the folio
  in *Create the other appearances*.
- The AX5805 and the EPP2338-0002 have no realistic view yet; the cards
  without a drawing of the user keep their symbol, generic view and table. The drives and
  the safety cards are not verified against a datasheet (see above).
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
