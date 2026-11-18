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
#include "imagestorage.h"

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QSettings>

#include <algorithm>

bool ImageStorage::convertEnabled()
{
	return QSettings().value(convertKey, true).toBool();
}

int ImageStorage::jpegQuality()
{
	return std::clamp(QSettings().value(qualityKey, defaultQuality).toInt(),
					  1, 100);
}

bool ImageStorage::isPng(const QByteArray &data)
{
	return data.startsWith("\x89PNG\r\n\x1a\n");
}

bool ImageStorage::isJpeg(const QByteArray &data)
{
	return data.startsWith("\xFF\xD8\xFF");
}

bool ImageStorage::hasTranslucentPixels(const QImage &image)
{
	if (!image.hasAlphaChannel())
		return false;
	const QImage argb = image.format() == QImage::Format_ARGB32
			? image : image.convertToFormat(QImage::Format_ARGB32);
	for (int y = 0 ; y < argb.height() ; ++y)
	{
		const QRgb *line = reinterpret_cast<const QRgb *>(argb.constScanLine(y));
		for (int x = 0 ; x < argb.width() ; ++x)
			if (qAlpha(line[x]) != 255)
				return true;
	}
	return false;
}

QImage ImageStorage::loadFile(const QString &path, bool *keepAlpha,
							  QByteArray *bytes)
{
	const QImage image(path);
	if (!image.isNull())
		inspectFile(path, image, keepAlpha, bytes);
	return image;
}

void ImageStorage::inspectFile(const QString &path, const QImage &image,
							   bool *keepAlpha, QByteArray *bytes)
{
	if (bytes)
		bytes->clear();
	const QString suffix = QFileInfo(path).suffix();
	const bool svg = suffix.compare(QLatin1String("svg"), Qt::CaseInsensitive) == 0
			|| suffix.compare(QLatin1String("svgz"), Qt::CaseInsensitive) == 0;
	if (keepAlpha)
		*keepAlpha = svg || hasTranslucentPixels(image);
	if (bytes)
	{
		QFile file(path);
		if (file.open(QIODevice::ReadOnly))
		{
			const QByteArray content = file.readAll();
			if (isJpeg(content))
				*bytes = content;
		}
	}
}

QByteArray ImageStorage::encode(const QImage &image, bool keepAlpha,
								bool convert, int quality)
{
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	if (keepAlpha || !convert)
	{
		image.save(&buffer, "PNG");
		return bytes;
	}

	// Qt flattens transparency on black: use white, as paper is.
	QImage flat(image.size(), QImage::Format_RGB32);
	flat.fill(Qt::white);
	{
		QPainter painter(&flat);
		painter.drawImage(0, 0, image);
	}
	flat.save(&buffer, "JPEG", std::clamp(quality, 1, 100));
	return bytes;
}
