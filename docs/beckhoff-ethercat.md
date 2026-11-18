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

Every card has up to **four appearances**:

- the **schematic** (the symbol, `X.elmt`): the wiring diagram of the card
  drawn inside its dotted outline (rail clips, optocouplers or output stages,
  numbered terminals, +24 V / 0 V, power contacts), with the terminals at the
  same positions as the realistic view, so the views are interchangeable.
  The +24 V and 0 V rails run from a filled square (contact on the left) to a
  circle (contact on the right) and are joined by black dots to the 0 V / +24 V
  pins of the card; the couplers only show the two rails of the power
  contacts. The cards without 24 V / 0 V terminals (EL1008, EL2008, generic
  cards...) show the rails and the contacts only: the links are generic, not
  from a datasheet;
- the **realistic** view (`_realistic`): the front with only the pin numbers
  and the I/O labels;
- the **description** view (`_description`): the realistic view with the
  descriptions of the I/O on the sides (leader lines, LEDs);
- the **table** (`_table`): a simple table N° pin | Label | TwinCAT ID.

There are no plain rectangle symbols any more: the `*_generic` symbols and the
couplers are schematics in a dotted outline too. The generic cards
(`el1xxx_generic`, `el2xxx_generic`, `el3xxx_generic`, `el4xxx_generic`,
`el2652`) have the four appearances, generated from the common model of the
EL1008 / EL2008 / EL2004 front with the pin number as label (the signal of
each contact depends on the exact type). The couplers (EK1100, EK1322,
EK1960), the EL6910, the EL9011 and `ethercat_box_generic` only have the
schematic.

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
| `*_description.elmt` | as the symbol | as the symbol | the realistic view (same terminals, same scale) with annotated legends and the status LEDs, for every card that has a realistic view (38 cards, see "Description appearance") |
| `*_table.elmt` | as the symbol | as the symbol | none: the simple I/O table only (every card with a realistic view) |
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

A card can be drawn up to four times, as four elements: **symbol**
(`el1008.elmt`), **realistic** view (`el1008_realistic.elmt`),
**description** view (`el1008_description.elmt`) and
**I/O table** (`el1008_table.elmt`); the ones that do not exist for a card
are skipped. The link between them is the label, not a
reference to another object:

