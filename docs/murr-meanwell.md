# Murrelektronik MICO and Mean Well power supplies

Symbols for the 24 V DC side of a control cabinet: Murrelektronik MICO
electronic circuit breakers and Mean Well DIN-rail power supplies. They are
built on the same principle as the [Beckhoff library](beckhoff-ethercat.md).
The symbols are in `extra-elements/16_murrelektronik_mico` and
`extra-elements/17_meanwell_psu`, installed into the common collection as the
categories **Murrelektronik MICO electronic circuit breakers** and
**Mean Well DIN-rail power supplies 24 V** (translated in `qet_directory`:
English, French, German).

Each product has its own folder, below a folder for its family:

| Folder | Products |
| --- | --- |
| `16_murrelektronik_mico/mico_basic` | MICO Basic 2.4, 4.4, 4.4.10, 8.6 |
| `16_murrelektronik_mico/mico_pro` | MICO Pro 1, 2 and 4 channels |
| `17_meanwell_psu/hdr` | HDR-15, 30, 60, 100 (24N), 150 |
| `17_meanwell_psu/ndr` | NDR-120, 240, 480 |
| `17_meanwell_psu/sdr` | SDR-75, 120, 240, 480, 960 |
| `17_meanwell_psu/edr` | EDR-75, 120, 150 |
| `17_meanwell_psu/drp` | DRP-240, 480 |

All are 24 V versions. A product folder holds four **appearances**, named like
the folder: `X.elmt` (schematic), `X_realistic`, `X_description` and
`X_table`. As for Beckhoff, the four are masters with the **terminals at the
same positions and with the same names**, so the views can be swapped on a
folio without redrawing the wires.

- **schematic**: the internal wiring inside a dotted outline. The +24 V rail
  is red, the 0 V rail blue, PE yellow-green (dashed). A power supply shows
  the AC/DC converter and, for the SDR-120 and above, the normally open
  "DC OK" relay contact. A MICO shows the +24 V bus, one current-limited
  channel (I>) per output and the control logic;
- **realistic**: stylised front view with the terminal markings, the LED, the
  adjustment potentiometer (power supplies) or the push button and current
  selector of each channel (MICO);
- **description**: the realistic view with legends and leader lines for the
  terminals and the LED;
- **table**: a simple table `N° pin | Label | ID`.

## Terminals

| Product | Terminals |
| --- | --- |
| Power supply | top: L, N, PE (no PE on the HDR); bottom: +V, -V; 13 and 14 (relay, SDR-120 to SDR-960) |
| MICO Basic | +24V, 0V, ON (enable input), ALARM (group alarm output), OUT1...OUTn |
| MICO Pro | +24V, 0V, CTRL (control input), ALARM, OUT1...OUTn and 0V1...0Vn |

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
| `m12_t_coupler` | T coupler, M12 male 5-pole input and 2 x M12 female 4-pole, LEDs: `7000-41135-0000000` |
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

- the cable 7000-40021 is 4-pole in the datasheet (only 4 pins are drawn); the
  free connectors are 4-pole (12481 from the datasheet, 12601 by analogy);
  only the T coupler input is 5-pole;
- the position of the pins (1 top left, 2 top right, 3 bottom right,
  4 bottom left, 5 centre, seen from the front of the male connector) and the
  colours of the conductors follow the usual M12 A-coded convention and the
  wire order given in the datasheet;
- Y distributor 7000-42421: the M12 is drawn male (EPP side) and the two valve
  connectors as sockets with contacts 1, 2 and PE; the datasheet of the 42421
  was not found (only the 42401 family), so the contact assignment is assumed;
- the LEDs of the T coupler are drawn without function and its internal wiring
  (distribution function) is not drawn;
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
- `misc/murr_meanwell/chain.py` follows the conductors with these routes;
  `gen_example.py` uses it to write the function of the source on each valve
  (the `function` element information, shown as a dynamic text next to the
  coil) and the destination (`-> Y1`) in the comment of the output;
- folio 7 of the example: EPP output X01 channel 1 -> 1 m cable -> Y
  distributor -> coils `Y1` (`EV12`) and `Y2` (`EV13`); X02 -> 2 m cable ->
  T coupler -> coils `Y4` (`EV14`) and `Y5` (`EV15`); the MICO output 1 ->
  5 m cable -> coil `Y3` (`EV16`).

Limits: the texts are computed when the example is generated. They do not
change by themselves when the function of an output is edited in the
application, and the intermediate parts show nothing. Making it live needs a
small C++ change, proposed here and not done: a generic pass-through step in
`Terminal::relatedPotentialTerminal` that, for an element with a per-terminal
channel map, continues through its other connector; the coil `function`
text and the EPP tooltip would read the result. It must be an option of the
settings dialog, off by default, and does not change the `.qet` format.

## Example

`examples/murr_meanwell_example.qet` shows an NDR-240-24 feeding a
MICO Basic 4.4 with three lamps, and an SDR-120-24 feeding a MICO Pro with
two channels (the DC OK contact of the power supply enables the breaker). The
same four devices are drawn as realistic views, description views and pin
tables (same labels), then every product of the two libraries is shown on a
folio; folios 7 and 8 show the M12 links and products (see above). The
elements are embedded in the project.

## Sources and assumptions

Only facts are used (reference, voltage, current, number of channels,
terminal and LED names), taken from the public datasheets of the
manufacturers: Mean Well (HDR, NDR, SDR, EDR, DRP series) and Murrelektronik
(MICO Basic and MICO Pro). All the drawings are original vector drawings; no
photo, logo or text of a datasheet is copied. **Assumed, to check against the
datasheet of the exact reference before use:**

- the order of the terminals in the symbols is a convention (the physical
  order varies with the series);
- MICO Basic: no individual 0 V terminal per channel; the assignment of the
  channels of the 4.4.10 (2 x 1-4 A, 2 x 4-10 A) is assumed;
- MICO Pro: the terminal layout, the LED and the marking `+n` / `-n` of the
  channels are assumed; there is no 8-channel Pro here;
- the numbers 13 / 14 of the "DC OK" relay contact are assumed;
- the current sharing terminals of the SDR-960 are not modelled;
- the realistic views are stylised, not dimensionally exact (only the width
  of the power supplies follows the datasheet);
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
```

`--check-elements` must report 0 failures; the only warnings are the "no
terminal" ones of the table views. `tst_murrmeanwellelements` checks the
names of the directories, the four appearances of each product, identical
terminals in the four views, that the drawings stay inside the frame with the
hotspot and the terminals on the grid, the rail colours, the ratings in the
data, that the example uses elements of the library, and for the M12 library
the order numbers, one terminal per connector, the data and the dashed
conductors of the example, and that the function follows the chain: the
channel routes, EPP -> T -> two valves, Y with two valves, a change at the
source, and a broken chain.
