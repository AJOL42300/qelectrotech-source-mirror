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
#ifndef AUTOLABEL_H
#define AUTOLABEL_H

#include <QSet>
#include <QString>

#include "properties/elementdata.h"

/**
	@brief Label given to an element when it is placed on a folio.

	The label is the prefix of the element ("K", "A"...) followed by the
	smallest number no other element of the project already carries:
	"K1", "K2"... A label which is free again is used again. A project
	which numbers its elements with a scheme keeps that scheme: this is for
	projects which have none.

	Pure logic, with no dependency on the project: the element and the
	tests are built on it.
*/
namespace autolabel
{
	QString collectionFolder(const QString &path);
	QString fallbackPrefix(
		ElementData::Type type,
		ElementData::MasterType masterType,
		const QString &folder);
	QString nextFreeLabel(const QString &prefix, const QSet<QString> &held);
}

#endif
