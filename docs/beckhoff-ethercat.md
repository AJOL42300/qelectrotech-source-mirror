# Beckhoff EtherCAT islands

A small set of symbols and tools for drawing a Beckhoff EtherCAT
architecture: a coupler with a stack of I/O terminals, EtherCAT boxes in a
daisy chain, and AF1000 drives. The symbols are in
`extra-elements/15_beckhoff_ethercat` and are installed into the common
collection, as the category **Beckhoff EtherCAT**, sorted in one folder per
family:

| Folder | Content |
| --- | --- |
| `ek_couplers` | EK1100, EK1322, EK1914 |
| `el1xxx_digital_in` | EL1004, EL1008, `el1xxx_generic` |
| `el2xxx_digital_out` | EL2004, EL2008, EL2522, EL2574, EL2652, `el2xxx_generic` |
| `el3xxx_analog_in` | `el3xxx_generic` |
| `el4xxx_analog_out` | `el4xxx_generic` |
| `el5xxx_encoder` | EL5152 |
| `el9xxx_system` | EL9011, EL9100 |
| `epp_box` | EPP2338, `ethercat_box_generic` |
| `twinsafe` | EL1904, EL1918, EL2904, EL2911, EL6910, EK1960 (yellow) |
| `drives_af1000`, `drives_ax5000` | the drives |

