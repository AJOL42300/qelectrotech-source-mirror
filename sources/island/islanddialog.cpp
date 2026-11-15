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
#include "islanddialog.h"

#include "islandnumbering.h"
#include "../diagram.h"
#include "../positionorder.h"
#include "../qet.h"
#include "../qetgraphicsitem/element.h"
#include "../qetproject.h"
#include "../undocommand/changeelementinformationcommand.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QRadioButton>
#include <QSet>
#include <QUndoCommand>
#include <QVBoxLayout>
#include <algorithm>

/**
	@brief IslandDialog::IslandDialog
	@param project : project whose cards are numbered
	@param diagram : folio whose selected elements are numbered
	@param parent : parent widget
*/
IslandDialog::IslandDialog(QETProject *project, Diagram *diagram,
						   QWidget *parent) :
	QDialog(parent),
	m_project(project),
	m_diagram(diagram)
{
	setWindowTitle(tr("Number the cards of an island"));
	QET::trackDialogGeometry(this);

	m_island = new QComboBox(this);
	m_island->setEditable(true);

		//The islands the project already has (labels "parent.number")
		//and the labels of the selected elements, in case the first card
		//of a new island is being numbered
	QSet<QString> islands;
	for (Diagram *d : m_project->diagrams()) {
		for (QGraphicsItem *item : d->items()) {
			if (Element *elmt = qgraphicsitem_cast<Element *>(item)) {
				const QString label = elmt->actualLabel();
				const int dot = label.lastIndexOf(QLatin1Char('.'));
				int number = 0;
				if (dot > 0
					&& IslandNumbering::numberOf(
						label, label.left(dot), &number)) {
					islands.insert(label.left(dot));
				}
			}
		}
	}
	for (Element *elmt : selectedElements()) {
		if (!elmt->actualLabel().isEmpty()) {
			islands.insert(elmt->actualLabel());
		}
	}
	QStringList sorted = islands.values();
	std::sort(sorted.begin(), sorted.end());
	m_island->addItems(sorted);

	m_number_selection = new QRadioButton(
		tr("Number the selected elements as cards of the island"), this);
	m_number_selection->setChecked(true);
	m_position = new QComboBox(this);
	m_left_to_right = new QCheckBox(
		tr("Order the selection from left to right (otherwise from top to "
		   "bottom)"), this);
	m_left_to_right->setChecked(true);
	m_renumber = new QRadioButton(
		tr("Renumber all the cards of the island by steps of %1")
			.arg(IslandNumbering::step), this);

	auto buttons = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel(
		tr("Island (the label of its coupler or box):"), this));
	layout->addWidget(m_island);
	layout->addWidget(m_number_selection);
	layout->addWidget(new QLabel(tr("Place them:"), this));
	layout->addWidget(m_position);
	layout->addWidget(m_left_to_right);
	layout->addWidget(m_renumber);
	layout->addWidget(buttons);

	fillPositions();
	connect(m_island, &QComboBox::editTextChanged,
			this, [this]() { fillPositions(); });
	connect(m_number_selection, &QRadioButton::toggled, this,
			[this](bool on) {
		m_position->setEnabled(on);
		m_left_to_right->setEnabled(on);
	});
}

/**
	@return the label of the island
*/
QString IslandDialog::island() const
{
	return m_island->currentText().trimmed();
}

/**
	@return the command made by the last accepted dialog, which the caller
	owns, or nullptr when it changes nothing
*/
QUndoCommand *IslandDialog::takeUndoCommand()
{
	QUndoCommand *command = m_command;
	m_command = nullptr;
	return command;
}

void IslandDialog::accept()
{
	QString error;
	QUndoCommand *command = build(&error);
	if (!error.isEmpty()) {
		QMessageBox::warning(this, windowTitle(), error);
		return;
	}
	delete m_command;
	m_command = command;
	QDialog::accept();
}