```
island A1
 ├─ A1.00  head (ek1100)             symbol
 ├─ A1.10  card (el1008)             symbol + realistic + description + table
 ├─ A1.20  card (el2008)             symbol + realistic + description + table
 └─ A1.30  card (el5152)             symbol + realistic + description + table
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
(`X.elmt`, `X_realistic.elmt`, `X_description.elmt`, `X_table.elmt`), as the
Beckhoff ones are. Renumbering and duplicating work on the label, so they
apply to every appearance at once.

The nomenclature of the project counts each appearance, so a card drawn four
times is counted four times by the core bill of materials. The I/O lists
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
| `ek1914_realistic` | 1.00 | 5 px | about 1.3 px | 130x330, (60,160) |
| `el2911_realistic` | 1.00 | 5 px | about 0.8 px | see the element |
| `el1918_realistic` | 1.00 | 5 px | 0 | 60x330, (30,160) |

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

**Terminals are only a circle.** In every realistic view of a card (not the
drives) a terminal is drawn as its circle, with the label of the terminal
(I1, 0V, OUT, 24V...) above it: the small square above the circle (the wire
slot) is not drawn. `misc/beckhoff/realistic_fixes.py` applies this, and the
two layouts below, to the generated views.

- **EK1914**: same height as the other cards (330, the rows are 25 px apart,
  stretched from the traced drawing, so the traced parts are a little
  distorted). The numbers 1 to 16 are outside the block of terminals, next to
  their circle (odd pins on the left, even pins on the right), and the whole
  lower block (pins 11 to 16, TwinSAFE) is yellow, as the TwinSAFE cards.
- **EL1918**: the same housing as the other cards (60x330, one yellow body),
  with 16 pins instead of 8: two columns of 8 rows, pins 1 to 8 in the left
  column and 9 to 16 in the right one, the rows 25 px apart (estimated from
  the front picture). The signals follow the Beckhoff order, not the channel
  order (Sn+ on the odd pins, SIn on the even ones, channels 1, 3, 5, 7 first,
  then 2, 4, 6, 8).
  `misc/beckhoff/fix_boxes.py` applies the two layouts.

### Description appearance

`*_description.elmt` is the realistic view with legends, as an annotated front
of a manufacturer manual: the same terminals on the same circles, the same
scale and the same position of the reference, and in addition

- a legend with a leader line for each terminal, on the left or the right of
  the card: `pin: signal - function` (`1: I1 - Input 1`, `2: 0V1 - 0 V`); the
  signal is the address of the I/O table, the pin is shown when it differs
  from the signal, the function is the text of the table or a factual generic
  one (Input n, Output n, Safe input n, +24 V DC, 0 V...);
- the **status LEDs**: one LED per channel at the top of the front, with a
  "Signal n" legend, and a "Diagnostic" LED for the TwinSAFE cards; four
  status LEDs for the EK1914; the header LEDs of the drives, which a "Status
  LEDs" legend points to;
- the power contacts (+24 V DC / 0 V) as two black marks on the side of the
  card, with a legend;
- for the drives, the connectors are summarised by one legend per connector
  (`Motor 1: U, V, W, PE`); for the EPP2338, one legend per socket.

The legends use Roboto Condensed (same fallback as the tables). The box of
the element is widened (only the width, height and hotspot change, never the
coordinates) so the legends fit and the terminals stay on the grid. The
files are written by `python misc/beckhoff/gen_description.py
<15_beckhoff_ethercat>` from the realistic views and their I/O tables (no
duplicated data; a second run gives identical files). There is one for each
of the 33 cards that have a realistic view: AF1103, AF1107, AF1115, AF1130,
AF1155, AF1215, AF1222, AX5101, AX5103, AX5106, AX5112, AX5118, AX5125,
AX5201, AX5203, AX5206, AX5212, AX5218, AX5225, EK1914, EL1004, EL1008,
EL1904, EL1918, EL2004, EL2008, EL2522, EL2574, EL2904, EL2911, EL5152,
EL9100 and EPP2338.

INCERTAIN: the number and the position of the LEDs, the "Signal n" and
"Diagnostic" legends, the power contacts and the generic functions (for
example "Functional ground", "Analog input") are generic, not taken from a
datasheet; the LEDs are drawn neutral (green); the EL2911 legends only cover
the outer columns (pins 1 to 4 and 13 to 16); the EK1914 legends assume
terminals 1 to 10 standard and 11 to 16 TwinSAFE. The layout (LED area at the
top, "Signal n" legends, power contacts) follows an annotated front of an
EL1004 given as a reference, redrawn in vector; no image or text of the
manufacturer is in the repository.

### I/O table appearance

The `_table` elements draw a **simple table with three columns**: `N° pin`,
`Label` and `ID TwinCAT`, in **Roboto Condensed**, size 7, with rows of 12 px
(about 1.5 times the default), a bold header on a grey band. There is no type,
function, address or comment column on the appearance. The data of the I/O
table (address, type, function, comment, terminals) is still stored in the
card as before: the TwinCAT synchronisation, the tooltips, the live cross
reference and the exports keep using it, only the drawing is simplified.

- N° pin: the pins of the channel (the Terminal column, index 5).
- Label: the function text of the channel.
- ID TwinCAT: the address of the channel (`I1`, `Q2`, `SI1`, `O3`,
  `X01(1)`...). The full identifier is `<card label>.<address>`, for example
  `A1.10.I3`. This is what the TwinCAT synchronisation uses as it is: the box
  is found by the label of the card, and the channel by its direction and the
  trailing number of the address. For the drives (`IN5`, `TH1`, `OUT`) and the
  EPP2338 (`X01(1)`) the trailing number is the one used for the match.

The columns are configurable per element (no fork of the drawing code): the
element keeps `columnVisibility`, `columnOrder` (here `5,2,1`) and
`columnNames` (the six columns, index 5 is the Terminal column). The simple
table is the default for the `_table` appearance. The element properties
editor only knows the five first columns: saving a table from it may drop the
name or visibility of the Terminal column. Roboto
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
realistic views, I/O tables for each island, and the description views of the
cards of `A1`), all on A3 folios with the A3 title
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
`--export-png`. The appearances are tied by their label; there is no
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

The EPP2338-0002 (EtherCAT P box, `epp_box`, reference `EPP2338-0002`) is a
dark box, not a terminal card: 8 channels on four **M12** 5-pin sockets X01 to
X04 (two channels per socket) and two **M8** EtherCAT P sockets X50 (IN) and
X51 (OUT). It has four appearances, all original vector drawings that follow
the structure of the reference pictures given by the user (socket stack, LEDs,
labelling areas, pin-out); no image or manufacturer text is in the repository:

- `epp2338.elmt` (schematic): dotted module outline with rail clips, the
  EtherCAT P sockets, a +24 V and a 0 V rail, the four M12 sockets, one block
  per channel (LED and phototransistor). Terminals are on the circles, on the
  5 px grid.
- `epp2338_realistic.elmt`: the front of the box at the EK1914 scale, with the
  six sockets, the LEDs (one per channel, L/A, Run, Us/Up) and the reference
  inside, at the bottom. **One terminal per M12 socket**: `X01` to `X04`
  (the other pins of the socket are drawn, not terminals); the two EtherCAT P
  sockets keep their terminals `IN X50` and `OUT X51`.
- `epp2338_description.elmt` and `epp2338_table.elmt`.

Each socket carries two channels, so two I/Os share the same terminal (A:
pin 4, B: pin 2 of the socket): the table has two rows per pin (`X01(1)`,
`X01(2)`, ...). The model of QET already allows several I/Os on one terminal
(the tooltip lists them by terminal, the TwinCAT match uses the sense and the
trailing number of the address, the cross reference goes by the index of the
I/O), so nothing changes in the program. The other pins of the socket are
+24 V DC Up (1), GND P (3), not connected (5).

INCERTAIN: the positions of the sockets, the LEDs and the labelling areas on
the front are assumed from the reference picture; check the pin-outs against
the datasheet of your box.

## Limits

- The generic cards and the couplers are generated from the common model, not
  from a drawing of the user. The stage circuits of the schematics (input
  optocoupler, output stage) are generic, not taken from a datasheet. The
  couplers, the EL6910, the EL9011 and the generic EtherCAT box have no
  realistic, description or table view yet.

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
- The +24 V / 0 V rails, the junction dots and the power contacts of the
  schematic views are generic drawings (decoration, no terminal): cards
  without 24 V / 0 V terminals only show the rails and the contacts, the
  couplers only show the two power contact rails, and the drives have no new
  marker (their mains and DC link are already drawn). They are not taken from
  a datasheet.
- The channel-to-pin mapping and the vertical pitch of the EK1914 (stretched
  to the common card height) and the row pitch of the EL1918 (16 pins in the
  standard housing) are estimated from the reference images.
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
python misc/beckhoff/realistic_fixes.py extra-elements/15_beckhoff_ethercat
python misc/beckhoff/gen_epp.py extra-elements/15_beckhoff_ethercat
python misc/beckhoff/fix_boxes.py extra-elements/15_beckhoff_ethercat
python misc/beckhoff/gen_generic.py extra-elements/15_beckhoff_ethercat
python misc/beckhoff/gen_schematic.py <absolute path of extra-elements/15_beckhoff_ethercat>
python misc/beckhoff/gen_couplers.py extra-elements/15_beckhoff_ethercat
python misc/beckhoff/gen_description.py extra-elements/15_beckhoff_ethercat
python misc/beckhoff/simple_tables.py extra-elements/15_beckhoff_ethercat
python misc/beckhoff/gen_titleblocks.py
python misc/beckhoff/gen_example.py
```

`organize.py` replaces the content of `extra-elements/15_beckhoff_ethercat`
(family folders and their `qet_directory` names).
