# References between the representations of a component

A component can be drawn several times in a project: a Beckhoff card
`A1.10` as a schematic symbol, as a realistic view, as a description and as
a table row. When the project asks for it, each symbol tells where the
other representations are:

```
A1.10
→ folio 3 (realistic), folio 5 (table)
```

Clicking a reference goes to that folio and selects the symbol, as a cross
reference does.

## What already existed

* The parent (master) symbol shows its children (slaves) in a cross
  reference (`sources/qetgraphicsitem/crossrefitem.*`). Nothing told where
  the *same* component is drawn elsewhere.
* `sources/duplicatelabels.*` lists labels used twice. The notion of "same
  component" (`place` + `key()` of the displayed label) comes from there.

## Settings

Project properties, *Show where a component is drawn elsewhere* (off by
default: an existing project looks the same):

* style: compact (one line) or detailed (one line per reference, with the
  position on the folio);
* position: under the symbol or on its right;
* *Keep the information of the symbols of a component the same*
  (independent of the display, off by default), see below.

Stored in the project as `<duplicate_references enabled="true"
style="compact|detailed" placement="below|right"
sync_information="true"/>`, written only when different from the default.
Older versions ignore it.

Each symbol can hide its references: *Hide the references to the other
representations* in its properties (`hide_duplicate_references`).

## What counts as the same component

Symbols of the same label, on the same structure (`place`), which are
simple or parent symbols. Children, reports and terminals are not
representations: they already have their own cross reference. A symbol
excluded from the bill of materials, or only a drawing aid, is ignored.
The appearance is recognised from the file name of the symbol
(`_realistic`, `_description`, `_table`, else schematic).

## Giving a label already used

In the properties of a symbol, a hint under the label says where the label
is already drawn. When the setting is on and the part numbers
(manufacturer reference) differ, the user is asked to confirm: it is
probably another component. The question is not asked in live editing,
which applies each keystroke; the hint is.

The references of every symbol sharing the label are recomputed at once,
also when the label changes through undo/redo, an automatic numbering, a
paste, a deletion or a move to another folio: the index follows the
label-change signal of the symbols, no code is duplicated.

## Synchronised information

When the project option is on, the symbols of a component (same label)
share their information fields, in both directions:

* editing a shared field on one symbol gives the value to the others;
* giving a symbol the label of a component already drawn completes the
  empty fields on both sides;
* when both sides have different non-empty values, a non-blocking question
  says which fields differ: keep the values of this symbol (the others are
  overwritten), take the values of the others, or cancel. It is not asked
  in live editing (the label is given without changing the others' values);
* all of it is one undo step (`ChangeElementInformationCommand` over all
  the symbols, or a macro when a numbering or a freeze is applied too).

Shared (the article list): designation (article number), description,
manufacturer, order number (`manufacturer_reference`), internal number,
supplier, quantity, unity, and the same fields of the auxiliary articles
(`auxiliary*`, `*_auxiliary*`); also the comment, notes, model, category,
voltage and current ratings, dimensions, unit price and labour time.

Not shared: the label, its formula and its identifier; the plant (`=`) and
the location (`+`), which tell which component a label is (the same label
in another plant or location is another component, as for the duplicate
labels detection); the function (it is followed through the wiring), the
device type, the terminal and conductor texts, the PLC data, the custom
properties, the options (hide, exclude from the bill of materials,
freeze). The position, the folio and the title block are not information
fields: they are never touched.

Limit: the article list being shared, the bill of materials still counts the
quantity of every symbol of a component (a card drawn four times is counted
four times). A symbol with "exclude from the bill of materials" is not a
component for the references, so it is neither linked nor synchronised.

Children (slaves) of a parent are not representations of it and keep their
own information; the function stays managed by its own propagation.


* `ComponentReferenceIndex` (`sources/componentreferences.*`), one per
  project, groups the symbols by label and refreshes only the groups that
  changed (no scan of the whole project at each change).
* `DuplicateRefItem` (`sources/qetgraphicsitem/duplicaterefitem.*`), child
  of the symbol, is part of the "cross references" items: it is hidden with
  them, printed, exported to PNG/SVG and linked in the PDF.
* The card tooltip lists the references too.

## Limits

* Only the folios already built are indexed (the project builds its folios
  lazily).
* The DXF export does not draw the references.
* Conflict detection compares part numbers only.
* Placing, pasting or auto-numbering a symbol does not synchronise by
  itself: it does so when the label is then given through the properties
  editor (the index and the references do follow every label change).
  The same API (`ComponentReferenceIndex::planSync()`/`syncCommand()`) can
  be called from those paths.

Tests: `tests/qttest/tst_componentreferences.cpp`.
