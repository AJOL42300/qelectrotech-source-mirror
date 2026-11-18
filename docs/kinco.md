# Kinco KS105 PLCs and HMI Green panels

Symbols, layout footprints and an example for the Kinco **KS105** PLCs (with
their KS expansion modules) and the Kinco **HMI Green** operator panels
(GL and GH series). The symbols are in
`extra-elements/10_electric/20_manufacturers_articles/kinco` (the
`elements/` submodule is not modified).

## Interpretation of the range

- "KS105" is read as the **Kinco KS105 series**: KS105-16DT, KS105C1-16DT,
  KS105C2-16DT, plus the **KS-series expansion modules** that the KS105
  drives (KS121-16DX, KS122-12XR, KS122-14DT, KS123-14DR, KS133-06IV,
  KS131-04RD).
- "HMI Green" is read as the **Kinco GREEN series**: GL (Basic) GL043,
  GL043E, GL070, GL070E, GL100, GL100E, GL104E, GL150E and GH (Enhanced)
  GH043, GH043E, GH070, GH070E, GH104E, GH150E.

That is 23 products. The data comes from the public Kinco site (product
pages) and the GL070E manual only; what could not be verified is listed in
[Uncertain data](#uncertain-data).

## Content

| Folder | Content |
| --- | --- |
| `ks105_cpu` | the 3 KS105 CPUs |
| `ks_expansion` | the 6 expansion modules |
| `hmi_green_gl` | the 8 GL panels |
| `hmi_green_gh` | the 6 GH panels |
| `implantation` | layout footprints |

Each product has its own folder (`ks105_16dt/`) with up to four appearances:

- the **schematic** (`ks105_16dt.elmt`): symbol with one terminal per real
  connection point;
- the **realistic** view (`_realistic`): redrawn front with the terminal
  labels;
- the **description** view (`_description`): the realistic view with the
  description of the I/O and ports;
- the **table** (`_table`): an I/O or port table without terminals.

The HMIs have no I/O: their terminals are the power plug (FG, 0V, 24V) and
the communication ports (`COMx.p` for the DB9 pins, `ETH`, `USB.HOST`,
`USB.SLAVE`).

Terminal colours: +24 V and L+ red, 0 V and L- blue, PE / FG orange.
Dimensions are multiples of 10 and the terminals are on the grid of 10.

Each element carries the manufacturer (Kinco), the exact reference, the
designation, the dimensions (width/height/depth in mm) and the **articles**
of the Parts tab (the connectors or the terminal blocks, described factually
because no public order number could be verified).

## Layout footprints

`implantation/` holds a front view at real size (1 px = 2 mm, like the other
libraries): `<ref>_fp` for the PLCs and modules (DIN rail 35 mm), and for the
HMIs `<ref>_fp_face` (bezel) and `<ref>_fp_cutout` (door cut-out). The label
is a dynamic text.

## Example

`examples/kinco_example.qet` (16 folios): a KS105C2-16DT with its expansion
modules, a GL070E panel, the 24 V supply, RS485, Ethernet and CAN links, the
realistic, description and table views, a layout folio and six catalogue
folios with all the products.

## Regenerating the library

```sh
python misc/kinco/gen_kinco.py      # library, from misc/kinco/kinco_data.py
python misc/kinco/gen_example.py    # examples/kinco_example.qet
```

The generators are reproducible (same output on each run). The example
generator reuses `misc/beckhoff/gen_example.py` and
`misc/beckhoff/gen_titleblocks.py`. To change a reference or a dimension,
edit `misc/kinco/kinco_data.py` and run both scripts again.

## Tests

`tests/qttest/tst_kincoelements.cpp` (`tst_kincoelements`) checks the product
list, the translated directories, the XML, the references (identical in the
four views), the channels against the terminals, the grid and the bounding
box, the table, the colours, the articles, the footprint scale and the
example.

## Uncertain data

- Terminal labels and layout of the expansion modules: logical names only.
- 19 or 20 terminals on the KS105 I/O block, and the top/bottom split.
- Expansion bus RJ45 pinout (one `EXP` terminal), and whether C1/C2 have it.
- CAN2 / port RJ45 of the C2 and the port numbering.
- Connector part numbers (none verified) and what is supplied.
- DB9 pins of COM1/COM3 on the GH; the DB9 pinout is verified on the GL070E
  only; COM2 is limited to pins 2, 3 and 5.
- HMI terminal block pinout for the models other than GL070E.
- GL070E depth (33 to 37.2 mm, 34 mm used) and the KS105 depth (75.5 or
  84.5 mm, 84.5 mm used).
- Realistic proportions and colours are generic.
- Digital input type (sink only or NPN/PNP).
- Module numbering, the KS101M-04DX (not covered), only RUN/ERR LEDs drawn.
- GL consumption and voltage ranges.
