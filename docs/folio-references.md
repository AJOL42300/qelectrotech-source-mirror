# Folio references (going and coming arrows)

A folio reference joins a wire that stops at the edge of one folio to the
wire that goes on at another folio. The interface says "Sheet reference"
(French: "Renvoi de folio"); the code says *report*.

Each end shows where the other end is, as folio number and position
(for example `2-D3`: folio 2, column 3, row D). The text follows the
"Folio report" label formula of the project, `%f-%l%c` by default.

The example `examples/folio_references_example.qet` has two folios: a wire
leaves terminal `-X1` of folio "Feed" through a going arrow, and comes back
on folio "Load" through a coming arrow to lamp `-H1`.

## The two arrows

| Arrow | Name in the interface | Element file | Link type |
| --- | --- | --- | --- |
| Going (outgoing) | Going arrow / Folio suivant | `02going_arrow.elmt` | `next_report` |
| Coming (incoming) | Coming arrow / Folio précédent | `01coming_arrow.elmt` | `previous_report` |

Both are in the common collection, folder
`10_electric/10_allpole/100_folio_referencing`. A going arrow links only to a
coming arrow, and the other way round. Each arrow takes one wire and one
link.

## Steps

1. On the first folio, right-click an empty part of the folio. The menu has
   a submenu "Sheet reference" ("Renvoi de folio"). Choose the going arrow:
   it is placed where you clicked.
2. Draw the wire from the device to the arrow terminal, as for any wire.
3. On the other folio, do the same with the coming arrow, and draw the wire
   from the arrow terminal to the next device.
4. Select one of the two arrows. Its properties panel has a "Sheet
   reference" tab ("Report de folio") with the free arrows of the other
   kind: wire number, function, folio number, position and folio title.
5. Right-click the wanted row and choose "Link the item". In the live edit
   mode of the panel the link is made at once; otherwise the row turns green
   and the link is made when the panel is applied.
6. Both arrows now show the folio and position of the other end. The link
   is one step of the undo stack of the project.

To undo the link, select an arrow and use the unlink button of the same tab
(the tab then shows the linked arrow, with a button to show it).

## Checked and not checked

Checked in the code (`sources/diagramview.cpp`,
`sources/ui/linksingleelementwidget.cpp`,
`sources/undocommand/linkelementcommand.cpp`):

- the submenu, the two files it always offers, and the placement at the
  click position;
- that only a going arrow and a coming arrow, both free, can be linked;
- that the link goes through `LinkElementCommand` on the undo stack.

Checked on the example (`--export-png`): the going arrow of "Feed" shows
`2-D3` and the coming arrow of "Load" shows `1-E12`.

The example was made with the scripting interface (`--run`), which uses the
same link command as the panel. The clicks of steps 1 and 5 were read in the
code and not made in the running interface.
