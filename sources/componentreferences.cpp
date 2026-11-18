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
// SPDX-License-Identifier: GPL-2.0-or-later
#include "componentreferences.h"

#include "autoNum/assignvariables.h"
#include "diagram.h"
#include "duplicatelabels.h"
#include "qet.h"
#include "qetgraphicsitem/duplicaterefitem.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"

#include <QCoreApplication>
#include <QDomElement>
#include <QRegularExpression>
#include <QUndoCommand>
#include <algorithm>

#include "undocommand/changeelementinformationcommand.h"

namespace {
	const QChar separator_char(0x1F);

	/// The index of each project, for Diagram to find it for each symbol
	/// it takes without searching the children of the project
	QHash<const QETProject *, ComponentReferenceIndex *> &registry()
	{
		static QHash<const QETProject *, ComponentReferenceIndex *> indexes;
		return indexes;
	}

	QString referenceText(const ComponentReferences::Reference &reference,
			      bool named)
	{
		QString text = QCoreApplication::translate(
			"ComponentReferences", "folio %1").arg(reference.folio);
		if (named) {
			text += QStringLiteral(" (%1)").arg(
				ComponentReferences::appearanceName(
					reference.appearance));
		}
		return text;
	}

	bool anyNamed(const QList<ComponentReferences::Reference> &references,
		      ComponentReferences::Appearance own)
	{
		if (own != ComponentReferences::Appearance::Schematic) return true;
		for (const auto &reference : references) {
			if (reference.appearance
					!= ComponentReferences::Appearance::Schematic)
				return true;
		}
		return false;
	}
}

QString ComponentReferences::xmlTag()
{
	return QStringLiteral("duplicate_references");
}

QString ComponentReferences::hideKey()
{
	return QStringLiteral("hide_duplicate_references");
}

ComponentReferences::Settings ComponentReferences::settingsFromXml(
		const QDomElement &project_root)
{
	Settings settings;
	const QDomElement node = project_root.firstChildElement(xmlTag());
	if (node.isNull()) return settings;
	settings.enabled = node.attribute(QStringLiteral("enabled"))
			== QLatin1String("true");
	settings.style = node.attribute(QStringLiteral("style"))
			== QLatin1String("detailed") ? Style::Detailed : Style::Compact;
	settings.placement = node.attribute(QStringLiteral("placement"))
			== QLatin1String("right") ? Placement::Right : Placement::Below;
	settings.sync_information = node.attribute(
		QStringLiteral("sync_information")) == QLatin1String("true");
	return settings;
}

void ComponentReferences::settingsToXml(const Settings &settings,
					QDomElement &project_root)
{
	if (settings == Settings()) return;
	QDomElement node = project_root.ownerDocument().createElement(xmlTag());
	node.setAttribute(QStringLiteral("enabled"),
			  settings.enabled ? QStringLiteral("true")
					   : QStringLiteral("false"));
	node.setAttribute(QStringLiteral("style"),
			  settings.style == Style::Detailed
				  ? QStringLiteral("detailed")
				  : QStringLiteral("compact"));
	node.setAttribute(QStringLiteral("placement"),
			  settings.placement == Placement::Right
				  ? QStringLiteral("right")
				  : QStringLiteral("below"));
	node.setAttribute(QStringLiteral("sync_information"),
			  settings.sync_information ? QStringLiteral("true")
						    : QStringLiteral("false"));
	project_root.appendChild(node);
}

ComponentReferences::Appearance ComponentReferences::appearanceOf(
		const QString &path)
{
	QString base = path;
	if (base.endsWith(QLatin1String(".elmt"), Qt::CaseInsensitive))
		base.chop(5);
	if (base.endsWith(QLatin1String("_realistic"), Qt::CaseInsensitive))
		return Appearance::Realistic;
	if (base.endsWith(QLatin1String("_description"), Qt::CaseInsensitive))
		return Appearance::Description;
	if (base.endsWith(QLatin1String("_table"), Qt::CaseInsensitive))
		return Appearance::Table;
	return Appearance::Schematic;
}

QString ComponentReferences::appearanceName(Appearance appearance)
{
	switch (appearance) {
	case Appearance::Realistic:
		return QCoreApplication::translate("ComponentReferences",
						   "realistic");
	case Appearance::Description:
		return QCoreApplication::translate("ComponentReferences",
						   "description");
	case Appearance::Table:
		return QCoreApplication::translate("ComponentReferences",
						   "table");
	case Appearance::Schematic: break;
	}
	return QCoreApplication::translate("ComponentReferences",
					   "schematic");
}

