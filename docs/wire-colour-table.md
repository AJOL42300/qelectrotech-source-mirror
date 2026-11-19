# Wire colour table (wire colour meaning table)

A **wire colour entry** gives a function (for example "+24 V DC") a drawing
colour, an optional second colour (stripe), a wire colour code (IEC 60757),
a section in mm² and a label. The profiles of a project form its **wire
colour meaning table**.

## Using a profile

- **Toolbar**: the "Wire colour" list applies the chosen profile to the
  selected conductors. It is in the default "Diagram" toolbar, after the
  conductor colour button; if your toolbar was customised, add it from
  *Settings → Configure QElectroTech → Toolbars* (item "Wire colour").
- **Context menu**: right-click one or more selected conductors →
  *Wire colour*.
- One choice is one undo step. It sets the line colour (and stripe), the
  section, the wire colour code, the function (profile name) and the text
  (label, shown). A conductor that belongs to a cable keeps its own colour
  code and section.
- The colour of the profile also becomes the colour of the next conductors
  drawn (the same as the conductor colour button). Section and label are
  not carried over.
- Conductors connected at the same potential as a selected one follow, as
  with the conductor colour button.

## Editing the table

*Project → Project properties → Wire colour table*: add, remove or edit lines
(function, colour code, colour, stripe colour, section, label) or restore
the default table. The table is saved in the `.qet` file as a
`<wirecolourtable>` element, only when it differs from the default table. A
project without that element uses the default table, so old projects open
and save as before.

## Legend

*Project → Add → Add a wire colour legend* adds a text with
Colour | Section | Function for each line of the table. It is a copy: it
is not updated when the table changes (add a new one, delete the old one).

## Default table

The default lines come from the colours used in a real sample schematic.
That schematic holds only the drawn colours (no section, function or type
data): the functions below are therefore **assumptions** to be validated.
IEC 60204-1 §13.2.4 is used where it gives a meaning (see sources).

| Function | Colour | Section | Label | Basis |
|---|---|---|---|---|
| +24 V DC | red | 0.75 mm² | +24V | assumed |
| 0 V DC | blue | 0.75 mm² | 0V | assumed (IEC: blue for DC control) |
| Protective earth (PE) | green-yellow | | PE | IEC 60204-1 |
| Power circuit L1 | black | | L1 | IEC (black: power); phase colour assumed |
| Power circuit L2 | brown | | L2 | assumed |
| Power circuit L3 | grey | | L3 | assumed |
| Neutral (N) | light blue | | N | IEC 60204-1 |
| AC control circuit | red | 1 mm² | | IEC 60204-1 (section assumed) |
| Circuit from an external source | orange | | | IEC 60204-1 |
| Analogue signal | violet | | | assumed |
| Bus (EtherCAT) | yellow | | | assumed |
| White / pink / yellow / green / light grey wire | as named | | | sample colours, no meaning |
| White-grey, white-green, white-yellow, brown-grey, brown-green, yellow-brown, yellow-red, black-red wire | two colours | | | sample colours, no meaning |

IEC 60204-1 advises against green or yellow alone where they could be
confused with green-yellow; the "Green wire" and "Yellow wire" lines are
kept because they appear in the sample.

## Limits

- No propagation of a profile along a potential, no import/export of the
  table.
- The legend is a static text.
- The table is per project; there is no application-wide table.

## Sources

- IEC 60204-1 §13.2.4 as summarised in public articles:
  <https://industrialmonitordirect.com/blogs/knowledgebase/european-control-panel-wire-colour-codes-per-iec-60204-1>,
  <https://ladx.ai/resources/control-panel-wire-colour-codes>
  (black power, red AC control, blue DC control, orange circuits from an
  external source, green-yellow protective, blue neutral). The standard
  itself was not read.
