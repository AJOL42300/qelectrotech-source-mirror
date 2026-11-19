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
#ifndef WIRECOLOURENTRYCOMBOBOX_H
#define WIRECOLOURENTRYCOMBOBOX_H

#include <QComboBox>

#include <properties/wirecolourtable.h>

class QETDiagramEditor;
class QETProject;

/**
	@brief The WireColourEntryComboBox class
	The drop-down list of the toolbar giving the wire colour entries of the
	project (see WireColourTable). Choosing one applies it to the selected
	conductors as one undo step, and the colour becomes the colour of the
	next conductor drawn.
*/
class WireColourEntryComboBox : public QComboBox
{
		Q_OBJECT

	public:
		explicit WireColourEntryComboBox(QETDiagramEditor *editor, QWidget *parent = nullptr);

		static QIcon swatchIcon(const WireColourEntry &profile);

	public slots:
		void updateEnabledState();

	protected:
		void showPopup() override;

	private:
		QETProject *currentProject() const;
		void refresh();
		void profileChosen(int index);

		QETDiagramEditor *m_editor = nullptr;
		QList<WireColourEntry> m_shown;
};

#endif // WIRECOLOURENTRYCOMBOBOX_H
