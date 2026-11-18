# Murrelektronik MICO and Mean Well power supplies

Symbols for the 24 V DC side of a control cabinet: Murrelektronik MICO
electronic circuit breakers and Mean Well DIN-rail power supplies. They are
built on the same principle as the [Beckhoff library](beckhoff-ethercat.md).
The symbols are in `extra-elements/16_murrelektronik_mico` and
`extra-elements/17_meanwell_psu`, installed into the common collection as the
categories **Murrelektronik MICO electronic circuit breakers** and
**Mean Well DIN-rail power supplies** (translated in `qet_directory`:
English, French, German).

Each product has its own folder, below a folder for its family:

| Folder | Products |
| --- | --- |
| `16_murrelektronik_mico/mico_basic` | MICO Basic 2.4, 2.6, 2.10 (9000-41042), 4.4, 4.6, 4.10 (9000-41034), 4.4.10, 8.6 |
| `16_murrelektronik_mico/mico_pro` | MICO Pro 1, 2 and 4 channels |
| `17_meanwell_psu/hdr` | HDR-15, 30, 60, 100 (24N), 150 |
| `17_meanwell_psu/ndr` | NDR-120, 240, 480 |
| `17_meanwell_psu/sdr` | SDR-75, 120, 240, 480, 960 |
| `17_meanwell_psu/edr` | EDR-75, 120, 150 |
| `17_meanwell_psu/drp` | DRP-240, 480 |
| `17_meanwell_psu/wdr` | WDR-60 (5, 12, 24, 48 V), WDR-120 (12, 24, 48 V), WDR-240 (24, 48 V), WDR-480 (24, 48 V) |

All are 24 V versions except the WDR series, which is included for every
voltage of the public data sheet (5, 12, 24 and 48 V). A product folder
holds four **appearances**, named like
the folder: `X.elmt` (schematic), `X_realistic`, `X_description` and
`X_table`. As for Beckhoff, the four are masters with the **terminals at the
same positions and with the same names**, so the views can be swapped on a
folio without redrawing the wires. Each power supply also has a fifth,
non-electrical element, `X_layout`, for the cabinet layout (see below).

- **schematic**: the internal wiring inside a dotted outline. The +V rail
  is red, the 0 V rail blue, PE yellow-green (dashed). A power supply shows
  the AC/DC converter and, for the models that have one, the normally open
  "DC OK" relay contact. A MICO shows the +24 V bus, one current-limited
  channel (I>) per output and the control logic;
- **realistic**: stylised front view with the terminal markings, the LED, the
  adjustment potentiometer (power supplies) or the push button and current
  selector of each channel (MICO). It is a redrawn vector view of the front
  face (screw terminal blocks, LED, trimmer, housing outline), not a photo;
- **description**: the realistic view with legends and leader lines for the
  terminals and the LED;
- **table**: a simple table `N° pin | Label | ID`;
- **layout** (power supplies only, `X_layout`): a front-view footprint at
  **1 px = 2 mm** for the cabinet layout, drawn to the width and height of the
  data sheet, with the 35 mm DIN rail (two dashed lines behind the body), the
  terminal screws on the top and bottom strips and the reference as a
  readable label (rotated on narrow housings). It is a simple element without
  terminals, so it never takes part in the wiring. The depth is in the
  description ("... 110x125.2x150 mm (width x height x depth)").
  The same figures are written to
  `extra-elements/17_meanwell_psu/devices_layout.json` to be merged into the
  device list of the enclosure layout (`ide-enclosures/devices.json`) when the
  implantation branch (`ajol42300-rail-snapping-implantation`) is merged:
  nothing of that branch is included here.

## Terminals

The number of terminals follows the public data sheets: every `+V` and `-V`
screw of the output is a terminal of its own (`+V`, `+V2`, `+V3`, `-V`,
`-V2`, `-V3`).

