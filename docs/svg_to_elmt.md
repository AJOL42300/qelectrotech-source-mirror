# Making an element from a SVG drawing

`qelectrotech --svg-to-elmt` turns a SVG drawing, typically a picture of a
device from a manufacturer, into an element (`.elmt`). The element keeps the
look of the drawing, and gets the terminals you give it.

```
qelectrotech --svg-to-elmt in.svg out.elmt [--name TEXT] [--lang CODE]
             [--width PX | --height PX | --scale FACTOR]
             [--terminal x,y[,n|e|s|w[,name]]]...
             [--terminals file.csv]
```

Then open `out.elmt` in the element editor and move the terminals with the
mouse if they are not exactly on the connectors.

## Giving the terminals

The SVG does not say where the terminals are. There are three ways, which can
be mixed:

- `--terminal 120,40,n,L1` once per terminal. The coordinates are the ones of
  the SVG; the orientation is the side the wire leaves by (`n`, `e`, `s`,
  `w`); without it the side of the drawing nearest to the terminal is used.
- `--terminals file.csv`: one terminal per line, `name;x;y;orientation`
  (`;`, `,` or a tab separate the fields; the orientation can be left out).
  A header line and lines starting with `#` are skipped.
- In the SVG itself (for example drawn over the picture in Inkscape): a shape
  whose `id` starts with `terminal` is not drawn, and marks a terminal at the
  centre of its bounding box. `terminal-L1` gives the terminal `L1`; set
  `data-orientation="n"` to choose the side.

## What is converted

Rectangles, circles, ellipses, lines, polylines, polygons and paths (lines,
curves and arcs) become parts of the element, in the order of the SVG, with
`transform`, `viewBox`, `<use>`, `style` and `class`. A gradient becomes its
average colour.

An element part can only use the colours of a fixed palette: each colour of
the SVG becomes the nearest one. Opacity is blended with white.

The element is centred on its insertion point, and its frame is rounded up to
the grid of 10.

## What is not converted

- Pictures (`<image>`): a photograph cannot become an element. Draw or
  trace it as a vector drawing first, or put it on the folio with the image
  tool.
- Texts, clip paths, masks and filters.
- The holes of a compound path are filled.

The command prints a warning for what it leaves out, and when no terminal
was given.