bool ComponentReferences::isEnabled(const QETProject *project)
{
	return project && project->componentReferences().enabled;
}

bool ComponentReferences::isSyncEnabled(const QETProject *project)
{
	return project && project->componentReferences().sync_information;
}

bool ComponentReferences::isSyncedKey(const QString &key)
{
	static const QSet<QString> bases = {
		QStringLiteral("designation"),
		QStringLiteral("description"),
		QStringLiteral("manufacturer"),
		QStringLiteral("manufacturer_reference"),
		QStringLiteral("machine_manufacturer_reference"),
		QStringLiteral("supplier"),
		QStringLiteral("unity"),
		QStringLiteral("comment"),
		QStringLiteral("notes"),
		QStringLiteral("model"),
		QStringLiteral("category"),
		QStringLiteral("voltage_rating"),
		QStringLiteral("current_rating"),
		QStringLiteral("width"),
		QStringLiteral("height"),
		QStringLiteral("depth"),
		QStringLiteral("unit_price"),
		QStringLiteral("labour_time")
	};
	static const QRegularExpression auxiliary(
		QStringLiteral("^auxiliary\\d+$"));
	static const QRegularExpression auxiliary_suffix(
		QStringLiteral("_auxiliary\\d+$"));
	if (bases.contains(key) || auxiliary.match(key).hasMatch())
		return true;
	QString base = key;
	base.remove(auxiliary_suffix);
	return base != key && bases.contains(base);
}

QString ComponentReferences::Verdict::message() const
{
	if (others.isEmpty()) return QString();
	const Appearance none = Appearance::Schematic;
	QList<Reference> list = others;
	const bool named = anyNamed(list, none);
	QStringList places;
	for (const Reference &reference : list) {
		places << referenceText(reference, named);
	}
	if (conflict) {
		return QCoreApplication::translate(
				"ComponentReferences",
				"This label is already used (%1) with another "
				"part number (%2): it may be another "
				"component.")
			.arg(places.join(QStringLiteral(", ")),
			     parts.join(QStringLiteral(", ")));
	}
	return QCoreApplication::translate(
			"ComponentReferences",
			"This label is already used (%1): the symbols "
			"will refer to each other as representations of "
			"the same component.")
		.arg(places.join(QStringLiteral(", ")));
}

ComponentReferenceIndex::ComponentReferenceIndex(QETProject *project) :
	QObject(project),
	m_project(project)
{
	registry().insert(project, this);
	m_timer.setSingleShot(true);
	m_timer.setInterval(0);
	connect(&m_timer, &QTimer::timeout,
		this, &ComponentReferenceIndex::flushItems);
	auto invalidate = [this]() {
		m_rebuild = true;
		m_all_items = true;
		schedule();
	};
	connect(project, &QETProject::diagramAdded, this, invalidate);
	connect(project, &QETProject::diagramRemoved, this, invalidate);
	connect(project, &QETProject::projectDiagramsOrderChanged,
		this, invalidate);
}

ComponentReferenceIndex::~ComponentReferenceIndex()
{
	registry().remove(m_project);
}

ComponentReferenceIndex *ComponentReferenceIndex::existing(
		const QETProject *project)
{
	return project ? registry().value(project) : nullptr;
}

ComponentReferenceIndex *ComponentReferenceIndex::of(QETProject *project)
{
	if (!project) return nullptr;
	if (ComponentReferenceIndex *index = existing(project)) return index;
	return new ComponentReferenceIndex(project);
}

bool ComponentReferenceIndex::isHidden(const Element *element)
{
	return QET::infoFlagIsTrue(
		element->elementInformations()
			.value(ComponentReferences::hideKey()).toString());
}

/// A plain symbol or a parent: not a child, a report or a terminal block
bool ComponentReferenceIndex::isComponent(Element *element) const
{
	if (!element->diagram() || element->isDrawingAid()) return false;
	const auto type = element->linkType();
	if (type != Element::Simple && type != Element::Master) return false;
	return !QET::infoFlagIsTrue(
		element->elementInformations()
			.value(QStringLiteral("exclude_from_bom")).toString());
}

QString ComponentReferenceIndex::keyOf(Element *element) const
{
	const QString label = DuplicateLabels::key(
		DuplicateLabels::displayedLabel(element), Qt::CaseSensitive);
	if (label.isEmpty()) return QString();
	return DuplicateLabels::place(element) + separator_char + label;
}

