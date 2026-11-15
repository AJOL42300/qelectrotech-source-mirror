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
#ifndef MODULETOOLTIP_H
#define MODULETOOLTIP_H

#include "properties/elementdata.h"

#include <QList>
#include <QString>
#include <QStringList>

/**
	@brief Text of the tooltip that describes the wiring of a module
	(a PLC card, a box, a drive): what is on each terminal and which
	terminals are linked inside the module.

	It reads no item: the caller gives the terminals and the I/O table, so
	that the text can be tested.

	The model uses what an element already has:
	- the terminals that share a non-empty potential of the symbol are
	linked inside the module (24 V, 0 V, PE, a feed-through...);
	- an entry of the I/O table belongs to a terminal when its address is
	the name of the terminal, or starts with it followed by "(", as
	"X01(1)" for the terminal "X01".
*/
namespace ModuleTooltip
{
	/// A terminal of the module, as the tooltip needs it.
	struct Pin {
		QString name;
		QString potential;
		/// what is wired on it, one text per wire
		QStringList wires;
		/// the folios where it is wired, on any placement of the module
		QStringList folios;
	};

	/// true if the pin is wired somewhere, or if its channel is taken by
	/// a linked symbol (an entry of the table with a cross reference)
	bool isUsed(const Pin &pin, const QVector<ElementData::PlcIO> &ios);

	/// The entries of @p ios that belong to the terminal @p pin_name.
	QList<ElementData::PlcIO> entriesOf(
		const QString &pin_name,
		const QVector<ElementData::PlcIO> &ios);

	/// "DI", "DO", "AI", "AO", "UI" or "UO".
	QString shortType(ElementData::PlcIOType type);

	/// One line (rich text) describing a terminal.
	QString pinText(const Pin &pin,
			const QList<Pin> &pins,
			const QVector<ElementData::PlcIO> &ios);

	/// The tooltip of the module (rich text), empty if there is nothing
	/// to say beyond its name.
	QString moduleText(const QString &title,
			   const QList<Pin> &pins,
			   const QVector<ElementData::PlcIO> &ios);
}

#endif // MODULETOOLTIP_H
