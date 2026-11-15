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
#ifndef PICTUREURL_H
#define PICTUREURL_H

#include <QByteArray>
#include <QString>
#include <QUrl>

/**
	@brief Fetch one picture from the web, on the user's request, to make
	an element from it (see SvgToElmt).

	This is not a crawler: one address in, at most two requests (the page
	and the picture it shows), like a browser that shows one product. The
	picture is kept in the user's cache folder, never in the repository.
	The picture belongs to its author (for the manufacturers' product
	pictures, the manufacturer): the caller must show notice().
*/
namespace PictureUrl {

	/// true if @p text is an http or https address
	bool isUrl(const QString &text);

	/**
		@brief The address of the picture a product page shows: the
		og:image of the page, else the first Beckhoff product preview.
		@param base used to complete a relative address
		@return an empty URL if @p html shows none
	*/
	QUrl pictureLinkIn(const QByteArray &html, const QUrl &base);

	/// Folder where the downloaded pictures are kept
	QString cacheFolder();

	/**
		@brief Download @p url, a picture or a page that shows one, to the
		cache folder (https only, 10 MB at most, 30 s).
		@param file receives the path of the picture
		@param pictureUrl receives the address the picture came from
		@return false, and the reason in @p error, if it failed
	*/
	bool download(const QString &url, QString *file, QUrl *pictureUrl,
				  QString *error);

	/// true if @p path is in a git working tree (do not commit pictures)
	bool isInGitRepository(const QString &path);

	/// What the user must know about the rights on the picture
	QString notice();

}

#endif // PICTUREURL_H
