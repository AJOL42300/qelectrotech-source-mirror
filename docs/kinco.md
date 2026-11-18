# Kinco KS105 PLCs and HMI M2 panels

Symbols, layout footprints and an example for the Kinco **KS105** PLCs (with
their KS expansion modules) and two Kinco **M2 series** operator panels
(M2070HE and M2100E). The symbols are in
`extra-elements/10_electric/20_manufacturers_articles/kinco` (the
`elements/` submodule is not modified).

## Interpretation of the range

- "KS105" is read as the **Kinco KS105 series**: KS105-16DT, KS105C1-16DT,
  KS105C2-16DT, plus the **KS-series expansion modules** that the KS105
  drives (KS121-16DX, KS122-12XR, KS122-14DT, KS123-14DR, KS133-06IV,
  KS131-04RD).
- The HMI range is limited to the **M2070HE** and **M2100E**. "M070E" does
  not exist as a Kinco reference; it is read as **M2070HE** (M2 series,
  7 inch, Ethernet). The MT070E is therefore not included. The other M2
  variants (M2043H(E), M2070H, M2070HE-WIFI, M2100, M2100E-WIFI) are not
  covered.

That is 11 products. The data comes from the public Kinco sites and
datasheets; what could not be verified is listed in
[Uncertain data](#uncertain-data).

### HMI Green series removed

The first version of the library also had the HMI Green series (GL043,
GL043E, GL070, GL070E, GL100, GL100E, GL104E, GL150E and GH043, GH043E,
GH070, GH070E, GH104E, GH150E: 14 panels, two folders, 28 footprints). It
was removed from the collection on request. Nothing is lost: the files are
in the git history (commit `ebc5bb965`), and the data is kept as
`HMI_GREEN_ARCHIVED` in `misc/kinco/kinco_data.py`.

## Content

| Folder | Content |
| --- | --- |
| `ks105_cpu` | the 3 KS105 CPUs |
| `ks_expansion` | the 6 expansion modules |
| `hmi_m2` | the 2 M2 panels |
| `implantation` | layout footprints |

Each product has its own folder (`ks105_16dt/`) with up to four appearances:

- the **schematic** (`ks105_16dt.elmt`): symbol with one terminal per real
  connection point;
- the **realistic** view (`_realistic`): redrawn front with the terminal
  labels;
- the **description** view (`_description`): the realistic view with the
  description of the I/O and ports;
- the **table** (`_table`): an I/O or port table without terminals.

The three KS105 CPUs have a fifth view, the **DIN rail view** (`_din`): the
PLC standing vertically on a horizontal DIN rail, with the front column of
19 screw terminals (1L+, I0.0 to I0.7, 2L-, 2L+, Q0.0 to Q0.7), the 3-pole
power plug (PE, 0V, 24V), the 2-pole RS485 plug (B, A, PORT1) and the RJ45
sockets with their pins. The terminals are those of the schematic, with the
same names. The pitch is enlarged for readability, so it is not to scale.
The faces that carry the plugs and the sockets are **assumed** (top and
bottom).

The HMIs have no I/O: their terminals are the power plug (24V, 0V, FG) and
the communication ports (`COMx.p` for the DB9 pins, `COM1.1`/`COM1.2` for
the 2-pole RS485 plug, `ETH`, `USB.HOST`, `USB.SLAVE`).

Terminal colours: +24 V and L+ red, 0 V and L- blue, PE / FG orange.
Dimensions are multiples of 10 and the terminals are on the grid of 10.

Each element carries the manufacturer (Kinco), the exact reference, the
designation, the dimensions (width/height/depth in mm) and the **articles**
of the Parts tab (the connectors or the terminal blocks, described factually
because no public order number could be verified).

## Panel cut-out of the HMIs

The cut-out is written **width x height** in the description, the facts and
the layout footprint.

| HMI | Cut-out (W x H) | Front (W x H x D) | Status | Source |
| --- | --- | --- | --- | --- |
| M2070HE | 192 x 138 mm | 204 x 150 x 37.2 mm | verified | Kinco flyer `https://dl.kinco-automation.com/HMI/manual/M2/Flyer_M2070HE.pdf`, official page `https://kincoautomation.ru/en/product/automation/m2/mass-series-2-m2-m2070he-11860` |
| M2100E | 260 x 179 mm | 271 x 191 x 36 mm | verified | official page `https://kincoautomation.ru/en/product/automation/m2/mass-series-2-m2-m2100e-11863` (3 concordant sources) |

## Layout footprints

`implantation/` holds a front view at real size (1 px = 2 mm, like the other
libraries): `<ref>_fp` for the PLCs and modules (DIN rail 35 mm), and for the
HMIs `<ref>_fp_face` (bezel) and `<ref>_fp_cutout` (door cut-out, a dotted
rectangle with its size "W x H mm" written inside). The label is a dynamic
text.

## Example

`examples/kinco_example.qet` (16 folios): a KS105C2-16DT with its expansion
modules, an M2070HE panel, the 24 V supply, RS485, Ethernet and CAN links,
the realistic, description, table and vertical DIN rail views, a layout folio
and catalogue folios with all the products.

## Regenerating the library

```sh
python misc/kinco/gen_kinco.py      # library, from misc/kinco/kinco_data.py
python misc/kinco/gen_example.py    # examples/kinco_example.qet
```

The generators are reproducible (same output on each run). They do not
delete stale files. The example generator reuses
`misc/beckhoff/gen_example.py` and `misc/beckhoff/gen_titleblocks.py`. To
change a reference or a dimension, edit `misc/kinco/kinco_data.py` and run
both scripts again.

## Tests

`tests/qttest/tst_kincoelements.cpp` (`tst_kincoelements`) checks the product
list, the translated directories, the XML, the references (identical in the
four views), the channels against the terminals, the grid and the bounding
box, the table, the colours, the articles, the footprint scale, the vertical
DIN rail view, the cut-out (values, "W x H" form, Green series absent) and
the example.

## Uncertain data

- HMI pinouts (power plug, COM1, DB9 COM0/COM2): taken from the MT070 / M2070
  and GL070E manuals; **assumed identical on the M2100E**.
- M2100E front: 271 x 191 x 36 mm (official) against 280 x 193 x 36 mm (a
  reseller, probably with the seal). M2070HE depth: 37 or 37.2 mm (37.2 used).
- "M070E" read as M2070HE; the other M2 variants are not covered.
- Terminal labels and layout of the expansion modules: logical names only;
  they have no DIN rail view.
- Expansion bus RJ45 pinout (one `EXP` terminal).
- Which faces carry the 3-pole and 2-pole plugs and the RJ45 sockets of the
  KS105 (assumed top and bottom); the DIN rail view is not to scale.
- Connector part numbers (none verified) and what is supplied.
- Port naming follows the KS105 datasheet (PORT0, PORT1, CAN2 / PORT0).
- Realistic proportions and colours are generic.
- Module numbering, the KS101M-04DX (not covered), only RUN/ERR LEDs drawn.
- `--check-elements` was not run: the application could not be built here
  (the `elements` submodule path is too long for the full configure).
