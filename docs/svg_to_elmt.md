# Making an element from a SVG drawing or an image

`qelectrotech --svg-to-elmt` turns a SVG drawing or a picture (PNG, JPEG,
BMP, GIF, WebP), typically a drawing of a device from a manufacturer, into an
element (`.elmt`). The element keeps the look of the drawing, and gets the
terminals you give it. The same conversion is in the element editor, in
**File > Import a SVG drawing or an image**, and with **Paste** (`Ctrl+V`)
when the clipboard holds an image.

```
qelectrotech --svg-to-elmt in.svg|in.png out.elmt [--name TEXT] [--lang CODE]
             [--mm-scale PX | --cells N | --width PX | --height PX
              | --scale FACTOR]
             [--terminal x,y[,n|e|s|w[,name]]]...
             [--terminals file.csv] [--no-snap]
             [--dpi N] [--threshold 1-255] [--invert] [--simplify PX]
             [--despeckle PX] [--max-paths N]
```

Then open `out.elmt` in the element editor and move the terminals with the
mouse if they are not exactly on the connectors.

## Size

One scale for the SVG and for pictures. The first of these that is given wins:

| Option | Meaning |
| --- | --- |
| `--scale F` | multiply the units of the drawing by `F` |
| `--width PX`, `--height PX` | drawing of this size in element pixels |
| `--cells N` | drawing `N` grid cells (10 px each) wide |
| `--mm-scale PX` | `PX` element pixels per millimetre of the drawing |

Without any of them, `--mm-scale 2` is used: 1 mm = 2 px. The millimetres of a
picture come from its resolution, `--dpi` (96 by default); those of a SVG
from its units (96 per inch).

The frame of the element is a multiple of 10 around its insertion point (the
middle of the drawing), and the terminals are moved to the nearest point of
the grid of 10, so that wires connect on the folio. `--no-snap` keeps them
where they are given.

## Tracing a picture

A picture is traced into outlines: a pixel is drawn when it is darker than
`--threshold` (128 by default; `--invert` draws the lighter ones, and a
transparent pixel counts as white). This suits line drawings, logos and
scans; it does not turn a photograph into something useful.

- `--simplify PX` (1 by default) merges the points of an outline closer than
  `PX` to a straight line; 0 keeps every pixel step.
- `--despeckle PX` (2) ignores outlines of this area or smaller.
- `--max-paths N` (500) keeps the `N` largest outlines and warns about the
  others.

## Giving the terminals

The drawing does not say where the terminals are. There are three ways, which
can be mixed:

- `--terminal 120,40,n,L1` once per terminal. The coordinates are the ones of
  the SVG, or the pixels of the picture; the orientation is the side the wire
  leaves by (`n`, `e`, `s`, `w`); without it the side of the drawing nearest
  to the terminal is used.
- `--terminals file.csv`: one terminal per line, `name;x;y;orientation`
  (`;`, `,` or a tab separate the fields; the orientation can be left out).
  A header line and lines starting with `#` are skipped.
- In the SVG itself (for example drawn over the picture in Inkscape): a shape
  whose `id` starts with `terminal` is not drawn, and marks a terminal at the
  centre of its bounding box. `terminal-L1` gives the terminal `L1`; set
  `data-orientation="n"` to choose the side.

## In the element editor

- **Import a SVG drawing or an image** asks for the size (pixels per
  millimetre, or a width or a height in grid cells) and, for a picture, the
  tracing options, then replaces the content of the element.
- **Paste** with an image, a SVG drawing or image files on the clipboard
  (for example a screenshot or a picture copied from a web page) asks the
  same, then lets you choose where to put the result, as for *Paste in
  area*; it is added to the element and can be undone. The clipboard that
  holds an element copied from an editor is pasted as before.

## What is converted

Rectangles, circles, ellipses, lines, polylines, polygons and paths (lines,
curves and arcs) become parts of the element, in the order of the SVG, with
`transform`, `viewBox`, `<use>`, `style` and `class`. A gradient becomes its
average colour.

An element part can only use the colours of a fixed palette: each colour of
the SVG becomes the nearest one. Opacity is blended with white.

## What is not converted

- Pictures inside a SVG (`<image>`), and photographs: no outline of a
  photograph is worth keeping. Use a clean line drawing or a SVG.
- Colours and shades of a picture: a traced picture is black and white.
- Texts, clip paths, masks and filters.
- The holes of a compound path are filled. A traced picture draws a hole as
  a white polygon on top of the area, so it hides what is behind it.
- The terminals are not detected.

The command prints a warning for what it leaves out, and when no terminal
was given.