| Series / model | Terminals |
| --- | --- |
| HDR-15, 30 | L, N, +V, -V (no PE) |
| HDR-60, 100, 150 | L, N, +V, +V2, -V, -V2 (no PE) |
| NDR-120, 240, 480; EDR-75, 120, 150; DRP-240, 480 | L, N, PE, +V, +V2, -V, -V2 |
| SDR-75 | L, N, PE, +V, +V2, -V, -V2 |
| SDR-120 | L, N, PE, +V, -V, 13, 14 |
| SDR-240, 480 | L, N, PE, +V, +V2, -V, -V2, 13, 14 |
| SDR-960 | L, N, PE, +V, +V2, +V3, -V, -V2, -V3, P-, P+ (current sharing), 13, 14 |
| WDR-60, 120 | L1, L2, PE, +V, -V, 13, 14 |
| WDR-240, 480 | L1, L2, PE, +V, +V2, -V, -V2, 13, 14 |

| Product | Terminals |
| --- | --- |
| Power supply | see above; 13 and 14 are the DC OK relay contact |
| MICO Basic | +24V, 0V, ON (enable input), 13, 14 (group alarm contact), OUT1...OUTn |
| MICO Pro | +24V, 0V, CTRL (control input), 13, 14 (group alarm contact), 23, 24 (90 % load contact), OUT1...OUTn and 0V1...0Vn |

The MICO terminal numbers are written beside the stems in the schematic and
outside the white terminal squares in the realistic view. The MICO current
range is shown boxed, in bold and centred under the selectors (for example
`4 x 1-2-3-4 A`); a mixed module shows one box per group of channels.

### MICO Basic references

| Reference | Name | Channels | Selector steps per channel |
| --- | --- | --- | --- |
| 9000-41042-0100400 | MICO Basic 2.4 | 2 | 1-2-3-4 A |
| 9000-41042-0100600 | MICO Basic 2.6 | 2 | 1-2-4-6 A |
| 9000-41042-0401000 | MICO Basic 2.10 | 2 | 4-6-8-10 A |
| 9000-41034-0100400 | MICO Basic 4.4 | 4 | 1-2-3-4 A |
| 9000-41034-0100600 | MICO Basic 4.6 | 4 | 1-2-4-6 A |
| 9000-41034-0401000 | MICO Basic 4.10 | 4 | 4-6-8-10 A |

Each channel has a transparent push button square with the status LED built
in (no separate LED row, no round button) and a selector with these discrete
steps. Sources: the Murrelektronik shop pages of these references
(`shop.murrelektronik.com`), the "MICO Classic" listing of Mouser and the
instructions of the MICO 2.4 / 4.4 (13-14 alarm contact). The 4.4.10
(9000-41034-0101000, split ranges), the 8.6 (9000-41068-0600000) and the Pro
references are kept from the earlier version and are **not verified**.

The terminals carry a `potential` (24V / 0V) and a plcIO type, so that the
data of the pins (the ID column of the table, the function text) can be filled
as for the Beckhoff cards. The element data hold the figures:

- power supply: manufacturer, reference and designation such as
  "DIN-rail power supply, 1-phase AC input, 24 V DC 10 A, 240 W, 63 mm wide";
- MICO: "N-channel electronic circuit breaker" with the current per channel
  (adjustable range such as 1-4 A, or fixed such as 6 A) and the order number.

## M12 cables, distributors and connectors

`extra-elements/18_murrelektronik_m12` holds the Murrelektronik M12 parts,
one appearance per product (a plain schematic symbol, no realistic, description
or table view), one sub-folder per family:

| Folder | Products |
| --- | --- |
| `m12_cable` | Moulded cable M12 male / M12 female, A-coded, PUR 4 x 0.34: `7000-40021-6340050`, `-6340100`, `-6340200`, `-6340300`, `-6340500`, `-6341000` (0.5, 1, 2, 3, 5, 10 m) |
| `m12_y_distributor` | Y distributor M12 male / valve connector form BI 18 mm: `7000-42421-6260060` (0.6 m), `-6260030` (0.3 m) |
| `m12_connector` | Free M12 A-coded connector, IDC terminals: male `7000-12481-0000000`, female `7000-12601-0000000` |
| `m12_t_coupler` | T coupler, M12 male 5-pole base and 2 x M12 female (5 contacts, 4 wired), 3 LEDs GN / WH / YE: `7000-41135-0000000` |
| `m12_valve_plug` | Valve plug with LED and suppression diodes, form BI, 24 V AC/DC, with cable: `7000-11021-2160500`, `7000-18141-6R52500` |
| `m12_valve_coil` | Generic solenoid valve coil form BI 18 mm, 24 V DC (no manufacturer reference) |

