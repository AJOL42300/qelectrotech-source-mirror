# Pictures in a folio: stretch, crop, rotation, opacity, storage

All of this is non-destructive: the picture file on disk is never changed,
and the original pixels stay in the project, so a crop or a transparent colour
can be changed or removed later.

## Stretch

Width and height are scaled independently (properties panel in %, or the
resize handles). The lock button keeps the proportions; "Restore proportions"
makes the height match the width again. Saved as `<transform scaleX= scaleY=>`.

## Crop

- Context menu or double click: crop on the folio, or the "Crop..." dialog.
- Properties panel: "Crop left / top / right / bottom", in pixels of the
  original picture. Each field is applied when editing finishes, as one undo
  step ("Crop an image"), and can be widened again later.
- Saved as `<crop x y w h>` next to `<image_base>` (the whole original).

## Rotation

- Free angle in degrees (panel, or rotate handle).
- "Rotate 90° clockwise / counterclockwise": context menu and the `+90°` /
  `-90°` buttons of the panel, one undo step each. The crop follows the
  rotated picture, since it is a part of the picture itself.

## Opacity and transparency

- Opacity, 0 to 100 %: applied by `paint()`, so it also reaches printing and
  the PDF / PNG / SVG exports. Saved as the optional `opacity` attribute,
  written only when below 100 %.
- "Transparency" check box, per picture:
  - checked: the picture is stored as PNG, with its alpha channel;
  - unchecked: the picture is stored as a smaller JPEG (transparent pixels
    flattened on white).
  It is checked and locked while a transparent colour is set.
- "Transparent color...": opens the existing dialog, where colours are picked
  on the picture and each has a tolerance (0 to 100 %) with a live preview.
  Colours and tolerances are saved in `<transparent_colors>`; such a picture
  is always stored as PNG. The existing alpha of the picture is kept and
  combined with the colour key.

## Storage format (Configuration > General > Pictures)

"Convert inserted pictures to JPEG" is on by default; the JPEG quality is 85 %.

| Picture inserted                  | Stored in the project                |
|-----------------------------------|--------------------------------------|
| JPEG                              | as it is, never recompressed         |
| PNG, BMP, TIFF, GIF, WebP... opaque | JPEG, flattened on white             |
| with at least one transparent pixel | PNG (the alpha is not lost silently) |
| SVG                               | rasterised, stored as PNG            |

The conversion is done when the picture is inserted (dialog, drag and drop,
"Replace image"). A picture loaded from a file is not converted again: PNG
stays PNG and JPEG stays JPEG. A picture created by code keeps PNG.

## File compatibility

There is no new element. Old files, without `opacity` or JPEG data, load
exactly as before. The JPEG bytes are in the same `<image>` text as PNG
used to be; older versions decode it with `QImage::fromData()`, and ignore
the `opacity` attribute.

## Tests

`tests/qttest/tst_imagestretchcrop.cpp`: stretch, crop and undo, rotation
and undo, opacity (saved, and reaching the painter), old file, PNG / JPEG
storage, JPEG not recompressed, transparent colour and tolerance.
