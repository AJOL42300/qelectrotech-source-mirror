#!/usr/bin/env python3
# Copyright 2006-2026 The QElectroTech Team
# This file is part of QElectroTech.
#
# QElectroTech is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# QElectroTech is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
"""Write titleblocks/A3_landscape.titleblock and A3_portrait.titleblock.

Both templates use the layout of the title block of the user's example
project: a strip of two rows of 25 px under the folio. Row 1: an empty slot
for a logo, author, project title, file name. Row 2: date, folio title,
folio number. The widths are relative, so the strip adapts to the folio
width. No logo and no text of a company is embedded: the logo slot is empty.

The matching folio sizes (see FOLIO_SIZES) are A3 at about 2.9 px per mm.

Usage: python misc/beckhoff/gen_titleblocks.py
"""
import os
from xml.sax.saxutils import escape

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT_DIR = os.path.join(ROOT, "titleblocks")

FOLIO_SIZES = {
    "A3 landscape": (20, 61, 10, 80),
    "A3 portrait": (14, 61, 15, 80),
}

TEMPLATES = {
    "A3_landscape": ("A3 landscape", "100px;r25%;r50%;r25%;"),
    "A3_portrait": ("A3 portrait", "100px;r30%;r40%;r30%;"),
}

LABELS = {
    "author": {"en": "Author", "fr": "Auteur", "de": "Autor", "es": "Autor",
               "it": "Autore"},
    "title": {"en": "Title", "fr": "Titre", "de": "Titel", "es": "T\u00edtulo",
              "it": "Titolo"},
    "file": {"en": "File", "fr": "Fichier", "de": "Datei",
             "es": "Archivo", "it": "File"},
    "date": {"en": "Date", "fr": "Date", "de": "Datum", "es": "Fecha",
             "it": "Data"},
    "folio": {"en": "Folio", "fr": "Folio", "de": "Blatt", "es": "Hoja",
              "it": "Foglio"},
}

FOLIO_VALUE = {"en": "%{folio-id} of %{folio-total}",
               "fr": "%{folio-id} sur %{folio-total}"}


def field(name, col, row, value, align="left", label=True, fontsize=None,
          values=None):
    attrs = (f'valign="center" hadjust="true" name="{name}" '
             f'displaylabel="{"true" if label else "false"}" '
             f'align="{align}" col="{col}" row="{row}"')
    if fontsize:
        attrs += f' fontsize="{fontsize}"'
    values = values or {"en": value}
    vals = "".join(f'<translation lang="{k}">{escape(v)}</translation>'
                   for k, v in values.items())
    labels = ""
    if name in LABELS:
        labels = "".join(
            f'<translation lang="{k}">{escape(v)}</translation>'
            for k, v in LABELS[name].items())
    return (f"<field {attrs}><value>{vals}</value>"
            f"<label>{labels}</label></field>")


def template_xml(title, cols):
    fields = [
        field("author", 1, 0, "%author"),
        field("title", 2, 0, "%projecttitle", "center", False),
        field("file", 3, 0, "%savedfilename"),
        field("date", 1, 1, "%date"),
        field("", 2, 1, "%title", "center", False, 9),
        field("folio", 3, 1, "", values=FOLIO_VALUE),
    ]
    return (f'<titleblocktemplate name="{title}">\n'
            f"<information>Two-row strip for an {title} folio. The first "
            "cell is left empty for a logo.</information>\n"
            f'<logos/>\n<grid cols="{cols}" rows="25;25;">\n' +
            "\n".join(fields) + "\n</grid>\n</titleblocktemplate>\n")


def main():
    for filename, (title, cols) in TEMPLATES.items():
        path = os.path.join(OUT_DIR, filename + ".titleblock")
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            f.write(template_xml(title, cols))
        print("wrote", os.path.relpath(path, ROOT))


if __name__ == "__main__":
    main()
