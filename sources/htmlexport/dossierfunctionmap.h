/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef DOSSIERFUNCTIONMAP_H
#define DOSSIERFUNCTIONMAP_H

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

/**
	The link between the functions of a machine overview picture (a badge
	"01 - 001 TRF1 - Chain 1") and the folios of a project that describe
	them, for the HTML technical dossier (see docs/html_export.md).

	Only QtCore is used, so that it can be tested alone.
*/
namespace DossierFunctionMap {

	/// One function of the overview picture.
	struct Function {
		QString id;      ///< "01": what the zoom view is named after
		QString anchor;  ///< "zoom-fonction-01": the view of the function
		QString tag;     ///< "001 TRF1": the machine tag of the function
		QString title;   ///< free text after the tag
		QList<int> hint; ///< folios the picture itself names, from 1
	};

	/// "01" and "1" are the same function: digits lose leading zeros.
	QString canonicalId(const QString &id);

	/// folio numbers (from 1) of each canonicalId() of a function
	using Map = QMap<QString, QList<int>>;

	/**
		The functions an overview SVG offers: each link of class
		"function-link" whose aria-label reads "id - tag - title". The
		folios the picture names with data-electrical-ref and
		data-source-folios are added as hints to the function of the same tag.
	*/
	QList<Function> functionsFromSvg(const QByteArray &svg);

	/// A tag in a comparable form: no space, upper case, no "*".
	QString normalizeTag(const QString &tag);

	/// "1,3-5 8" -> 1 3 4 5 8, without what is out of 1..max, sorted, unique.
	QList<int> parseFolioList(const QString &text, int max);

	/**
		A map written by hand. JSON: {"functions": {"01": {"folios":
		[18, 25]}}}, or {"01": [18, 25]}, or {"01": "18,25-27"}. CSV: one
		"function;folios" per line (";", "," or tab between the two, a
		header line allowed, "#" starts a comment).
		@return false, with @p error set, when the text is neither.
	*/
	bool parse(const QByteArray &data, int folio_count, Map *map,
			   QString *error);

	/// The map as the JSON file --html-map reads back.
	QByteArray toJson(const Map &map, const QList<Function> &functions);

	/**
		Guess the folios of each function from the texts of the folios.
		@p folio_texts has the texts of folio n (labels, texts, wire
		numbers, titles) at index n - 1. A function goes to the folios which
		mention its tag the most, at most @p max_folios of them; its hints
		count as a mention. Functions with no folio are left out.
	*/
	Map deduce(const QList<Function> &functions,
			   const QList<QStringList> &folio_texts, int max_folios = 6);
}

#endif // DOSSIERFUNCTIONMAP_H
