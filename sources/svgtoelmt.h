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
#ifndef SVGTOELMT_H
#define SVGTOELMT_H

#include <QByteArray>
#include <QList>
#include <QPointF>
#include <QString>
#include <QStringList>

/**
	@brief Convert a SVG drawing (typically a manufacturer's picture of
	a device) to an element definition (.elmt).

	The drawing keeps its look: each shape becomes a line, rectangle,
	circle, ellipse or polygon part, in the order of the SVG, with its fill
	and stroke colours mapped to the nearest colour an element part can
	have. The terminals are not drawn by the SVG: they are given by the
	caller or marked in the SVG (see convert()).

	Not converted: pictures (<image>), texts, clip paths, masks, filters,
	opacity (it is blended with white), the holes of a compound path.

	Usage from the command line:
	  qelectrotech --svg-to-elmt <input.svg> <output.elmt>
	      [--name <text>] [--lang <code>]
	      [--width <px> | --height <px> | --scale <factor>]
	      [--terminal x,y[,n|e|s|w[,name]]]...
	      [--terminals <file.csv>]
*/
namespace SvgToElmt {

	/// A terminal to put on the element, in the coordinates of the SVG.
	struct Terminal {
		QString name;
		QPointF position;
		/// "n", "e", "s" or "w"; empty: the side of the drawing nearest to it
		QString orientation;
	};

	struct Options {
		/// Name of the element
		QString name;
		/// Language code of the name
		QString language = QStringLiteral("en");
		/// Size of the drawing in element pixels. At most one is used, the
		/// first non-zero of scale, width and height; none: the SVG units
		/// are the element pixels. (An element pixel is a tenth of the
		/// grid step of a folio.)
		double scale = 0;
		double width = 0;
		double height = 0;
		/// Added to the terminals the SVG marks with an id starting
		/// with "terminal"
		QList<Terminal> terminals;
	};

	struct Result {
		bool ok = false;
		QString error;
		QStringList warnings;
		QByteArray definition;
		int parts = 0;
		int terminals = 0;
	};

	/**
		@brief Convert @p svg to an element definition.

		A shape of the SVG whose id starts with "terminal" (circle, rectangle,
		path...) is not drawn: it marks a terminal at the centre of its
		bounding box. The name of the terminal is the id without that prefix
		and its separators ("terminal-L1" gives "L1"); its orientation is
		the attribute data-orientation (n, e, s or w) or the side of the
		drawing nearest to it.

		The element is centred on its insertion point, and its frame is
		rounded up to the grid of 10.
	*/
	Result convert(const QByteArray &svg, const Options &options);

	/**
		@brief Parse "x,y[,orientation[,name]]", the argument of --terminal.
		@return false if @p text is not that.
	*/
	bool parseTerminal(const QString &text, Terminal *terminal);

	/**
		@brief Read terminals from text, one per line: name, x, y and
		optionally the orientation, separated by ";", "," or a tab. Empty
		lines, lines starting with # and a header line are skipped.
		@param errors receives the lines that are not terminals.
	*/
	QList<Terminal> parseTerminals(const QString &text, QStringList *errors);

	/**
		@brief Run --svg-to-elmt, @p args being what follows the flag.
		@return process exit code (0 on success).
	*/
	int run(const QStringList &args);

}

#endif // SVGTOELMT_H