Principles:

- **one terminal per M12 connector** (`X1` for a free connector or a coil,
  `X1` / `X2` for the two ends of a cable, `X1` / `X2` / `X3` for the Y
  distributor and the T coupler), like the EPP2338-0002 which has one terminal
  per M12 socket. The pins are only drawn (circle with pins numbered 1 to 5:
  four corners and the centre), never terminals;
- **male = solid pins, female = hollow sockets**, the female drawing being the
  mirror of the male one; the cable is male on the left (EPP side) and female
  on the right; the wires inside the cable are not drawn, the length and the
  order number are written on the cable;
- the data of each element are the manufacturer, the exact order number
  (`manufacturer_reference`), the designation in French, and a description
  with the poles, the coding, the length and the colour code of the conductors
  (1 brown, 2 white, 3 blue, 4 black);
- **detailed drawings** (vector, redrawn): the M12 connector has a knurled
  coupling nut, a thread ring, an insulator, a coding key (a lug on the male,
  a slot on the female) and contacts numbered 1 to 5 in the pin colours
  (brown, white, blue, black, grey); the cable has an overmould at each end and
  a grey sheath; the free connector adds a side view (nut with ribs, body, IDC
  cap); the T coupler has a T-shaped body, round green LEDs aligned with a
  legend (`X2`, `X1`, `X3`, the output they follow); the Y distributor has the
  overmoulded junction, two branches and two valve connectors whose contacts
  1, 2 and PE are numbered without overlap (red, blue, orange, the library
  colour code); the valve coil has the body with the winding, the valve core,
  A1 / A2 inside the outline and `24 V DC`. The terminal positions, the grid
  and the data are unchanged by these drawings;
- the links are drawn with a **dashed conductor** (the existing conductor
  line style, `line-style: dashed;` in the conductor `style` attribute, shown
  with the pattern of `conductordashpattern.h`). In the example the dashed
  conductors are set in the project; in the application, choose the dashed
  style in the conductor properties (or in the default conductor of the
  folio / project).

Folio 7 of the example links, with dashed conductors, the M12 sockets `X01`
and `X02` of an EPP2338-0002 to a 1 m cable, then a Y distributor feeding two
valve coils, and a 2 m cable, then a T coupler and two valve coils; a MICO
Basic output feeds a 5 m cable and a valve coil. Folio 8 shows every M12
product.

Dependency: these cables are elements (symbols). The branch
`ajol42300-network-cables-ethercat` (not merged here) offers another way of
describing a cable, as conductor properties with a built-in catalogue (it
already knows `7000-40021-6340500` and a Y distributor `7000-42401-6360060`)
and a user catalogue `cable_catalog.json`. Nothing of that branch is used or
duplicated here; after its merge the other 40021 lengths and the 42421
distributors can be added to its catalogue without any change to the
elements.

Assumed, to check against the datasheets:

- the cable 7000-40021: the female end has **5 contacts** but the cable has
  **4 conductors** (1 brown, 2 white, 3 blue, 4 black); contact 5 is drawn
  but not wired, the male end draws 4 pins. This is the user's information,
  not read in a datasheet; the free connectors are 4-pole (12481 from the
  datasheet, 12601 by analogy);
- female contacts are drawn as discs filled with the pin colour, with a
  black outline and a small hole, so that the colours are as readable as on
  the male pins; the male/female difference stays in the shape (solid pin
  vs. socket with hole and slot);
- **T coupler** (wiring read from the internal diagram supplied by the user,
  "vérifié par schéma fourni par l'utilisateur"): base pin 1 -> contact 1 of
  both sockets; pin 4 -> contact 4 of socket 1 only; pin 2 -> contact 4 of
  socket 2 only; pin 3 -> contact 3 and pin 5 -> contact 5 of both; three
  LEDs with a resistor, cathodes on pin 3: GN between pins 1 and 3, WH between
  pins 2 and 3, YE between pins 4 and 3. Dimensions 32 x 45 mm, M12x1, socket
  spacing 16.4 mm, from the user's dimensioned drawing (recorded in the
  description, not to scale in the symbol). The internal diagram is drawn
  inside a T-shaped housing silhouette (stem with the pin lines and LEDs,
  bar with the buses and socket labels; stylised, not the exact housing
  proportions). The exact order reference
  (kept `7000-41135-0000000` from the original list) is **to be confirmed**
  on murrelektronik.com. There is no realistic or layout (32 x 45 mm)
  appearance for the coupler: the M12 library is single-appearance;
