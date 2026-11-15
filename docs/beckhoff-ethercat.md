# Beckhoff EtherCAT symbols

A small set of symbols for drawing a Beckhoff EtherCAT architecture: a
coupler with a stack of I/O terminals, EtherCAT boxes in a daisy chain, and
AF1000 drives. They are in `extra-elements/15_beckhoff_ethercat` and are
installed into the common collection, as the category
**Beckhoff EtherCAT (generic)**.

## What is there

| Symbol | Manufacturer | Designation | Terminals |
| --- | --- | --- | --- |
| `el3xxx_generic.elmt` | Beckhoff | EL3xxx | contacts 1 to 8 |
| `el4xxx_generic.elmt` | Beckhoff | EL4xxx | contacts 1 to 8 |
| `el9011.elmt` | Beckhoff | EL9011 | none (end cap) |
| `ethercat_box_generic.elmt` | (empty) | (empty) | EtherCAT IN, EtherCAT OUT |
| `af1000_2axes_generic.elmt` | Beckhoff | AF1000 | EtherCAT IN/OUT, A1-U/V/W, A2-U/V/W, L1, L2, L3, PE, RB+, RB- |

The EK1100 coupler, and EL1008, EL2008, EL2564 and other terminals, are
already in the standard collection, under
`10_electric/20_manufacturers_articles/beckhoff`. These symbols add what is
missing there: analog terminals, the end cap, a box, and the AF1000 drive.

## What is generic

Nothing here is copied from a datasheet or a drawing of the manufacturer.

- EL3xxx and EL4xxx are drawn the same way for every type: eight numbered
  contacts. The signal on each contact differs from one type to another:
  rename the terminals after the datasheet of the terminal you use, and set
  the exact designation (for example EL3004) in the element properties.
- The AF1000 symbol puts its terminals where the connectors are on the front
  of a two-axis drive, as a layout aid. The terminal names are plain
  (A1-U, L1...). The exact type (AF1215...), its power and its current are
  left for the user to fill in.
- The box has no manufacturer and no reference.

## Using them

- Numbering the terminals on the bus: give the label of each element a
  formula such as `A%id` in the existing autonumbering (a numbering scheme
  for elements), then place the terminals in order along the bus.
- Showing a picture of the real product: the symbol editor has no picture
  part. Add the picture to the folio with the image tool, then place the
  symbol over it and move the terminals of a copy of the symbol onto the
  real connectors. Only use pictures you may use.

## Limits

- No E-bus or power contact terminals are drawn between the I/O terminals.
- The symbols are not scaled to a picture; the terminal positions of the
  drive follow the proportions of a drawing, not measurements.
