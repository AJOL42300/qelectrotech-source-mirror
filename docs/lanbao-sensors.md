# Lanbao sensors

Three sensors of the Shanghai Lanbao Sensing Technology range, built on the
same principle as the [Murrelektronik and Mean Well library](murr-meanwell.md).
The symbols are in `extra-elements/19_lanbao_sensors`, installed into the
common collection as the category **Lanbao sensors** (translated in
`qet_directory`: English, French, German).

| Folder | Reference | Type |
| --- | --- | --- |
| `photoelectric/psm_bc100dpb_e2` | PSM-BC100DPB-E2 | M18 diffuse photoelectric sensor, 100 cm, PNP, NO/NC |
| `inductive/lr18xcf12dpoy_e2` | LR18XCF12DPOY-E2 | M18 inductive sensor, quasi-flush, Sn 12 mm, PNP, NO |
| `laser_distance/pdb_cc50tgi` | PDB-CC50TGI | Laser distance sensor 80-500 mm, 4-20 mA and switch output |

A product folder holds four **appearances**, named like the folder:
`X.elmt` (schematic), `X_realistic`, `X_description` and `X_layout`. The first
three have the **terminals at the same positions and with the same names**,
so the views can be swapped on a folio without redrawing the wires. They are
simple elements: a sensor has no pin table, so there is no `X_table` and no
master link.

- **schematic**: the sensor symbol inside a dotted outline, with the +24 V
  rail in red, the 0 V rail in blue and the output marked with its type
  (PNP, NO/NC) and the LEDs;
- **realistic**: redrawn vector view (side view of the M18 barrels with their
  nuts, the front of the laser sensor with its display and keys). It is an
  original drawing, not a photo and not a manufacturer drawing;
- **description**: the realistic view with legends and leader lines;
- **layout**: footprint at **1 px = 2 mm** for the cabinet layout, drawn to
  the dimensions of the data sheet (the depth is in the description, "Lanbao
  PDB-CC50TGI, 51x65x23 mm (width x height x depth)"). The same figures are
  in `extra-elements/19_lanbao_sensors/devices_layout.json`.

## Terminals

| Reference | Terminal | Function | Colour |
| --- | --- | --- | --- |
| PSM-BC100DPB-E2 | `1` (M12 pin 1) | +24 V | BN |
| | `2` (M12 pin 2) | NO/NC selector (open or + = NO, 0 V = NC) | WH |
| | `3` (M12 pin 3) | 0 V | BU |
| | `4` (M12 pin 4) | output | BK |
| LR18XCF12DPOY-E2 | `1` (M12 pin 1) | +24 V | BN (supposed) |
| | `3` (M12 pin 3) | 0 V | BU (supposed) |
| | `4` (M12 pin 4) | output | BK (supposed) |
| PDB-CC50TGI | `BN` | +24 V | BN |
| | `BU` | 0 V | BU |
| | `BK` | switch output Q | BK |
| | `GY` | analog output 4-20 mA | GY |

## Detection type and distance

| Reference | Detection type | Detection distance | Status |
| --- | --- | --- | --- |
| PSM-BC100DPB-E2 | Diffuse reflection photoelectric, infrared 940 nm | 100 cm, adjustable with a single-turn potentiometer; stated for a 90 % white card of 20 x 20 cm, the real distance depends on the target | verified (data sheet) |
| LR18XCF12DPOY-E2 | Inductive, metal targets, quasi-flush mounting | nominal Sn 12 mm, assured Sa 0-9.6 mm; standard target Fe 36 x 36 x 1 mm | verified (data sheet) |
| PDB-CC50TGI | Laser distance measurement, red laser 650 nm class 2 | measuring range 80-500 mm (full scale 420 mm), 4-20 mA over the range; data given for a 90 % white card | verified (data sheet) |

The correction factors of the LR18X for metals other than steel are not in the
data sheet and are not given. The measuring principle of the PDB is not named
in the data sheet and is not stated. The figures are shown:

- in the `comment` and `description` fields of every appearance;
- on the schematic, under the output type ("Diffuse / 100 cm",
  "Inductive / Sn 12 mm", "Laser 80-500 mm"), and under the realistic and
  description views;
- in the function text of the inputs of the example.

The layout view only gives the dimensions. The insertion point of the three
electrical views is the +24 V terminal (the grid check of `--check-elements
--grid` wants it on a connection point).

The elements carry the manufacturer (`Lanbao`), the reference, a French
designation and an English description with the ratings (supply 10-30 V DC,
or 12-24 V DC for the PDB, switching current, range, protection, IP67).

## Sources

Public data sheets of the manufacturer, redrawn and summarised (no text or
picture copied):

- PSM: <https://www.cnlanbaosensor.com/uploads/PSM-Diffise-reflection-100cm-M12-connector.pdf>
- LR18X: <https://www.cnlanbaosensor.com/uploads/LR18XC-Y-DC-3-E2.pdf>
- PDB: <https://www.cnlanbaosensor.com/uploads/laser-distance-measuring-PDB-CC50-series.pdf>

## Assumed, to check

- The wire colours of the LR18X are not published; BN/BK/BU are assumed.
- The positions of the nuts, collar, LEDs and potentiometer of the PSM, and
  the nut width, are read from the drawing and approximate.
- The PDB mounting holes are read from the drawing (about 1 mm); the sizes of
  its optical windows are estimated. The data sheet draws only the 4-20 mA
  variant: the switching mode of Q (PNP, NPN or push-pull) is settable, so the
  symbol shows a generic output. The cable shield is not drawn.
- "E2" is read as an M12 four-pin connector (stated for the PSM and its
  plastic twin PSS, "M12 connector" for the LR18X). The PDB has a 2 m cable.
- The PSM data sheet is titled "PSS series"; it is the metal-housing PSM
  that is drawn (M18x1, 47.2 mm with connector).

## Regenerating

The elements are generated, not drawn by hand:

```sh
python misc/lanbao/gen_sensors.py       # extra-elements/19_lanbao_sensors
python misc/lanbao/gen_lanbao_example.py  # examples/lanbao_example.qet
```

The generator checks that every drawing stays in the frame of its element and
that the terminals are on the 10 px grid. The test `tst_lanbaoelements`
checks the generated files.

## Example

`examples/lanbao_example.qet` has two folios. The first wires a Mean Well
HDR-60-24 supply to the three sensors: +24 V and 0 V to each sensor, the PSM
and LR18X outputs to inputs 1 and 2 of a Beckhoff EL1008 card, and the PDB
analog output to an analog input card. The PDB switch output and the PSM
selector are left unconnected. The second folio shows the realistic,
description and layout views. The conductors are routed by QElectroTech when
the project is loaded: check the routing visually.
