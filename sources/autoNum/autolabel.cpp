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
#include "autolabel.h"

#include <QStringList>

namespace autolabel
{
	/**
		@brief The top-level folder of an element in its collection.
		@param path the path of the element, such as
		"embed://import/15_beckhoff_ethercat/el1xxx/el1008.elmt"
		@return "15_beckhoff_ethercat", or an empty string for an element
		which is not in a folder
	*/
	QString collectionFolder(const QString &path)
	{
		QString rest = path;
		const int protocol = rest.indexOf(QLatin1String("://"));
		if (protocol >= 0)
			rest = rest.mid(protocol + 3);
		QStringList parts = rest.split(
			QLatin1Char('/'), Qt::SkipEmptyParts);
		if (!parts.isEmpty() && parts.first() == QLatin1String("import"))
			parts.removeFirst();
		return parts.size() > 1 ? parts.first() : QString();
	}

	/**
		@brief The prefix of an element whose collection does not give one.
		The collections which describe their prefixes (qet_labels.xml) are
		asked first: this is only for the others, by the folder of the
		collection and, failing that, by what the element is.
		@param type the kind of the element
		@param masterType what a master element stands for
		@param folder the top-level folder of the element
		@return the prefix, or an empty string when there is none to give
	*/
	QString fallbackPrefix(
		ElementData::Type type,
		ElementData::MasterType masterType,
		const QString &folder)
	{
			//The cards, couplers and boxes of the Beckhoff library
		if (folder == QLatin1String("15_beckhoff_ethercat"))
			return QStringLiteral("A");
		if (type == ElementData::Terminal)
			return QStringLiteral("X");
		if (type != ElementData::Master)
			return QString();
		switch (masterType)
		{
			case ElementData::Coil:
				return QStringLiteral("K");
			case ElementData::Protection:
				return QStringLiteral("Q");
			case ElementData::Commutator:
				return QStringLiteral("S");
			case ElementData::PLC:
				return QStringLiteral("A");
		}
		return QString();
	}

	/**
		@brief The first label of a prefix which no other element has.
		A number which is free again is used again: with K1 and K3 in the
		project, the next one is K2.
		@param prefix the prefix of the label
		@param held the labels of the other elements of the project
		@return the label, or an empty string when the prefix is empty
	*/
	QString nextFreeLabel(const QString &prefix, const QSet<QString> &held)
	{
		if (prefix.isEmpty())
			return QString();
		for (int number = 1; number < 1000000; ++number)
		{
			const QString label = prefix + QString::number(number);
			if (!held.contains(label))
				return label;
		}
		return QString();
	}
}
