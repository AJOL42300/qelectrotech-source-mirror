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
#ifndef PINUSAGE_H
#define PINUSAGE_H

#include <QString>
#include <QStringList>

class Element;
class Terminal;

/**
	@brief Which terminals of a module are wired, on which folios.

	A module (a PLC card, a box) can be placed on several folios, each time
	showing the channels used there. The placements of one module share
	the same label: the terminals with the same name on those placements
	are the same pin of the module.

	The usage of a whole project is gathered once, then kept until a
	conductor or a label changes (invalidate()).
*/
namespace PinUsage
{
	/// The pins greyed when they are wired on another placement of the
	/// module (a setting, off by default).
	bool greyUsedPinsIsOn();
	void setGreyUsedPins(bool on);

	/// Something changed: the usage must be gathered again.
	void invalidate();

	/// true if @p terminal has no conductor but the same pin of another
	/// placement of its module has one.
	bool usedElsewhere(const Terminal *terminal);

	/// The folios ("folio 12") where the pin @p pin_name of the module
	/// that @p element belongs to is wired, in folio order.
	QStringList foliosOf(const Element *element, const QString &pin_name);
}

#endif // PINUSAGE_H
