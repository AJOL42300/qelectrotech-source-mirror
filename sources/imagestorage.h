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
#ifndef IMAGESTORAGE_H
#define IMAGESTORAGE_H

#include <QByteArray>
#include <QImage>

/**
	How a folio's pictures are stored in a project file: a picture
	without transparency is stored as JPEG, which is far smaller than
	PNG for a photo, and a picture with transparency as PNG. Free of any
	item so that it can be tested on its own.
*/
namespace ImageStorage
{
	/// Settings keys of Configuration > General.
	const char convertKey[] = "images/convert-to-jpeg";
	const char qualityKey[] = "images/jpeg-quality";
	const int defaultQuality = 85;

	/// Whether pictures without transparency are stored as JPEG.
	bool convertEnabled();
	/// JPEG quality, 1 to 100.
	int jpegQuality();

	bool isPng(const QByteArray &data);
	bool isJpeg(const QByteArray &data);

	/// True when at least one pixel of @p image is not fully opaque.
	bool hasTranslucentPixels(const QImage &image);

	/**
		Reads the picture file @p path. @p keepAlpha tells whether it
		must be stored with its transparency: it has some, or it is an SVG
		(rasterised here, since a project file holds no vector picture).
		@p bytes receives the file's content when it is a JPEG, which
		can then be stored as it is.
	*/
	QImage loadFile(const QString &path, bool *keepAlpha, QByteArray *bytes);
	/// Same, for an @p image already loaded from @p path.
	void inspectFile(const QString &path, const QImage &image,
					 bool *keepAlpha, QByteArray *bytes);

	/**
		@return @p image as PNG when @p keepAlpha or @p convert is false,
		otherwise as JPEG of the given quality, its transparent parts
		flattened on white.
	*/
	QByteArray encode(const QImage &image, bool keepAlpha,
					  bool convert, int quality);
}

#endif
