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
#ifndef HTMLDOSSIER_H
#define HTMLDOSSIER_H

#include <QString>
#include <QStringList>
#include <functional>

class QETProject;

/**
	The interactive technical dossier: a folder of HTML pages made from a
	project, opened with a double click and needing no server. See
	docs/html_export.md.
*/
namespace HtmlDossier {

	struct Options {
		/// A picture (SVG) of the whole machine whose badges lead to the
		/// folios of their function; copied into the dossier, never changed
		/// in place. Empty: the home page has only the table of folios.
		QString overviewSvg;
		/// A function -> folios map written by hand (see DossierFunctionMap
		/// ::parse()). Empty: <project>.functions.json / .csv beside the
		/// project, then what the labels of the folios say.
		QString mapFile;
		/// Folios to export, "1,3-5", from 1. Empty: all of them.
		QString folios;
		/// Also write dossier.pdf with bookmarks
		bool pdf = true;
		bool showTerminals = false;
		/// Called with (done, total, what) as the work goes on
		std::function<void(int, int, const QString &)> progress;
	};

	struct Result {
		bool ok = false;
		QString error;
		int folios = 0;
		qint64 milliseconds = 0;
		QStringList warnings;
	};

	/// The file name of the page of folio @p number (from 1) of @p count
	QString folioPageName(int number, int count);

	Result exportDossier(QETProject &project, const QString &dir,
						 const Options &options);
}

#endif // HTMLDOSSIER_H
