# Element size and the manufacturers catalogue example

## Size of the manufacturer symbols

The schematic elements generated for the manufacturers (Beckhoff, Murr,
Mean Well, Kinco, Schneider, LANBAO, Pilz, ...) were drawn about three to
five times larger than the conventional symbols of the QElectroTech
collection. `misc/scale_elements.py` scales them by 0.5 on each axis:

```
python misc/scale_elements.py extra-elements
```

- Every connection point is moved onto the 10 px grid, with at least 10 px
  between two of them, and the lead lines follow.
- The insertion point is the connection point nearest to the centre, and
  the frame (width, height, hotspot) lies on the grid. `qelectrotech
  --check-elements extra-elements --grid` reports no grid warning.
- Static texts are scaled by 0.7, dynamic texts keep their font.
- Implantation elements (layouts, footprints, `ide-enclosures`), pin
  tables (`_table`) and elements without drawing are left as they are.
- `misc/scale_elements_manifest.txt` stores the hash of every scaled file:
  running the script again skips them. After running a generator in
  `misc/<brand>/` (which writes full-size elements), run the script again.

## Catalogue example

`examples/manufacturers_catalog_example.qet` has one set of folios per
brand and per category (one row per product, with its views). Write it
again with:

```
python misc/catalog_example.py
qelectrotech --resave examples/manufacturers_catalog_example.qet out.qet
```

After changing elements, refresh the definitions embedded in the other
examples with `python misc/update_embedded_elements.py <file.qet>...`,
and re-save every example with `--resave` so that it is in the format
written by the program.