void ComponentReferenceIndex::schedule()
{
	if (!m_timer.isActive()) m_timer.start();
}

void ComponentReferenceIndex::dirtyGroup(const QString &key)
{
	m_dirty_groups.insert(key);
	schedule();
}

void ComponentReferenceIndex::rebuild()
{
	m_groups.clear();
	m_keys.clear();
	m_dirty_elements.clear();
	m_dirty_groups.clear();
	m_rebuild = false;
	for (Diagram *diagram : m_project->builtFolios()) {
		for (Element *element : diagram->elements()) {
			connectTo(element);
			if (!isComponent(element)) continue;
			const QString key = keyOf(element);
			if (key.isEmpty()) continue;
			m_groups[key] << element;
			m_keys.insert(element, key);
		}
	}
}

/// The symbols whose label or kind changed are read again.
void ComponentReferenceIndex::update()
{
	const QSet<QObject *> dirty = m_dirty_elements;
	m_dirty_elements.clear();
	for (QObject *object : dirty) {
		Element *element = static_cast<Element *>(object);
		const QString key = isComponent(element)
				? keyOf(element) : QString();
		const QString old = m_keys.value(object);
		if (key == old) {
			if (!key.isEmpty()) dirtyGroup(key);
			continue;
		}
		if (!old.isEmpty()) {
			m_groups[old].removeAll(object);
			if (m_groups[old].isEmpty()) m_groups.remove(old);
			m_keys.remove(object);
			dirtyGroup(old);
		}
		if (key.isEmpty()) {
			m_stale.insert(object);
		} else {
			m_groups[key] << object;
			m_keys.insert(object, key);
			dirtyGroup(key);
		}
	}
}

void ComponentReferenceIndex::refreshNow()
{
	if (m_rebuild) rebuild();
	else if (!m_dirty_elements.isEmpty()) update();
}

/// Draw the references now, for a caller which has no event loop (CLI)
void ComponentReferenceIndex::flushNow()
{
	m_timer.stop();
	m_rebuild = true;
	m_all_items = true;
	flushItems();
}

void ComponentReferenceIndex::connectTo(Element *element)
{
	connect(element, &Element::elementInfoChange,
		this, &ComponentReferenceIndex::elementInfoChanged,
		Qt::UniqueConnection);
	connect(element, &QObject::destroyed,
		this, &ComponentReferenceIndex::elementDestroyed,
		Qt::UniqueConnection);
}

void ComponentReferenceIndex::watch(Element *element)
{
	connectTo(element);
	if (m_rebuild) return;
	m_dirty_elements.insert(element);
	schedule();
}

void ComponentReferenceIndex::remove(const QObject *object)
{
	QObject *key_object = const_cast<QObject *>(object);
	m_dirty_elements.remove(key_object);
	const QString old = m_keys.take(object);
	if (old.isEmpty()) return;
	m_groups[old].removeAll(key_object);
	if (m_groups[old].isEmpty()) m_groups.remove(old);
	dirtyGroup(old);
}

void ComponentReferenceIndex::forget(Element *element)
{
	if (m_rebuild) return;
	remove(element);
}

void ComponentReferenceIndex::elementInfoChanged()
{
	if (m_rebuild) return;
	if (QObject *object = sender()) {
		m_dirty_elements.insert(object);
		schedule();
	}
}

void ComponentReferenceIndex::elementDestroyed(QObject *object)
{
	m_stale.remove(object);
	m_items.remove(object);
	remove(object);
}

void ComponentReferenceIndex::positionChanged(const Element *element)
{
	if (m_rebuild) return;
	const QString key = m_keys.value(element);
	if (!key.isEmpty() && m_groups.value(key).size() > 1) dirtyGroup(key);
}

void ComponentReferenceIndex::settingsChanged()
{
	m_all_items = true;
	schedule();
}

DuplicateRefItem *ComponentReferenceIndex::itemFor(Element *element,
						   bool create)
{
	QPointer<DuplicateRefItem> item = m_items.value(element);
	if (item) return item;
	if (!create) return nullptr;
	item = new DuplicateRefItem(element);
	m_items.insert(element, item);
	return item;
}