- **valve plugs** `7000-11021-2160500` (MSUD, form BI 11 mm, 24 V AC +/-20 %
  / DC +/-25 %, LED and suppression, 4 A per contact, cable length 5 m
  inferred from the suffix) and `7000-18141-6R52500` (24 V AC/DC, LED
  green/yellow, PUR 5 x 0.5, 25 m; the shop title says A-18 mm while the text
  says form BI 11 mm). Sources: shop.murrelektronik.com, "Valve-Connectors /
  With-open-ended-wires" pages of these two references. The drawn circuit
  (LED + resistor, two anti-series suppression diodes between contacts 1 and
  2, PE) comes from the Y distributor diagram supplied by the user, not from
  a plug datasheet; diode type (TVS or Zener) and polarity are not confirmed.
  Form BI is 11 mm on the Murrelektronik site, the library keeps "18 mm"
  from the designation of the 42421;
- Y distributor 7000-42421 ("vérifié par schéma de fiche fourni par
  l'utilisateur"): male M12 with pins 2, 3, 4, 5 wired (pin 1 free); pin 4 ->
  contact 1 of valve 1, pin 2 -> contact 1 of valve 2, pin 3 -> contact 2 of
  both, pin 5 -> PE of both; each valve connector has a yellow LED, a
  resistor and two anti-series suppression diodes between 1 and 2;
- the position of the pins (1 top left, 2 top right, 3 bottom right,
  4 bottom left, 5 centre, seen from the front of the male connector) and the
  colours of the conductors follow the usual M12 A-coded convention and the
  wire order given in the datasheet;
- Y distributor 7000-42421: genders (male M12 on the EPP side, valve
  connectors as sockets) and branch lengths are assumed; its own datasheet
  was not found (only the 42401 family);
- the T-shaped body of the coupler is a schematic silhouette, not to scale;
- the coding key position, the contact layout and the pin colours of the
  M12 drawings come from the general A-coded convention, and the contact
  colours of the valve connector (red, blue, orange) are the library colour
  code, not the manufacturer's;
- the symbols are schematic, the drawn length of a cable does not depend on
  its real length;
- the valve coil is a generic symbol (A1 / A2, 24 V DC, connector form BI
  18 mm); it has no manufacturer reference.

## Function following the chain (EPP to valve)

The function written on an output of the EPP2338-0002 (for example `EV12`) is
shown on the valve coil that the output drives, through the moulded cables,
the T coupler and the Y distributor, and the output shows where it goes.

What QElectroTech already has: potentials and the "function" of a terminal
are followed by `Terminal::relatedPotentialTerminal`, but only through folio
reports and terminal-type elements; "simple" elements (cables, couplers, coils)
stop the search. The PLC data (`functionText`, comment, terminal of a channel)
are stored in the `PlcIO` of the master and pushed to the linked slaves
(`Element::setElementData`, `LinkElementCommand`), which is a master / slave
link, not a terminal to terminal chain. `ModuleTooltip::entriesOf` and
`PinUsage` show the channels of a module but do not follow conductors.

Approach without C++ (nothing in the application changes):

- the cables, the Y distributor and the T coupler are passive transit
  elements. `function_routes.json` (written by `gen_m12.py` from its `ROUTES`
  table) tells, for each family, which channel of which connector comes out
  on which connector: cable `X1` <-> `X2` (channel 1 and 2 kept); T coupler
  and Y distributor: trunk `X1` channel 1 <-> `X2`, trunk channel 2 <-> `X3`.
  A branch carries one channel, seen as its channel 1. The two channels
  never mix, and a channel with no route (for example channel 3) ends the
  chain, as does a missing conductor;
- `misc/murr_meanwell/chain.py` follows the conductors with these routes, in
  both directions (the walk is symmetric). `reconcile()` applies the rule of
  the two directions: a function written on one end only (the output of the
  EPP or the valve) is copied to the other end; two different functions are
  a **conflict**: both are kept, nothing is overwritten, and the generator
  prints it. `gen_example.py` uses it to write the function on each valve
  (the `function` element information, shown as a dynamic text next to the
  coil) and on the output (`functionText` of the channel), and the
  destination (`-> Y1`) in the comment of the output. In the example the
  function of `Y5` is typed on the valve only, and the generator gives it to
  channel 2 of socket `X02` of the EPP;
- folio 7 of the example: EPP output X01 channel 1 -> 1 m cable -> Y
  distributor -> coils `Y1` (`EV12`) and `Y2` (`EV13`); X02 -> 2 m cable ->
  T coupler -> coils `Y4` (`EV14`) and `Y5` (`EV15`, typed on the valve); the
  MICO output 1 -> 5 m cable -> coil `Y3` (`EV16`).

Without the setting below, the texts are computed when the example is
generated and do not change by themselves.

### Live and bidirectional (setting, off by default)

**Settings > General > "Follow the function of an M12 output along the
cables, T couplers and Y distributors"** (`project/enable-function-propagation`
in the application settings; off by default, so nothing changes until it is
ticked). It is an application setting, not a project one, because a project
option would change the `.qet` format, which stays as it is. With it on:

