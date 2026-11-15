# Interactive technical dossier (HTML export)

QElectroTech can export a project as a folder of HTML pages: the
*interactive technical dossier*. The folder opens with a double click on
`index.html`. It needs no web server and fetches nothing from the network.

## Producing it

- Menu: **File > Export the interactive technical dossier**. The dialog asks
  for the destination folder, an optional overview picture (SVG), an optional
  function map, and whether to write a PDF with bookmarks. The overview and
  the PDF choice are remembered.
- Command line:

```sh
qelectrotech --export-html project.qet out_dir \
    [--html-overview machine.svg[#anchor]] \
    [--html-map functions.json|functions.csv] \
    [--html-folios "1,3-5"] [--html-no-pdf] [--show-terminals]
```

`--html-folios` exports only the listed folios (numbered from 1); links to
the others are shown as plain text. The PDF always contains every folio.

## What is in the folder

| File | Content |
|---|---|
| `index.html` | Home: the overview picture, the list of functions, the table of folios |
| `folio-NN.html` / `.svg` | One folio: the SVG made by the SVG export, plus a transparent layer of links and tooltips |
| `nomenclature.html` | Bill of materials (grouped devices) |
| `equipment.html`, `terminals.html`, `cables.html`, `wires.html` | Tables of the project database |
| `io.html` | The I/O tables (PLC addresses) |
| `search.html` | Search through labels, texts and references of the whole dossier |
| `dossier.pdf` | The whole project, with bookmarks |
| `function-map.json` | The function -> folios map that was used (a model to edit) |
| `data/*.js` | The data the pages read (scripts, because `fetch` is refused on `file://`) |

Every table has a filter box (searches in all columns) and sortable columns.
The URL `nomenclature.html?q=text` (same for the other tables) opens a table already filtered. Folio pages have
previous/next buttons (also the left and right arrow keys), a zoom (buttons,
Ctrl + wheel) and a "Folios" link back to the table of folios.

### In a folio

- Hover an **element** or a **terminal**: the tooltip shows the label,
  the terminal name, the signal or function, the I/O address (when the terminal
  is a PLC channel), the id, the colour and the section of the connected wire.
  A click keeps the details in a panel.
- **Folio references** (reports, folio-tri, arrows) and **cross-references**
  (contacts of a coil) are links to the folio they point to; hovering them
  gives the destination.
- An element has a link to its **nomenclature** row, and the row lists the
  folios where the device is used.

## The overview and the function map

`--html-overview` takes an SVG, copied as `overview.svg` (the original is never
changed; a small script is added to the copy). A `#fragment` after the path
is the view shown first. The page shows it in an `<iframe>`: the views
(`:target`, `<view>`) of the SVG work as in a browser, and the home page
follows them.

Functions are read from the SVG links whose class is `function-link` and
whose `aria-label` is `"01 - 001 TRF1 - Title"` (id, machine tag, title).

The folios of a function come, in this order of priority, from:

1. the file given with `--html-map`;
2. `<project>.functions.json` or `.csv` beside the project file;
3. a deduction: the folios whose texts (labels, element information, wire
   texts, titles) mention the function's machine tag the most, at most six per
   function. Spaces, case and `*` are ignored. Codes such as `001 TRF1` are
   also looked for inside longer tags. A folio that names more than a third
   of the functions (a general folio) is dropped, unless nothing else is left.

Map file formats (the folios are numbered from 1):

```json
{"functions": {"01": {"folios": [18, 25]}, "02": "30-32"}}
```

```csv
function;folios
01;18,25
02;30-32
```

Without an overview, the home page has only the table of folios.

## PDF

`dossier.pdf` is the output of `--export-pdf`: the folios, with the
cross-reference links of QElectroTech, plus a **bookmark per folio**
("N - title"). The bookmarks are added by `PdfLinks::addOutline()`, an
incremental update appended to the file written by Qt (Qt has no outline
API). If the layout of that file is not the expected one, the PDF is kept
as it was, without bookmarks.

## Measures

On a project of 58 folios (70 MB, Release with debug info, offscreen):
loading 40-70 s, HTML only 206 s in total, with the PDF 309 s in total
(236 s for the export itself), peak memory about 800 MB, about 105 MB of
output (PDF 20 MB). 27 of the 29 functions of the overview got a folio.

## Limits

- The deduction of the functions is a guess based on text: check
  `function-map.json` and correct it with a map file.
- The hotspots are drawn from the geometry of the elements: a rotated
  element has a rectangle around its bounds, not its exact shape.
- Elements which are not in the database (no device data) are not in the
  nomenclature.
- Memory: the folios are built one after the other but are not unloaded; a
  big project needs the memory of the project fully loaded.
- A project without PLC data, terminal strips or cables gives almost empty
  I/O, terminals and cables pages. Wire ids are empty when the conductors
  have no text.
- The tooltips, search and filters were checked in the tests by their data,
  not with an automated browser.
- The PDF holds all the folios even with `--html-folios`.

## Tests

`tests/qttest/tst_dossierfunctionmap.cpp` (map parsing, SVG functions,
deduction) and `tests/qttest/tst_htmldossier.cpp` (the export and the PDF
bookmarks, through the binary).