void ComponentReferenceIndex::flushItems()
{
	refreshNow();
	const bool enabled = ComponentReferences::isEnabled(m_project);
	auto refreshGroup = [&](const QList<QObject *> &group) {
		for (QObject *object : group) {
			Element *element = static_cast<Element *>(object);
			if (DuplicateRefItem *item =
					itemFor(element, enabled && group.size() > 1))
				item->refresh();
		}
	};

	if (m_all_items) {
		for (auto it = m_groups.constBegin(); it != m_groups.constEnd(); ++it)
			refreshGroup(it.value());
		const auto items = m_items;
		for (auto it = items.constBegin(); it != items.constEnd(); ++it) {
			if (it.value()) it.value()->refresh();
		}
	} else {
		const QSet<QString> keys = m_dirty_groups;
		for (const QString &key : keys)
			refreshGroup(m_groups.value(key));
	}
	for (QObject *object : std::as_const(m_stale)) {
		if (DuplicateRefItem *item = itemFor(
				static_cast<Element *>(object), false))
			item->refresh();
	}
	m_dirty_groups.clear();
	m_stale.clear();
	m_all_items = false;
	emit refreshed();
}

ComponentReferences::Reference ComponentReferenceIndex::referenceTo(
		Element *element) const
{
	ComponentReferences::Reference reference;
	reference.element = element;
	Diagram *diagram = element->diagram();
	reference.folio = m_project->folioIndex(diagram) + 1;
	QString formula = m_project->defaultXRefProperties()
			.value(QStringLiteral("coil")).masterLabel();
	if (formula.isEmpty()) formula = QStringLiteral("%f-%l%c");
	autonum::sequentialNumbers sequence;
	reference.position = autonum::AssignVariables::formulaToLabel(
		formula, sequence, diagram, element);
	reference.appearance = ComponentReferences::appearanceOf(
		element->location().path());
	return reference;
}

/**
	@return the other representations of the component @p element
	represents: sorted by folio, then position
*/
QList<ComponentReferences::Reference> ComponentReferenceIndex::referencesOf(
		const Element *element)
{
	QList<ComponentReferences::Reference> references;
	refreshNow();
	const QString key = m_keys.value(element);
	if (key.isEmpty()) return references;
	for (QObject *object : m_groups.value(key)) {
		if (object == element) continue;
		Element *other = static_cast<Element *>(object);
		if (!other->diagram()) continue;
		references << referenceTo(other);
	}
	std::stable_sort(
		references.begin(), references.end(),
		[](const ComponentReferences::Reference &a,
		   const ComponentReferences::Reference &b) {
			return a.folio != b.folio ? a.folio < b.folio
						  : a.position < b.position;
		});
	return references;
}

/**
	@return what giving @p label to @p element means: the other symbols
	which have it, and whether their part number agrees with @p part
	(the manufacturer reference of @p element, as it will be).
*/
ComponentReferences::Verdict ComponentReferenceIndex::check(
		const Element *element,
		const QString &label,
		const QString &part)
{
	ComponentReferences::Verdict verdict;
	Element *self = const_cast<Element *>(element);
	refreshNow();
	if (!self || !isComponent(self) || label.contains(QLatin1Char('%')))
		return verdict;
	const QString text = DuplicateLabels::key(label, Qt::CaseSensitive);
	if (text.isEmpty()) return verdict;
	const QString key = DuplicateLabels::place(self) + separator_char + text;
	QStringList parts;
	if (!part.trimmed().isEmpty()) parts << part.trimmed();
	for (QObject *object : m_groups.value(key)) {
		if (object == element) continue;
		Element *other = static_cast<Element *>(object);
		if (!other->diagram()) continue;
		verdict.others << referenceTo(other);
		const QString other_part = other->elementInformations()
			.value(QStringLiteral("manufacturer_reference"))
			.toString().trimmed();
		if (!other_part.isEmpty()) parts << other_part;
	}
	parts.removeDuplicates();
	parts.sort();
	if (parts.size() > 1) {
		verdict.conflict = true;
		verdict.parts = parts;
	}
	std::stable_sort(
		verdict.others.begin(), verdict.others.end(),
		[](const ComponentReferences::Reference &a,
		   const ComponentReferences::Reference &b) {
			return a.folio < b.folio;
		});
	return verdict;
}