/**
	@return the selected elements of the folio, in the order of the bus
*/
QList<Element *> IslandDialog::selectedElements() const
{
	QList<Element *> list;
	if (!m_diagram) {
		return list;
	}
	for (QGraphicsItem *item : m_diagram->selectedItems()) {
		if (Element *elmt = qgraphicsitem_cast<Element *>(item)) {
			list << elmt;
		}
	}
	const bool horizontal = m_left_to_right ? m_left_to_right->isChecked()
											: true;
	std::sort(list.begin(), list.end(),
			  [horizontal](Element *a, Element *b) {
		return horizontal
			? PositionOrder::roundedXThenY(a->pos(), b->pos())
			: PositionOrder::roundedYThenX(a->pos(), b->pos());
	});
	return list;
}

/**
	@param island : label of the island
	@return the cards of the island on all the folios, ordered by number
*/
QList<IslandDialog::Card> IslandDialog::cards(const QString &island) const
{
	QList<Card> list;
	for (Diagram *d : m_project->diagrams()) {
		for (QGraphicsItem *item : d->items()) {
			if (Element *elmt = qgraphicsitem_cast<Element *>(item)) {
				int number = 0;
				if (IslandNumbering::numberOf(
						elmt->actualLabel(), island, &number)) {
					list << Card(elmt, number);
				}
			}
		}
	}
	std::sort(list.begin(), list.end(),
			  [](const Card &a, const Card &b) {
		return a.second < b.second;
	});
	return list;
}

void IslandDialog::fillPositions()
{
	const auto selection = selectedElements();
	m_position->clear();
	m_position->addItem(tr("At the end of the island"), -1);
	m_position->addItem(tr("At the beginning of the island"), 0);
	for (const Card &card : cards(island())) {
		if (!selection.contains(card.first)) {
			m_position->addItem(
				tr("After card %1").arg(
					IslandNumbering::childLabel(island(), card.second)),
				card.second);
		}
	}
}

/**
	@param error : receives the reason when nothing can be done
	@return the command renumbering the cards, or nullptr
*/
QUndoCommand *IslandDialog::build(QString *error) const
{
	const QString parent = island();
	if (parent.isEmpty()) {
		*error = tr("Give the label of the island.");
		return nullptr;
	}

	QList<QPair<Element *, QString>> changes;
	if (m_renumber->isChecked()) {
		int number = IslandNumbering::step;
		for (const Card &card : cards(parent)) {
			changes << qMakePair(
				card.first,
				IslandNumbering::childLabel(parent, number));
			number += IslandNumbering::step;
		}
	} else {
		QList<Element *> selection;
		for (Element *elmt : selectedElements()) {
				//The coupler itself is not one of its cards
			if (elmt->actualLabel() != parent) {
				selection << elmt;
			}
		}
		if (selection.isEmpty()) {
			*error = tr("Select the elements to number first.");
			return nullptr;
		}
		QList<int> used;
		for (const Card &card : cards(parent)) {
			if (!selection.contains(card.first)) {
				used << card.second;
			}
		}
		const QList<int> numbers = IslandNumbering::numbersAfter(
			used, selection.size(), m_position->currentData().toInt());
		if (numbers.size() != selection.size()) {
			*error = tr("There is no room there for %n card(s): renumber "
						"the island first.", nullptr, selection.size());
			return nullptr;
		}
		for (int i = 0 ; i < selection.size() ; ++i) {
			changes << qMakePair(
				selection.at(i),
				IslandNumbering::childLabel(parent, numbers.at(i)));
		}
	}

	auto macro = new QUndoCommand(tr("Number the cards of %1").arg(parent));
	for (const auto &change : changes) {
		DiagramContext old_info = change.first->elementInformations();
		DiagramContext new_info = old_info;
		new_info.addValue(QStringLiteral("label"), change.second);
			//A formula would hide the label
		if (!new_info.value(QStringLiteral("formula")).toString().isEmpty()) {
			new_info.addValue(QStringLiteral("formula"), QString());
		}
		if (old_info != new_info) {
			new ChangeElementInformationCommand(
				change.first, old_info, new_info, macro);
		}
	}
	if (macro->childCount() == 0) {
		delete macro;
		return nullptr;
	}
	return macro;
}