`misc/beckhoff/organize.py <scratch folder> extra-elements` sorts the output
of the generators into these folders (see
[Regenerating the library](#regenerating-the-library)). The elements named
`*_generic` in the table (`el1xxx_generic`...) are the base symbols of a
family, not a view.

Every card has up to **three appearances**: the symbol, the realistic view
(`_realistic`) and the I/O table (`_table`). A card without a drawing of its
own keeps its symbol and its table. There is no "generic view" any more.

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
| `*_realistic.elmt` | as the symbol | as the symbol | terminals on the screw circles of the drawing (EL1004, EL1008, EL1904, EL1918, EL2004, EL2008, EL2522, EL2574, EL2904, EL2911, EL5152, EL9100, EK1914, EPP2338), and the vector view of the drives |
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
  type "universal"; the actual use of each channel is yours to define. The M8
  pin-out and the physical layout are assumed (see [EPP2338](#epp2338-box)).
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
| EL1918 | 16 pins in 8 rows of 2: SIn on the "-" pin, the clock output Sn+ on the other one; pins 1/2 for SI1, 3/4 for SI3, 5/6 for SI5, 7/8 for SI7, 9/10 for SI2, ... 15/16 for SI8 | the user: **16 terminals**; the order of the inputs 1, 3, 5, 7, 2, 4, 6, 8 follows the public terminal list of the card (see [Safety cards](#safety-cards-twinsafe-yellow)) |
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
Each reference has three elements: the symbol, `_realistic` (the vector
view, a little smaller than in the first version: 160x506 px for the AF,
190x506 px for the AX) and `_table`, with the same label and information.
Terminals are on the 10 px grid. The realistic view is coloured like a real front (grey body, black cover, red
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

`twinsafe/` has EL1904, EL1918, EL2904 and EL2911 (symbol, realistic view and
table), EL6910 and EK1960 (symbol only),
drawn with a yellow body. EL1904, EL1918, EL2904 and EL2911 have a
**realistic** view; the EK1914 coupler (`ek_couplers`) has its lowest block of
terminals (pins 11 to 16, the TwinSAFE part) in yellow.

**EL1918: 16 terminals.** The user says that the card has 16 terminals (this
replaces the earlier 8-terminal drawing). The factual layout comes from the
public list of the terminals of the card (8 safe inputs, each with its clock
output): one pair per input, the clock output `Sn+` and the safe input `SIn`.
The order of the inputs on the housing is 1, 3, 5, 7, 2, 4, 6, 8, so the
pairs of pins are 1/2 for SI1, 3/4 for SI3, 5/6 for SI5, 7/8 for SI7, 9/10 for
SI2, 11/12 for SI4, 13/14 for SI6, 15/16 for SI8. **Uncertain**: the exact
position of the terminals on the real front (rows of two, left to right) is
an assumption; the drawing is a layout made here, in vector, with a pitch of
15 px between the rows, and nothing is copied from the manufacturer. The
table lists the two pins of each input. The EL1904 and EL2904 have the
channels on pins 1, 5, 3, 7 and 0 V on 2, 6, 4, 8. The EL6910 and EK1960 are
not verified and flagged so in the description. `misc/beckhoff/gen_safety.py`
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

A card can be drawn up to three times, as three elements: **symbol**
(`el1008.elmt`), **realistic** view (`el1008_realistic.elmt`) and
**I/O table** (`el1008_table.elmt`); the ones that do not exist for a card
are skipped. The link between them is the label, not a
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
(`X.elmt`, `X_realistic.elmt`, `X_table.elmt`), as the
Beckhoff ones are. Renumbering and duplicating work on the label, so they
apply to every appearance at once.

The nomenclature of the project counts each appearance, so a card drawn three
times is counted three times by the core bill of materials. The I/O lists
of the island and `qet_plc_io` list a card once.

### Realistic appearances and the grid

**Realistic** = the user's own drawings of the cards, with their geometry and
colours (the yellow of the safety cards); the drives have a vector view drawn
here. The card views are rebuilt by
`misc/beckhoff/realistic_from_model.py <folder of the drawings> <elements>`:
each terminal is placed on the centre of the screw circle it belongs to. All
the card views are at the **scale of the EK1914 (1.00)**, so a card has the
same size in every view, and the reference of the card (EL1004, EL2911...)
is drawn inside the card, at the bottom. The drawings are read from a folder
outside the repository (a local copy of the user's own company collection); no
generated file, example or test refers to that collection at run time, and
the example project does not embed it.

| Element | Scale | Grid | Largest remaining gap | Size, hotspot |
| --- | --- | --- | --- | --- |
| single-width cards (EL1004, EL1008, EL1904, EL2004, EL2008, EL2522, EL2574, EL2904, EL5152, EL9100) | 1.00 | 5 px | about 1 px | 60x330, (30,160) |
| `ek1914_realistic` | 1.00 | 5 px | about 1.3 px | 130x220, (60,110) |
| `el2911_realistic` | 1.00 | 5 px | about 0.8 px | see the element |
| `el1918_realistic` | 1.00 | 5 px | 0.4 px | 90x160, (20,50) |

The row pitch of the drawings is not uniform, so a single scale cannot bring
all the circles to the grid: the gap above is what is left, and the circles
(with the small parts inside them) are moved onto the grid nodes, so the
terminals are exactly at the centres. **Grid 5 px, not 10**: at the scale of
the EK1914 the spacing of the columns of the EL cards is 15 px and the gaps
between the rows of the EK1914 are 14.5 and 21 px (ratio 2:3), so no uniform
scale gives a 10 px grid for all of them; the 10 px grid is kept for the
drives only. A terminal is on a node of the grid when the element is placed
with its hotspot on a node (the 5 px nodes are on the 10 px grid every other
time). The grid used is in the `informations` of each element
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

`examples/beckhoff_islands_example.qet` has three islands on 9 folios (symbols,
realistic views, I/O tables for each island), all on A3 folios with the A3 title
blocks (see below):

- `A1`: head `A1.00` (EK1100), cards `A1.10` (EL1008), `A1.20` (EL2008),
  `A1.30` (EL5152). Two sensors are wired on pins 1 and 2 of `A1.10` (pin 2 is
  channel I3) and a lamp on pin 1 of `A1.20`; these pins show as used.
- `A2`: head `A2.00`, the yellow card `A2.10` (EL1918, 16 terminals), the
  1-axis drive `A2.20` (AF1103) and the 2-axis drive `A2.30` (AF1215). A
  sensor is wired on the pins 2 and 4 (SI1 and SI3) of `A2.10` and one on IN1
  of `A2.20`.
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

### A3 title blocks

The folder `titleblocks` has two templates, **A3 landscape** and **A3 portrait**
(`A3_landscape.titleblock`, `A3_portrait.titleblock`), installed with the
other title blocks (the build reads `build\titleblocks` next to the
executable: copy the folder there to see them in the list of templates). They
reproduce the layout of the title block of a typical schematic: a strip of two
rows of 25 px under the folio, with an empty cell for a logo, the author, the
project title and the file name on the first row, then the date, the title of
the folio and the folio number. The widths are relative, so the strip fits the
folio. They use variables only: no logo and no company text is in the
repository (put your logo in the first cell with the title block editor).

A3 is 20 x 10 cells of 61 x 80 px for a landscape folio and 14 x 15 cells for a
portrait folio (about 2.9 px per mm). In the example, every folio is landscape
except the I/O tables of the island `A1` (portrait, to show it). The
templates are embedded in the project, so it does not need the installed
ones, and new folios of the example are created as A3 landscape.
`python misc/beckhoff/gen_titleblocks.py` writes the two files.

## EPP2338 box

The EPP2338 (EtherCAT P box, `epp_box`) has two views, both original vector
drawings made from the public description of the box (M8 connectors, two
channels per socket); no Beckhoff image or text is used:

- `epp2338.elmt`: a functional view in the style of a module diagram: dotted
  module outline with rail clips, IN X50 / OUT X51, a +24 V and a 0 V rail,
  four sockets X01 to X04 of four pins, one block per channel (LED and
  phototransistor), junction dots on the supplies. Terminals are on the
  circles, on the 5 px grid.
- `epp2338_realistic.elmt`: the front of the box at the EK1914 scale, with the
  six sockets, their LEDs and the reference inside, at the bottom.
  The terminals are named `X0n.1` to `X0n.4` (pin of the socket); the two
  extra terminals are `IN X50` and `OUT X51`.

The table, the views and "Create the other appearances" share the same data.
Each channel has its pin: channel 1 of a socket is on pin 4 and channel 2 on
pin 2, pin 1 is +24 V and pin 3 is 0 V.

INCERTAIN: the M8 pin-out and the position of the sockets on the front are
assumed (pin numbering of the realistic view too). Check them against the
datasheet of your box. The functional style could also be applied to the other
cards if you want it.
## Limits

- The ios of the EK1914 name the channels `I1`..`SO2`, but the pin of each
  channel is unknown (the user's drawing has no channel names), so they are
  not listed and the tooltip of its realistic view, with numbered pins,
  cannot tell the channels.
- The designation of the EL5152 is generic and was not checked against a
  datasheet.
- No table is added automatically on the nomenclature folio: choose the folio
  in *Create the other appearances*.
- The AX5805 has no realistic view yet; the cards
  without a drawing of the user keep their symbol and table. The drives and
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

## Regenerating the library

From the root of the repository (the drawings of the user are read from a
local folder outside the repository; `<scratch>` is any empty folder):

```
python misc/beckhoff/gen_symbols.py <scratch>
python misc/beckhoff/gen_safety.py <scratch>
python misc/beckhoff/gen_drives.py <scratch>
python misc/beckhoff/realistic_from_model.py <drawings> <scratch>/15_beckhoff_ethercat
python misc/beckhoff/organize.py <scratch> extra-elements
python misc/beckhoff/gen_titleblocks.py
python misc/beckhoff/gen_example.py
```

`organize.py` replaces the content of `extra-elements/15_beckhoff_ethercat`
(family folders and their `qet_directory` names).
