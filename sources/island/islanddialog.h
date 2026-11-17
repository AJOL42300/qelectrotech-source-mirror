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
#include <QStringList>

class Diagram;
class Element;
class QCheckBox;
class QComboBox;
class QETProject;
class QLineEdit;
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

/**
	@brief Dialog to duplicate an island: its head and all its cards.

	"New copy" makes an independent island: the copy takes a new label
	(the next free one) and its cards take it by steps of 10, in the same
	order. "Linked representation" shows the same island again, with the
	same labels, on another folio. All the copies are made by one undo
	command, on the target folio.
*/
class IslandDuplicateDialog : public QDialog
{
	Q_OBJECT

	public:
		IslandDuplicateDialog(QETProject *project, const QString &island,
							  Diagram *target, QWidget *parent = nullptr);

		void accept() override;

	private:
		QList<Element *> islandElements(Diagram *diagram) const;
		QStringList allLabels() const;
		bool duplicate();

		QETProject *m_project;
		QString m_island;
		QRadioButton *m_new_copy;
		QRadioButton *m_linked;
		QLineEdit *m_label;
		QComboBox *m_folio;
		QCheckBox *m_keep_wires;
		QCheckBox *m_keep_signals;
};

/**
	@brief Other appearances of a card.

	A card can be drawn as a symbol, as a realistic view and as a table of
	its inputs and outputs: three elements of the same collection folder,
	named "card.elmt", "card_realistic.elmt" and "card_table.elmt"
	(see IslandNumbering::appearancePaths). They share the label of the
	card, so they are the same card on the folios.
*/
namespace IslandAppearances
{
	bool hasMissing(QETProject *project, Element *element);
	int create(QETProject *project, Element *element, Diagram *target,
			   QString *error = nullptr);
	void askAndCreate(QETProject *project, Element *element,
					  Diagram *current, QWidget *parent);
}

#endif