/**
	@brief ComponentReferenceIndex::planSync
	@see the header
*/
ComponentReferences::SyncPlan ComponentReferenceIndex::planSync(
		Element *element,
		const DiagramContext &old_info,
		const DiagramContext &new_info,
		ComponentReferences::SyncPrefer prefer)
{
	ComponentReferences::SyncPlan plan;
	if (!element || !ComponentReferences::isSyncEnabled(m_project)
	    || !isComponent(element))
		return plan;
	refreshNow();

	const auto keyOfLabel = [element](const DiagramContext &info) {
		const QString label = info.value(QStringLiteral("label"))
					.toString();
		if (label.contains(QLatin1Char('%'))) return QString();
		const QString text =
			DuplicateLabels::key(label, Qt::CaseSensitive);
		if (text.isEmpty()) return QString();
		return DuplicateLabels::place(element) + separator_char + text;
	};
	const QString key = keyOfLabel(new_info);
	if (key.isEmpty()) return plan;
	const bool joining = key != keyOfLabel(old_info);

	QList<Element *> others;
	for (QObject *object : m_groups.value(key)) {
		if (object == element) continue;
		Element *other = static_cast<Element *>(object);
		if (other->diagram()) others << other;
	}
	if (others.isEmpty()) return plan;

	QSet<QString> keys;
	const auto collect = [&keys](const DiagramContext &info) {
		for (const QString &k : info.keys())
			if (ComponentReferences::isSyncedKey(k)) keys << k;
	};
	collect(old_info);
	collect(new_info);
	for (Element *other : others) collect(other->elementInformations());
	QStringList sorted_keys = keys.values();
	sorted_keys.sort();

	QMap<Element *, DiagramContext> after;
	for (Element *other : others)
		after.insert(other, other->elementInformations());
	plan.mine = new_info;

	const auto text = [](const DiagramContext &info, const QString &k) {
		return info.value(k).toString();
	};
	const auto give = [](DiagramContext &info, const QString &k,
			     const QString &value) {
		if (info.contains(k))
			info.addValue(k, value, info.keyMustShow(k));
		else if (!value.isEmpty())
			info.addValue(k, value);
	};

	for (const QString &k : sorted_keys) {
		const QString mine = text(new_info, k);
		QString wanted = mine;
		if (joining) {
			QStringList theirs;
			for (Element *other : others) {
				const QString value =
					text(after.value(other), k);
				if (!value.trimmed().isEmpty()
				    && !theirs.contains(value))
					theirs << value;
			}
			if (mine.trimmed().isEmpty()) {
				if (theirs.isEmpty()) continue;
				wanted = theirs.first();
			} else if (!theirs.isEmpty()
				   && (theirs.size() > 1
				       || theirs.first() != mine)) {
				plan.conflicts << k;
				if (prefer == ComponentReferences::SyncPrefer
						::Theirs)
					wanted = theirs.first();
			}
		} else if (text(old_info, k) == mine) {
			continue;
		}
		if (wanted != mine) {
			give(plan.mine, k, wanted);
			plan.mine_changed = true;
		}
		for (Element *other : others) {
			if (text(after.value(other), k) != wanted)
				give(after[other], k, wanted);
		}
	}
	for (Element *other : others) {
		if (after.value(other) != other->elementInformations())
			plan.others.insert(
				QPointer<Element>(other),
				qMakePair(other->elementInformations(),
					  after.value(other)));
	}
	return plan;
}

QUndoCommand *ComponentReferenceIndex::syncCommand(
		Element *element,
		const DiagramContext &new_info,
		const ComponentReferences::SyncPlan &plan)
{
	if (plan.isEmpty()) return nullptr;
	auto *macro = new QUndoCommand(QCoreApplication::translate(
		"ComponentReferences",
		"Synchronise the information of the same component"));
	if (!plan.others.isEmpty())
		new ChangeElementInformationCommand(plan.others, macro);
	if (plan.mine_changed)
		new ChangeElementInformationCommand(
			element, new_info, plan.mine, macro);
	return macro;
}

/// Rich text for the tooltip of a symbol, empty if it has no reference

QString ComponentReferenceIndex::tooltipText(const Element *element)
{
	if (!ComponentReferences::isEnabled(m_project) || isHidden(element))
		return QString();
	const QList<ComponentReferences::Reference> references =
		referencesOf(element);
	if (references.isEmpty()) return QString();
	const bool named = anyNamed(
		references,
		ComponentReferences::appearanceOf(element->location().path()));
	QStringList texts;
	for (const auto &reference : references) {
		texts << referenceText(reference, named)
			      + QStringLiteral(", ") + reference.position;
	}
	return QCoreApplication::translate("ComponentReferences",
					   "Also drawn: %1")
		.arg(texts.join(QStringLiteral("; ")));
}
