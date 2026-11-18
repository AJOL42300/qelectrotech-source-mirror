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

## Example

`examples/murr_meanwell_example.qet` shows an NDR-240-24 feeding a
MICO Basic 4.4 with three lamps, and an SDR-120-24 feeding a MICO Pro with
two channels (the DC OK contact of the power supply enables the breaker). The
same four devices are drawn as realistic views, description views and pin
tables (same labels), then every product of the two libraries is shown on a
folio. The elements are embedded in the project.

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
python misc/murr_meanwell/gen_elements.py   # both libraries
python misc/murr_meanwell/gen_example.py    # examples/murr_meanwell_example.qet
```

`gen_elements.py` holds the data of the products (`PSUS`, `MICOS`) and the
drawings, `qetlib.py` the writer of the element files. The output is
deterministic (uuids derived from the product names). To add a product, add a
`Psu(...)` or `Mico(...)` line and run both scripts.

## Tests

```sh
qelectrotech --check-elements extra-elements/16_murrelektronik_mico --grid
qelectrotech --check-elements extra-elements/17_meanwell_psu --grid
ctest --test-dir build -R tst_murrmeanwellelements -j1
```

`--check-elements` must report 0 failures; the only warnings are the "no
terminal" ones of the table views. `tst_murrmeanwellelements` checks the
names of the directories, the four appearances of each product, identical
terminals in the four views, that the drawings stay inside the frame with the
hotspot and the terminals on the grid, the rail colours, the ratings in the
data, and that the example uses elements of the library.
