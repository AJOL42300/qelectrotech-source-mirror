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
#ifndef WIREPROFILESWIDGET_H
#define WIREPROFILESWIDGET_H

#include <properties/wireprofiles.h>

#include <QHash>
#include <QWidget>

class QComboBox;
class QTableWidget;

/**
	@brief The WireProfilesWidget class
	Edits the standard wires of a project: a name, a colour and a
	section for each. The conductors that name a profile follow when the
	change is applied (ChangeWireProfilesCommand); renamed() tells which
	profiles were renamed so that their conductors keep them.
*/
class WireProfilesWidget : public QWidget
{
		Q_OBJECT

	public:
		explicit WireProfilesWidget(QWidget *parent = nullptr);

		void setWireProfiles(const WireProfiles &profiles);
		WireProfiles wireProfiles() const;
		QHash<QString, QString> renamed() const;

	private:
		void addRow(const WireProfile &profile);

		QTableWidget *m_table = nullptr;
};

#endif // WIREPROFILESWIDGET_H
