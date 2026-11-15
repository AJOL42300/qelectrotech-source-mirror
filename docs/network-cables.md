# Network cables

A network cable is an ordinary conductor with three extra properties:
cable type, length and branch. Nothing changes for conductors that do not
use them: the new attributes are written to the project file only when a
cable type is set, so old projects open and are saved as before.

## Use

Select one or more conductors, right-click, **Network cable**:

- pick a type: Ethernet (RJ45), EtherCAT (RJ45), EtherCAT P (M8, red and
  black), M12 4, 5 or 8 pins, M12 Y cable, motor cable (no connector);
- **Length...** (in metres) and **Designation...** (for example
  `4 G 1.5 mm²`);
- **Branch** (1) / (2) for the two legs of a Y cable;
- **Totals...** lists cables, connectors and metres per type for the project;
- **Remove the network cable** clears the three properties.

Every change is one undoable step.

## Drawing

The conductor takes the sheath colour of the type, and a plug symbol is drawn
at each end. A label shows the branch, designation and length. Where an end
sits on a folio report element, no plug is drawn and the text `Folio N` is
written instead, N being the folio of the other end of the report.

## Moulded cables and Y cables

A moulded cable (island to island, box to box) is one conductor: type, length,
connectors included, no strands. Do not wire it strand by strand.
A Y cable is two conductors from the same box terminal, with branch (1)
and (2).

## Totals

Counts are per conductor. When a cable continues on another folio through a
folio report, give the type to both segments but the length to one only,
otherwise the cable and its metres are counted twice.

## Existing element-based cables

Cables drawn as elements with one terminal per strand (for example
`cablereseau4p`, `cable-moteur`, `rj45_renvoi_folio`) keep working, as the
element loading is unchanged. Their designation and length are static text, so
they are not editable nor totalled. To migrate by hand, replace the cable
element and its plug elements by one network conductor. Do not keep the plug
elements, or the plugs would be drawn twice.

## Servo motors

`extra-elements/16_servo_motor` has two generic elements, a servo motor with
encoder and a servo motor with resolver: power terminals U V W PE, feedback
terminals and shield, label, designation and description (put the motor cable
and its length there). The standard motor of the element library is not
duplicated.

## Cable catalogue

One catalogue holds the cables of three manufacturers, each row with a
manufacturer field: LAPP for motor, power and control cables, Beckhoff for the
EtherCAT cables (green RJ45, black with red stripe for EtherCAT P) and
Murrelektronik for moulded M12 and Y cables for valves. Network cable >
`Catalogue cable...` asks for a usage (motor, servo, control, ethernet,
ethercat, ethercat_p, m12, y_m12), then a cable. It fills the reference, the
designation, the section and, for a made-up cable, the length of the selected
conductors, in one undo step. A cable type already chosen is kept, otherwise the
type of the usage is set (a control cable gets the plain type `Cable`). The
reference is saved in the conductor as `network_reference`, only when set.
`Totals...` lists the cables and metres per reference.

The built-in rows are examples, plain facts read on the manufacturer sites in
November 2026: two LAPP cables (1119304 ÖLFLEX CLASSIC 110 4G1,5 and 0027969
ÖLFLEX SERVO FD 796 CP), Beckhoff ZK1090-9191 (RJ45, green) and ZK7000-0102
(EtherCAT P, M8) in five lengths each, and four Murrelektronik cables
(7000-40041-2150500, 7000-40021-6340500, 7000-40701-0130200,
7000-42401-6360060). Check them against the data sheet before ordering.

Your own rows go in `cable_catalog.json` of the application configuration
folder, outside the project files. A row with the same manufacturer and
reference replaces a built-in one.

```json
{"version": 1, "cables": [{"manufacturer": "LAPP", "reference": "1119304",
  "designation": "4G1,5", "family": "OELFLEX CLASSIC 110", "conductors": 4,
  "pairs": 0, "section": "1,5", "diameter": "7,2", "shield": "",
  "sheath": "PVC", "color_code": "numbers", "usage": "control",
  "length": ""}]}
```

`Import catalogue (CSV)...` adds the rows of a CSV file exported from a
manufacturer catalogue (Excel: save as CSV). The delimiter (`;` `,` or tab),
UTF-8 or Latin 1, and quoted fields are detected. Columns are found by their
header in English, German or French, for example `Artikelnummer`, `Article
number` or `Reference` (required), `Bezeichnung`, `Adernzahl`, `Querschnitt`,
`Außendurchmesser`, `Hersteller`. Rows without a reference are skipped, and a
missing usage is guessed from the designation. The core colour code is stored as
text only.

## Limits

- Connectors are drawn symbols, not bill-of-materials elements.
- No propagation across folio reports or potentials: select each conductor.
- The cable does not fill the motor cartouche by itself.
- No automatic migration of element-based cables.
- The catalogue reads CSV only, not Excel files, and was not tried on a real
  export of a manufacturer. The colour code is not linked to the wire colours.
- Beckhoff green M8 cables (ZK1090-3131) are not listed: the green Ethernet
  type draws RJ45 plugs.
