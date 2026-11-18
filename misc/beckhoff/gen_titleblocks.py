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
"""Write the A3 title blocks of the titleblocks folder.

A3_landscape and A3_portrait use the layout of the title block of the
user's example project: a strip of two rows of 25 px under the folio. Row 1:
an empty slot for a logo, author, project title, file name. Row 2: date,
folio title, folio number. The widths are relative, so the strip adapts to
the folio width. No logo and no text of a company is embedded: the logo slot
is empty.

ISO7200_A3_landscape and ISO7200_A3_portrait are generic, company free
title blocks that carry the fields of ISO 7200 (legal owner, document number,
title, supplementary title, document type, status, language, sheet, creator,
approver, date of issue, revision) in a strip of four rows of 17 px. Fields
without a QElectroTech variable (owner, document number, ...) are read from
the custom fields of the folio (%{owner}, %{docnumber}, ...).

The matching folio sizes (see FOLIO_SIZES) are A3 at about 2.94 px per mm:
a folio is cols * colsize + 20 px wide and rows * rowsize + 20 px + the
height of its title block tall.

Usage: python misc/beckhoff/gen_titleblocks.py
"""
import os
from xml.sax.saxutils import escape

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT_DIR = os.path.join(ROOT, "titleblocks")

#   name: (columns, column width, rows, row height), in px
FOLIO_SIZES = {
    "A3 landscape": (20, 61, 10, 80),
    "A3 portrait": (14, 61, 14, 83),
    "A3 ISO 7200 landscape": (20, 61, 10, 78),
    "A3 ISO 7200 portrait": (14, 61, 14, 82),
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


ISO_ROWS = "17;17;17;17;"

# name -> (template name, columns). The first column holds the logo, the
# others are relative so that the strip adapts to the folio width.
ISO_TEMPLATES = {
    "ISO7200_A3_landscape": ("A3 ISO 7200 landscape",
                             "100px;r25%;r25%;r25%;r25%;"),
    "ISO7200_A3_portrait": ("A3 ISO 7200 portrait",
                            "80px;r25%;r25%;r25%;r25%;"),
}

ISO_FIELDS = {
    "owner": {"en": "Legal owner", "fr": "Propri\u00e9taire l\u00e9gal",
              "de": "Rechtlicher Eigent\u00fcmer",
              "es": "Propietario legal", "it": "Proprietario legale"},
    "docnumber": {"en": "Document number", "fr": "N\u00b0 de document",
                  "de": "Dokumentnummer", "es": "N\u00famero de documento",
                  "it": "Numero del documento"},
    "title": {"en": "Title", "fr": "Titre", "de": "Titel",
              "es": "T\u00edtulo", "it": "Titolo"},
    "subtitle": {"en": "Supplementary title",
                 "fr": "Titre compl\u00e9mentaire", "de": "Zusatztitel",
                 "es": "T\u00edtulo complementario",
                 "it": "Titolo supplementare"},
    "doctype": {"en": "Document type", "fr": "Type de document",
                "de": "Dokumentart", "es": "Tipo de documento",
                "it": "Tipo di documento"},
    "status": {"en": "Status", "fr": "Statut", "de": "Status",
               "es": "Estado", "it": "Stato"},
    "language": {"en": "Language", "fr": "Langue", "de": "Sprache",
                 "es": "Idioma", "it": "Lingua"},
    "sheet": {"en": "Sheet", "fr": "Feuille", "de": "Blatt",
              "es": "Hoja", "it": "Foglio"},
    "author": {"en": "Created by", "fr": "\u00c9tabli par",
               "de": "Erstellt von", "es": "Creado por",
               "it": "Creato da"},
    "approver": {"en": "Approved by", "fr": "Approuv\u00e9 par",
                 "de": "Gepr\u00fcft von", "es": "Aprobado por",
                 "it": "Approvato da"},
    "date": {"en": "Date of issue", "fr": "Date d'\u00e9mission",
             "de": "Ausgabedatum", "es": "Fecha de emisi\u00f3n",
             "it": "Data di emissione"},
    "revision": {"en": "Revision", "fr": "R\u00e9vision", "de": "Revision",
                 "es": "Revisi\u00f3n", "it": "Revisione"},
    "format": {"en": "Format", "fr": "Format", "de": "Format",
               "es": "Formato", "it": "Formato"},
}

# (field, value, column, row, extra columns, extra rows); the value of the
# sheet field is the same in every language, the others are variables
ISO_LAYOUT = [
    ("owner", "%{owner}", 1, 0, 1, 0),
    ("docnumber", "%{docnumber}", 3, 0, 1, 0),
    ("title", "%title", 1, 1, 1, 0),
    ("subtitle", "%{subtitle}", 3, 1, 1, 0),
    ("doctype", "%{doctype}", 1, 2, 0, 0),
    ("status", "%{status}", 2, 2, 0, 0),
    ("language", "%{language}", 3, 2, 0, 0),
    ("sheet", "%{folio-id} / %{folio-total}", 4, 2, 0, 0),
    ("author", "%author", 1, 3, 0, 0),
    ("approver", "%{approver}", 2, 3, 0, 0),
    ("date", "%date", 3, 3, 0, 0),
    ("revision", "%indexrev", 4, 3, 0, 0),
]


def iso_field(name, value, col, row, colspan, rowspan, align="left"):
    attrs = (f'valign="center" hadjust="true" name="{name}" '
             f'displaylabel="true" align="{align}" col="{col}" row="{row}"')
    if colspan:
        attrs += f' colspan="{colspan}"'
    if rowspan:
        attrs += f' rowspan="{rowspan}"'
    labels = "".join(
        f'<translation lang="{k}">{escape(v)}</translation>'
        for k, v in ISO_FIELDS[name].items())
    return (f'<field {attrs}><value><translation lang="en">'
            f"{escape(value)}</translation></value>"
            f"<label>{labels}</label></field>")


def iso_template_xml(title, cols, size):
    fields = [iso_field(*f) for f in ISO_LAYOUT]
    # the logo slot spans the first three rows, the format sits under it
    fields.append('<field valign="center" hadjust="true" name="logo" '
                  'displaylabel="false" align="center" col="0" row="0" '
                  'rowspan="2"><value/><label/></field>')
    fields.append(iso_field("format", size, 0, 3, 0, 0, "center"))
    return (f'<titleblocktemplate name="{title}">\n'
            "<information>ISO 7200 title block for an A3 folio. The first "
            "column is left empty for a logo; the owner, document number, "
            "supplementary title, document type, status, language and "
            "approver are read from the custom fields of the folio "
            "(owner, docnumber, subtitle, doctype, status, language, "
            "approver).</information>\n"
            f'<logos/>\n<grid cols="{cols}" rows="{ISO_ROWS}">\n' +
            "\n".join(fields) + "\n</grid>\n</titleblocktemplate>\n")


def main():
    for filename, (title, cols) in TEMPLATES.items():
        path = os.path.join(OUT_DIR, filename + ".titleblock")
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            f.write(template_xml(title, cols))
        print("wrote", os.path.relpath(path, ROOT))
    for filename, (title, cols) in ISO_TEMPLATES.items():
        path = os.path.join(OUT_DIR, filename + ".titleblock")
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            f.write(iso_template_xml(title, cols, "A3"))
        print("wrote", os.path.relpath(path, ROOT))


if __name__ == "__main__":
    main()