- editing the function of a channel of the EPP (the PLC table of the master,
  or the module properties) or the `function` of a valve gives the new text
  to the other end(s) of the chain, in both directions. One edit is one undo
  step: undo restores every end;
- the walk is made by `FunctionPropagation` (`sources/functionpropagation.cpp`),
  not by `Terminal::relatedPotentialTerminal`, which is potential based and
  does not know the channels: changing it would change the results of its
  other users. It follows the conductors, crosses the transit elements with
  their route table and keeps the channel, so channels never mix. A chain
  broken by a missing conductor, a channel without route, or two sockets
  wired directly (no transit element between) propagates nothing;
- the tables are in the data of the elements, as `kindInformation` entries of
  the `.elmt` written by `gen_m12.py` (no table in the C++):
  - `function_routes`: `X1:1>X2:1;X1:2>X2:2` (symmetric: terminal, channel
    on one side, terminal, channel on the other side);
  - `function_ends`: `X1:1` on the valve coil, the channel that carries the
    function. The channels of a PLC module come from its `plcIO` table
    (channel k of a terminal = k-th `plcIO` of that terminal);
- the rule is the one of `chain.reconcile()`: the other end empty, copy; the
  same text, nothing; a different text is a conflict, both are kept and a
  message is shown in the status bar (and `FunctionPropagation::notifier()`
  emits `conflictFound`). One addition for live edits: an end that still holds
  the value the edited end had before the edit is in sync and follows (it
  makes typing and a later change at the source follow). The text that a PLC
  symbol brings from its definition (`In-/Output A`) counts as empty. Nothing
  is ever overwritten silently; to resolve a conflict, edit the end that must
  change.

Limits: only the function (not the comment or the label) is followed; it is
followed when a function is edited, not when conductors are added or removed;
only direct conductors (not folio reports); with the setting on, a chain of
edits cannot be merged in one undo step; the M12 elements already embedded in
an old project have no route table until they are updated from the library.

## Example

`examples/murr_meanwell_example.qet` shows an NDR-240-24 feeding a
MICO Basic 4.4 with three lamps, and an SDR-120-24 feeding a MICO Pro with
two channels (the DC OK contact of the power supply enables the breaker). The
same four devices are drawn as realistic views, description views and pin
tables (same labels), then every product of the two libraries is shown on a
folio; folios 7 and 8 show the M12 links and products (see above). Folio 9
shows the layout footprints of all the power supplies at 1 px = 2 mm and
folio 10 the WDR series (the other power supplies are on folio 6). The
elements are embedded in the project.

## Sources and assumptions

Only facts are used (reference, voltage, current, number of channels,
dimensions, terminal and LED names), taken from the public datasheets of the
manufacturers: Mean Well (HDR, NDR, SDR, EDR, DRP, WDR series) and
Murrelektronik
(MICO Basic and MICO Pro). All the drawings are original vector drawings; no
photo, logo or text of a datasheet is copied. **Assumed, to check against the
datasheet of the exact reference before use:**

