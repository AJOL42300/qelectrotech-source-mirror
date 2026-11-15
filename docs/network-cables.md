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

## Limits

- Connectors are drawn symbols, not bill-of-materials elements.
- No propagation across folio reports or potentials: select each conductor.
- The cable does not fill the motor cartouche by itself.
- No automatic migration of element-based cables.
