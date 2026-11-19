// SPDX-License-Identifier: GPL-2.0-or-later
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
#ifndef WIRECOLOURTABLEWIDGET_H
#define WIRECOLOURTABLEWIDGET_H

#include <properties/wirecolourtable.h>

#include <QWidget>

class QTableWidget;

/**
	@brief The WireColourTableWidget class
	Edits the wire colour meaning table of a project: for each function,
	the wire colour code, the colour (and stripe colour) it is drawn in,
	the section and the label.
*/
class WireColourTableWidget : public QWidget
{
		Q_OBJECT

	public:
		explicit WireColourTableWidget(QWidget *parent = nullptr);

		void setWireColourTable(const WireColourTable &profiles);
		WireColourTable wireColourTable() const;

	private:
		void addRow(const WireColourEntry &profile);
		void chooseColor(QWidget *button, bool allow_none);
		void setProfiles(const QList<WireColourEntry> &profiles);

		QTableWidget *m_table = nullptr;
};

#endif // WIRECOLOURTABLEWIDGET_H
