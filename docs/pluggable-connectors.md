# Pluggable terminal blocks, terminal magnet, wire id at terminals

## Pluggable terminal blocks

`extra-elements/16_pluggable_connectors/` holds generic symbols of
pluggable (detachable) terminal blocks, installed with the common
collection ("Pluggable terminal blocks"):

- single row: 2, 3, 4, 6, 8, 9 and 12 positions;
- double row (two stacked levels): 2 x 2, 2 x 3, 2 x 4, 2 x 6 and 2 x 9
  positions, numbered 1 to 2N;
- each in two layouts: pins stacked, wires left / plug right, or `_v`
  (vertical, like the "borne-embrochable" of the model project): pins side by
  side at a 20 unit pitch, wires on top, plug underneath.

Each symbol has a common mark (the `label` element information, drawn under
the block) and numbered pins. A pin has a **wire side** terminal (west, or
north for `_v`) and a **plug side** terminal (east, or south for `_v`) with
the same `potential`, so the wire id and number carry across the plug.

The terminals are on the 10 unit grid and the plug side faces away from the
wires, so the block meets the facing terminals of a device or base (see the
Beckhoff EtherCAT symbols) with the terminal magnet.

## Terminal magnet

Settings > General > "Snap moved elements so that their terminals meet"
(`diagrameditor/terminal_magnet`, off by default).

At the drop of a move, if a free terminal of a moved element is within 12
scene units of a free terminal of another element facing it, the whole move
is corrected so both terminals meet (nearest pair wins). The correction is
part of the move: one undo takes it back. Terminals that already carry a wire
are left alone.

## Wire id at terminals

Settings > General > "Show the id of the wire connected to a terminal"
(`diagrameditor/terminal_shows_wire_id`, off by default).

Writes the text of the wire (its id) beside the terminal, outside the
element. It follows the "show terminal names" display option of the folio.

## Limits

- The magnet acts at the drop only, not while dragging, and not when a new
  element is placed from the elements panel.
- Only the first wire of a terminal is shown.
- The symbols are generic, not copied from a manufacturer drawing.
- `extra-elements/` is installed by a line shared with the Beckhoff EtherCAT
  branch.
