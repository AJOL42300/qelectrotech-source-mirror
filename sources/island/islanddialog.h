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
#ifndef ISLANDDIALOG_H
#define ISLANDDIALOG_H

#include <QDialog>
#include <QList>
#include <QPair>
#include <QString>

class Diagram;
class Element;
class QCheckBox;
class QComboBox;
class QETProject;
class QRadioButton;
class QUndoCommand;

/**
	@brief Dialog to number the cards of an island.

	The cards take the label of their island and a number by steps of 10
	(see IslandNumbering). Either the selected elements of the folio are
	numbered as cards of the island, at its end or after a given card
	without renumbering the others, or all the cards are renumbered by
	steps of 10 in their current order.
*/
class IslandDialog : public QDialog
{
	Q_OBJECT

	public:
		IslandDialog(QETProject *project, Diagram *diagram,
					 QWidget *parent = nullptr);

		QString island() const;
		QUndoCommand *takeUndoCommand();

		void accept() override;

	private:
		typedef QPair<Element *, int> Card;

		QList<Element *> selectedElements() const;
		QList<Card> cards(const QString &island) const;
		QUndoCommand *build(QString *error) const;
		void fillPositions();

		QETProject *m_project;
		Diagram *m_diagram;
		QComboBox *m_island;
		QRadioButton *m_number_selection;
		QRadioButton *m_renumber;
		QComboBox *m_position;
		QCheckBox *m_left_to_right;
		QUndoCommand *m_command = nullptr;
};

#endif
