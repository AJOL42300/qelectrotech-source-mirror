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
#include "../diagramcommands.h"
#include "../diagramcontent.h"
#include "../ElementsCollection/elementslocation.h"
#include "../factory/elementfactory.h"
#include "../positionorder.h"
#include "../qet.h"
#include "../qetgraphicsitem/element.h"
#include "../qetproject.h"
#include "../undocommand/addgraphicsobjectcommand.h"
#include "../undocommand/changeelementdatacommand.h"
#include "../undocommand/changeelementinformationcommand.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDomDocument>
#include <QHash>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QRadioButton>
#include <QSet>
#include <QUndoCommand>
#include <QUndoStack>
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
				const QString island = IslandNumbering::islandOf(label);
				if (island != label) {
					islands.insert(island);
				}
			}
		}
	}
	for (Element *elmt : selectedElements()) {
		if (!elmt->actualLabel().isEmpty()) {
			islands.insert(IslandNumbering::islandOf(elmt->actualLabel()));
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
		tr("Island (its identifier, e.g. A1):"), this));
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
					//The head (number 0) is not one of its cards
				if (IslandNumbering::numberOf(
						elmt->actualLabel(), island, &number) && number > 0) {
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
		*error = tr("Give the identifier of the island.");
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
			int head_number = -1;
			if (elmt->actualLabel() != parent
				&& (!IslandNumbering::numberOf(
						elmt->actualLabel(), parent, &head_number)
					|| head_number > 0)) {
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

/**
	@brief IslandDuplicateDialog::IslandDuplicateDialog
	@param project : project of the island
	@param island : label of the head of the island to duplicate
	@param target : folio where the copy is made by default
	@param parent : parent widget
*/
IslandDuplicateDialog::IslandDuplicateDialog(QETProject *project,
											 const QString &island,
											 Diagram *target,
											 QWidget *parent) :
	QDialog(parent),
	m_project(project),
	m_island(island)
{
	setWindowTitle(tr("Duplicate the island %1").arg(island));
	QET::trackDialogGeometry(this);

	m_new_copy = new QRadioButton(
		tr("New copy: an independent island with a new label"), this);
	m_new_copy->setChecked(true);
	m_label = new QLineEdit(
		IslandNumbering::nextParent(island, allLabels()), this);
	m_linked = new QRadioButton(
		tr("Linked representation: the same island shown again, with the "
		   "same labels"), this);

	m_folio = new QComboBox(this);
	for (Diagram *d : m_project->diagrams()) {
		m_folio->addItem(d->title(), QVariant::fromValue<void *>(d));
		if (d == target) {
			m_folio->setCurrentIndex(m_folio->count() - 1);
		}
	}

	m_keep_wires = new QCheckBox(
		tr("Keep the wires between the cards of the island"), this);
	m_keep_wires->setChecked(true);
	m_keep_signals = new QCheckBox(
		tr("Keep the function and the comment of the channels"), this);

	auto buttons = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto layout = new QVBoxLayout(this);
	layout->addWidget(m_new_copy);
	layout->addWidget(new QLabel(tr("Identifier of the new island (e.g. A2):"), this));
	layout->addWidget(m_label);
	layout->addWidget(m_keep_signals);
	layout->addWidget(m_linked);
	layout->addWidget(new QLabel(tr("Folio of the copy:"), this));
	layout->addWidget(m_folio);
	layout->addWidget(m_keep_wires);
	layout->addWidget(buttons);

	connect(m_new_copy, &QRadioButton::toggled, this, [this](bool on) {
		m_label->setEnabled(on);
		m_keep_signals->setEnabled(on);
	});
}

/**
	@param diagram : a folio of the project
	@return the head of the island and its cards on @p diagram
*/
QList<Element *> IslandDuplicateDialog::islandElements(Diagram *diagram) const
{
	QList<Element *> list;
	for (QGraphicsItem *item : diagram->items()) {
		if (Element *elmt = qgraphicsitem_cast<Element *>(item)) {
			const QString label = elmt->actualLabel();
			if (label == m_island
				|| IslandNumbering::numberOf(label, m_island, nullptr)) {
				list << elmt;
			}
		}
	}
	return list;
}

/**
	@return the labels of all the elements of the project
*/
QStringList IslandDuplicateDialog::allLabels() const
{
	QStringList labels;
	for (Diagram *d : m_project->diagrams()) {
		for (QGraphicsItem *item : d->items()) {
			if (Element *elmt = qgraphicsitem_cast<Element *>(item)) {
				labels << elmt->actualLabel();
			}
		}
	}
	return labels;
}

void IslandDuplicateDialog::accept()
{
	if (duplicate()) {
		QDialog::accept();
	}
}

/**
	Copies the island on the chosen folio, by one undo command.
	@return false, after telling why, if nothing was done
*/
bool IslandDuplicateDialog::duplicate()
{
	auto *target = static_cast<Diagram *>(m_folio->currentData().value<void *>());
	if (!target) {
		return false;
	}
	const bool linked = m_linked->isChecked();
	const QString new_island = m_label->text().trimmed();
	if (!linked) {
		if (new_island.isEmpty()) {
			QMessageBox::warning(this, windowTitle(),
								 tr("Give the identifier of the new island."));
			return false;
		}
		if (IslandNumbering::isTaken(new_island, allLabels())) {
			QMessageBox::warning(
				this, windowTitle(),
				tr("The label %1 is already used by an island or by one of "
				   "its cards.").arg(new_island));
			return false;
		}
	}

		//The islands elements by folio, in the order of the folios
	QList<QPair<Diagram *, QList<Element *>>> groups;
	QList<int> numbers;
	QHash<QString, DiagramContext> source_info;
	for (Diagram *d : m_project->diagrams()) {
		const QList<Element *> elements = islandElements(d);
		if (elements.isEmpty()) {
			continue;
		}
		groups << qMakePair(d, elements);
		for (Element *elmt : elements) {
			int number = 0;
			if (IslandNumbering::numberOf(
					elmt->actualLabel(), m_island, &number)
				&& number > 0 && !numbers.contains(number)) {
				numbers << number;
			}
			if (!source_info.contains(elmt->actualLabel())) {
				source_info.insert(elmt->actualLabel(),
								   elmt->elementInformations());
			}
		}
	}
	if (groups.isEmpty()) {
		QMessageBox::warning(this, windowTitle(),
							 tr("There is no element labelled %1.")
								 .arg(m_island));
		return false;
	}

		//The new label of each label of the island
	std::sort(numbers.begin(), numbers.end());
	QHash<QString, QString> renamed;
	renamed.insert(m_island, linked ? m_island : new_island);
	renamed.insert(IslandNumbering::headLabel(m_island),
				   linked ? IslandNumbering::headLabel(m_island)
						  : IslandNumbering::headLabel(new_island));
	const QStringList copies =
		IslandNumbering::copyLabels(new_island, numbers.size());
	for (int i = 0 ; i < numbers.size() ; ++i) {
		const QString old_label =
			IslandNumbering::childLabel(m_island, numbers.at(i));
		renamed.insert(old_label, linked ? old_label : copies.at(i));
	}

	QUndoStack &stack = target->undoStack();
	stack.beginMacro(linked
		? tr("Show the island %1 again").arg(m_island)
		: tr("Duplicate the island %1 as %2").arg(m_island, new_island));
	qreal next_x = 0;
	bool first = true;
	for (const auto &group : std::as_const(groups)) {
		Diagram *source = group.first;
		QRectF area;
		for (Element *elmt : group.second) {
			area = area.united(elmt->sceneBoundingRect());
		}

			//Only the elements of the island are copied: the selection is
			//made for the export, then given back
		const QList<QGraphicsItem *> previous = source->selectedItems();
		source->clearSelection();
		for (Element *elmt : group.second) {
			elmt->setSelected(true);
		}
		QDomDocument document = source->toXml(false, true);
		source->clearSelection();
		for (QGraphicsItem *item : previous) {
			item->setSelected(true);
		}
		if (!m_keep_wires->isChecked()) {
			const QDomElement wires = document.documentElement()
				.firstChildElement(QStringLiteral("conductors"));
			if (!wires.isNull()) {
				document.documentElement().removeChild(wires);
			}
		}

		QPointF position;
		if (first) {
			position = source == target
				? QPointF(area.right() + 40, area.top())
				: area.topLeft();
		} else {
			position = QPointF(next_x, area.top());
		}
			//A null position would not be applied
		position += QPointF(0.1, 0.1);
		next_x = position.x() + area.width() + 40;
		first = false;

		target->clearSelection();
		DiagramContent content;
		target->fromXml(document, position, false, &content);
		QHash<Element *, QString> old_labels;
		for (Element *elmt : content.m_elements) {
			old_labels.insert(elmt, elmt->actualLabel());
		}
		auto *paste = new PasteDiagramCommand(target, content);
		paste->setAutoNumbering(false);
		stack.push(paste);

		for (Element *elmt : content.m_elements) {
			const QString old_label = old_labels.value(elmt);
			if (!renamed.contains(old_label)) {
				continue;
			}
			ElementData data = elmt->elementData();
			if (linked) {
				data.m_informations = source_info.value(old_label);
			} else {
				data.m_informations.addValue(
					QStringLiteral("label"), renamed.value(old_label));
				data.m_informations.addValue(
					QStringLiteral("formula"), QString());
				data.m_informations.addValue(
					QStringLiteral("comment"), QString());
				data.m_informations.addValue(
					QStringLiteral("location"), QString());
				if (!m_keep_signals->isChecked()) {
					ElementData::PlcMasterData plc = data.plcMasterData();
					for (auto &io : plc.ios) {
						io.functionText.clear();
						io.comment.clear();
						io.crossRef.clear();
					}
					data.setPlcMasterData(plc);
				}
			}
			stack.push(new ChangeElementDataCommand(elmt, data));
		}
		target->adjustSceneRect();
	}
	stack.endMacro();
	return true;
}

namespace {
	const QStringList &collectionProtocols()
	{
		static const QStringList list{
			QStringLiteral("common://"), QStringLiteral("custom://"),
			QStringLiteral("company://")};
		return list;
	}

		//Where the other appearances of the element are: in the project,
		//else in a collection of the machine (to be imported)
	QList<ElementsLocation> siblings(QETProject *project, Element *element)
	{
		QList<ElementsLocation> found;
		const QString own = element->location().collectionPath(false);
		const QString prefix = QStringLiteral("import/");
		for (const QString &path : IslandNumbering::appearancePaths(own)) {
			ElementsLocation embedded(path, project);
			if (embedded.exist()) {
				found << embedded;
				continue;
			}
			const QString relative = path.startsWith(prefix)
				? path.mid(prefix.size()) : path;
			for (const QString &protocol : collectionProtocols()) {
				ElementsLocation file(protocol + relative);
				if (file.exist()) {
					found << file;
					break;
				}
			}
		}
		return found;
	}

		//Is a card of this label already drawn in this appearance?
	bool alreadyDrawn(QETProject *project, const QString &label,
					  const ElementsLocation &appearance)
	{
		const QString name = appearance.fileName();
		for (Diagram *d : project->diagrams()) {
			for (QGraphicsItem *item : d->items()) {
				auto *elmt = qgraphicsitem_cast<Element *>(item);
				if (elmt && elmt->actualLabel() == label
					&& elmt->location().fileName() == name) {
					return true;
				}
			}
		}
		return false;
	}

	QList<ElementsLocation> missing(QETProject *project, Element *element)
	{
		QList<ElementsLocation> list;
		const QString label = element->actualLabel();
		if (label.isEmpty()) {
			return list;
		}
		for (const ElementsLocation &location : siblings(project, element)) {
			if (!alreadyDrawn(project, label, location)) {
				list << location;
			}
		}
		return list;
	}
}

/**
	@return true if the element has an appearance which is not drawn yet
*/
bool IslandAppearances::hasMissing(QETProject *project, Element *element)
{
	return project && element && !missing(project, element).isEmpty();
}

/**
	Draws the appearances of the card which are not drawn yet on the
	target folio, by one undo command. The label, the informations and the
	table of inputs and outputs are those of the element.
	@return the number of elements added
*/
int IslandAppearances::create(QETProject *project, Element *element,
							  Diagram *target, QString *error)
{
	const QList<ElementsLocation> wanted = missing(project, element);
	if (wanted.isEmpty() || !target) {
		if (error) {
			*error = QObject::tr("Every appearance of this card is already "
								 "drawn, or it has none.");
		}
		return 0;
	}

	const QStringList copied{
		QStringLiteral("label"), QStringLiteral("formula"),
		QStringLiteral("plant"), QStringLiteral("location"),
		QStringLiteral("function"), QStringLiteral("comment"),
		QStringLiteral("designation"), QStringLiteral("manufacturer"),
		QStringLiteral("manufacturer_reference")};
	const DiagramContext source_info = element->ownInformations();
	const ElementData source_data = element->elementData();

	QList<Element *> created;
	for (ElementsLocation location : wanted) {
		ElementsLocation imported = location.isFileSystem()
			? project->importElement(location) : location;
		if (!imported.exist()) {
			continue;
		}
		int state = 0;
		Element *copy = ElementFactory::Instance()->createElement(
			ElementsLocation(imported.projectCollectionPath()),
			nullptr, &state);
		if (state || !copy) {
			delete copy;
			continue;
		}
		ElementData data = copy->elementData();
		for (const QString &key : copied) {
			if (source_info.contains(key)) {
				data.m_informations.addValue(key, source_info.value(key));
			}
		}
			//Only the channels are shared: the layout (fonts, columns, row
			//height) belongs to the appearance
		auto master = data.plcMasterData();
		master.ios = source_data.plcMasterData().ios;
		data.setPlcMasterData(master);
		copy->setElementData(data);
		created << copy;
	}
	if (created.isEmpty()) {
		if (error) {
			*error = QObject::tr("The appearances could not be loaded.");
		}
		return 0;
	}

		//Side by side, to the right of the element when it is on the target
	const bool same_folio = element->diagram() == target;
	qreal x = element->pos().x();
	if (same_folio) {
		x += element->sceneBoundingRect().width() + 40;
	}
	QUndoStack &stack = target->undoStack();
	stack.beginMacro(QObject::tr("Add the other appearances of %1")
						 .arg(element->actualLabel()));
	for (Element *copy : std::as_const(created)) {
		stack.push(new AddGraphicsObjectCommand(
			copy, target, QPointF(x, element->pos().y())));
		x += copy->sceneBoundingRect().width() + 40;
	}
	stack.endMacro();
	target->adjustSceneRect();
	return created.size();
}

/**
	Asks for the folio (the current one, another, or a new one after the
	current one), then draws the other appearances of the element there.
*/
void IslandAppearances::askAndCreate(QETProject *project, Element *element,
									 Diagram *current, QWidget *parent)
{
	if (!hasMissing(project, element)) {
		QMessageBox::information(
			parent, QObject::tr("Create the other appearances"),
			QObject::tr("Every appearance of this card is already drawn, "
						"or it has none."));
		return;
	}

	const QList<Diagram *> folios = project->diagrams();
	QStringList choices;
	choices << QObject::tr("New folio after the current one");
	int current_row = 0;
	for (int i = 0; i < folios.size(); ++i) {
		choices << QObject::tr("%1: %2").arg(i + 1).arg(folios.at(i)->title());
		if (folios.at(i) == current) {
			current_row = i + 1;
		}
	}
	bool ok = false;
	const QString choice = QInputDialog::getItem(
		parent, QObject::tr("Create the other appearances"),
		QObject::tr("Folio for the other appearances of %1:")
			.arg(element->actualLabel()),
		choices, current_row, false, &ok);
	if (!ok) {
		return;
	}
	Diagram *target = nullptr;
	const int row = choices.indexOf(choice);
	if (row <= 0) {
		target = project->addNewDiagram(project->folioIndex(current) + 1);
	} else {
		target = folios.at(row - 1);
	}
	QString error;
	if (!create(project, element, target, &error)) {
		QMessageBox::warning(parent,
							 QObject::tr("Create the other appearances"),
							 error);
	}
}
