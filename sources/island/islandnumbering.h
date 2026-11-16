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
#ifndef ISLANDNUMBERING_H
#define ISLANDNUMBERING_H

#include <QList>
#include <QString>
#include <QStringList>

/**
	@brief Numbering of the cards (I/O terminals) of an island.

	An island has an identifier ("A1", "A2"...), a head (the coupler or
	box, labelled "A1.00") and cards. A card takes the identifier of its
	island and a number: "A1.10", "A1.20"... The numbers go by steps of 10,
	so a card can be inserted later ("A1.15") without renumbering the
	others. The order of the cards on the bus is the order of their
	numbers. The head is number 0. An older project may label the head with
	the bare identifier ("A1"): it is still taken for the head.
	The identifier is free ("A1", "BOX 03"...): a copy of an island takes
	the next number after the one it ends with.

	Pure logic, with no dependency on the project: the dialog and the
	island folio are built on it.
*/
namespace IslandNumbering
{
	constexpr int step = 10;

	QString childLabel(const QString &parent, int number);
	QString headLabel(const QString &parent);
	QString islandOf(const QString &label);
	QStringList appearancePaths(const QString &path);
	bool numberOf(const QString &label, const QString &parent, int *number);
	QList<int> numbersAfter(const QList<int> &used, int count, int after);
	bool isTaken(const QString &parent, const QStringList &labels);
	QString nextParent(const QString &parent, const QStringList &labels);
	QStringList copyLabels(const QString &parent, int count);
	QString cardsQuery(const QString &parent);
	QString ioQuery(const QString &parent);
}

#endif
