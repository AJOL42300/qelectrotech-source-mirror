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

#include "rastervectorizer.h"

#include <QByteArray>
#include <QImage>
#include <QList>
#include <QPointF>
#include <QString>
#include <QStringList>

class QMimeData;

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

	A picture (PNG, JPG...) is traced first (see RasterVectorizer): it is
	good for line drawings and logos, not for photographs.

	Usage from the command line:
	  qelectrotech --svg-to-elmt <input.svg|input.png|https://address>
	      <output.elmt>
	An https address is fetched once (see PictureUrl): a picture, or a
	product page that shows one.
	      [--name <text>] [--lang <code>]
	      [--mm-scale <px per mm> | --cells <width in grid cells>
	       | --width <px> | --height <px> | --scale <factor>]
	      [--no-snap] [--dpi <n>]
	      [--threshold <0-255>] [--invert] [--simplify <px>]
	      [--despeckle <px>] [--max-paths <n>]
	      [--terminal x,y[,n|e|s|w[,name]]]...
	      [--terminals <file.csv>]
	See docs/svg_to_elmt.md.
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
		/// Width of the drawing in cells of the grid (10 element pixels):
		/// used when none of scale, width and height is
		double widthInCells = 0;
		/// Element pixels per millimetre of the drawing, used when none
		/// of the above is. 0: the units of the drawing are the pixels.
		double pixelsPerMm = 0;
		/// Size of a unit of the drawing, in millimetres: a SVG unit is
		/// a 96th of an inch
		double unitMm = 25.4 / 96;
		/// Put the terminals on the grid of 10
		bool snapTerminals = true;
		/// Warn when the element has no terminal
		bool warnNoTerminal = true;
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
		@brief Convert a picture: its dark areas are traced by
		RasterVectorizer, then converted as a SVG. A pixel is 25.4 / @p dpi
		millimetres. Terminals are given in pixels of the picture.
	*/
	Result convertImage(const QImage &image, Options options,
						const RasterVectorizer::Options &vectorizer,
						double dpi = 96);

	/**
		@brief Convert the SVG or picture file @p path (see convertImage()).
	*/
	Result convertFile(const QString &path, const Options &options,
					   const RasterVectorizer::Options &vectorizer,
					   double dpi = 96);

	/// File name filter listing the SVG and picture formats convertFile() reads
	QString fileFilter();

	/**
		@brief Find what to convert in @p mime, such as the clipboard: SVG
		data or text, a picture, or a file (URL) that is one.
		@param name receives a name for the element, when there is one
		@return true if there is a SVG (in @p svg), or a picture (in @p image)
	*/
	bool sourceFromMimeData(const QMimeData *mime, QByteArray *svg,
							QImage *image, QString *name = nullptr);

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