- the order of the terminals in the symbols is a convention (the physical
  order varies with the series, and several models have the outputs on top);
- the terminal counts and numbering come from the pin tables of the public
  data sheets. They were read for the HDR-15/30/60/100, NDR-120/480,
  EDR-75/150, DRP-480, SDR and WDR models; the HDR-150, NDR-240, EDR-120,
  DRP-240 and WDR-60 layouts were deduced from the neighbouring models;
- the SDR-960 current sharing connector (P-, P+) and its relay contact pins
  are read from the text and the picture of the data sheet; the NC contact
  (pins 7 and 8) of the SDR-480 is not drawn;
- the dimensions are those of the data sheet (width x height x depth, +/- 1
  mm); the screws of the layout footprints are evenly spread, not at their
  exact positions, and the LED and trimmer positions are generic;
- the WDR models for 5, 12 and 48 V come from the public data sheet; their
  availability on the market is not known;
- the layout scale (1 px = 2 mm) is that of the enclosure layout branch; the
  Beckhoff cards use another scale (about 2.9 px/mm);
- MICO Basic: no individual 0 V terminal per channel; the assignment of the
  channels of the 4.4.10 (2 x 1-2-3-4 A, 2 x 4-6-8-10 A) is assumed;
- MICO terminals 13 / 14 (group alarm contact) come from the MICO 2.4 / 4.4
  instructions; 23 / 24 (90 % contact) come from the MICO Pro manual and
  are assumed for the Pro elements here; the presence of 13 / 14 on the
  4.4.10, 8.6 and Pro models, and the Pro `CTRL` terminal, are assumed;
- the push button as a transparent square with a built-in LED, and the LED
  colour (green), are an interpretation of the data sheet text; the shape
  and positions are assumed;
- the Mean Well elements have no voltage adjustment range in their data, so
  no range is highlighted near the trimmer;
- text placement: QET draws the `<text>` of an element with (x, y) on the
  baseline. The generators now convert their top left box to that baseline
  (`Canvas.text`), which moves every text of the three libraries about 6
  units down compared with the earlier revisions; the factor is estimated
  for Roboto Condensed and was not checked on a rendered image;
- MICO Pro: the terminal layout, the LED and the marking `+n` / `-n` of the
  channels are assumed; there is no 8-channel Pro here;
- the numbers 13 / 14 of the "DC OK" relay contact are assumed;
- the realistic views are stylised vector drawings, not photographs;
- the input voltage range is not stated in the elements.

## Regenerating the library

```sh
python misc/murr_meanwell/gen_elements.py   # MICO and Mean Well libraries
python misc/murr_meanwell/gen_m12.py        # M12 library (18_murrelektronik_m12)
python misc/murr_meanwell/gen_example.py    # examples/murr_meanwell_example.qet
```

`gen_elements.py` holds the data of the products (`PSUS`, `MICOS`) and the
drawings, `gen_m12.py` those of the M12 products, `qetlib.py` the writer of
the element files. The output is
deterministic (uuids derived from the product names). To add a product, add a
`Psu(...)` or `Mico(...)` line (or a length in `gen_m12.py`) and run the
scripts.

## Tests

```sh
qelectrotech --check-elements extra-elements/16_murrelektronik_mico --grid
qelectrotech --check-elements extra-elements/17_meanwell_psu --grid
qelectrotech --check-elements extra-elements/18_murrelektronik_m12 --grid
ctest --test-dir build -R tst_murrmeanwellelements -j1
ctest --test-dir build -R tst_functionpropagation -j1
```

`--check-elements` must report 0 failures; the only warnings are the "no
terminal" ones of the table and layout elements. `tst_murrmeanwellelements` checks the
names of the directories, the four appearances of each product (and the
layout footprint of the power supplies), identical
terminals in the four views, the exact terminal lists of sample models, the
complete WDR series, the layout dimensions against the data sheet figures, that the drawings stay inside the frame with the
hotspot and the terminals on the grid, the rail colours, the ratings in the
data, that the example uses elements of the library, and for the M12 library
the order numbers, one terminal per connector, the data and the dashed
conductors of the example, and that the function follows the chain: the
channel routes, EPP -> T -> two valves, Y with two valves, a change at the
source, and a broken chain.
