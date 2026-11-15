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
#ifndef RASTERVECTORIZER_H
#define RASTERVECTORIZER_H

#include <QByteArray>
#include <QImage>
#include <QString>
#include <QStringList>

/**
	@brief Trace the dark areas of a picture (PNG, JPG...) to outlines.

	Meant for line drawings, logos and symbols: the picture is reduced to
	black and white at a threshold, the outline of each black area and of
	each hole in it is followed along the pixel edges, then simplified.
	A photograph or a gradient is not converted properly: it gives a
	posterised blob.

	The result is a SVG drawing, in the pixels of the picture, that
	SvgToElmt::convert() turns into an element. A black area is a black
	polygon, a hole in it a white polygon drawn above. (The holes are not
	transparent: an element part cannot cut one.)
*/
namespace RasterVectorizer {

	struct Options {
		/// A pixel darker than this (0 black - 255 white) is drawn. A
		/// transparent pixel is white.
		int threshold = 128;
		/// Draw the light pixels instead: for a white drawing on black
		bool invert = false;
		/// Largest distance, in pixels of the picture, between an outline
		/// and its simplification. 0: keep every step of the pixels
		double simplify = 1.0;
		/// Areas (and holes) of at most this many pixels are dropped
		int despeckle = 2;
		/// Most outlines kept: the largest ones. 0: no limit
		int maxPaths = 500;
	};

	struct Result {
		bool ok = false;
		QString error;
		QStringList warnings;
		/// A SVG drawing, in the pixels of the picture
		QByteArray svg;
		/// Outlines in it
		int paths = 0;
	};

	Result toSvg(const QImage &image, const Options &options);

}

#endif // RASTERVECTORIZER_H
