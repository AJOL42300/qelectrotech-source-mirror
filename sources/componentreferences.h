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
#ifndef COMPONENTREFERENCES_H
#define COMPONENTREFERENCES_H

#include <QHash>
#include <QList>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>

#include "diagramcontext.h"

class DuplicateRefItem;
class Element;
class QDomElement;
class QETProject;
class QUndoCommand;

/**
	@brief References between the representations of one component.

	A component (a PLC card "A1.10", a relay "-K1") can be drawn several
	times in a project: as a symbol, as its realistic view, as its annotated
	view, as a table. Those representations share one label (and one
	place, see DuplicateLabels::place()). The cross references of QET
	only go from a parent to its children; nothing tells where else the
	same component is drawn. This part tells it, as page references do.

	Two symbols are the same component when they have the same place and
	the same label (DuplicateLabels::key()). Children of a parent
	(contacts), reports and terminal blocks are not representations of
	a component: the cross reference of the parent already lists the
	children, so only the parents and the plain symbols are compared.

	Only the folios built are read. The index is kept by label and is
	updated for the symbols that changed, not read again.
*/
namespace ComponentReferences
{
	enum class Style {
		Compact,  ///< one line: "-> 3-C2 (realistic), 5-F4 (table)"
		Detailed  ///< one line per reference, with folio, position and view
	};

	enum class Placement {
		Below,    ///< under the symbol
		Right     ///< on the right of the symbol
	};

	/// The way a project shows the references, see QETProject.
	struct Settings {
		bool enabled = false;
		Style style = Style::Compact;
		Placement placement = Placement::Below;
		/// The information fields of the symbols of a component follow
		/// each other, see ComponentReferenceIndex::planSync()
		bool sync_information = false;

		bool operator==(const Settings &other) const {
			return enabled == other.enabled
				&& style == other.style
				&& placement == other.placement
				&& sync_information == other.sync_information;
		}
		bool operator!=(const Settings &other) const {
			return !(*this == other);
		}
	};

	/// true if the information @p key is the same for all the
	/// representations of a component: the designation, the manufacturer
	/// and part numbers, the supplier, the comment, the auxiliary parts...
	/// Not the label itself, its formula, the plant and the location (they
	/// tell which component a label is: see DuplicateLabels::place()), the
	/// function (followed through the wiring), the device type, the
	/// quantity, the terminal data nor the custom properties.
	bool isSyncedKey(const QString &key);

	/// What the information of the representations of a component become
	/// when one of them is edited.
	struct SyncPlan {
		/// The other symbols to change: their information before/after
		QMap<QPointer<Element>, QPair<DiagramContext, DiagramContext>> others;
		/// The edited symbol, once completed: only the synced fields
		/// differ from what it was given.
		DiagramContext mine;
		bool mine_changed = false;
		/// The fields both have, with different values: with
		/// SyncPrefer::Mine they are given to the others, else taken.
		QStringList conflicts;
		bool isEmpty() const { return others.isEmpty() && !mine_changed; }
	};

	enum class SyncPrefer {
		Mine,    ///< the values of the edited symbol win
		Theirs   ///< the values of the others win
	};

	/// <duplicate_references> child of the root of a project: the default
	/// settings when it is missing.
	Settings settingsFromXml(const QDomElement &project_root);
	/// Written only when the settings are not the default ones.
	void settingsToXml(const Settings &settings, QDomElement &project_root);
	/// Name of the XML child.
	QString xmlTag();

	/// The element information which hides the references of a symbol.
	QString hideKey();

	enum class Appearance {
		Schematic,
		Realistic,
		Description,
		Table
	};

	/// The appearance of a symbol, from the name of its file ("x.elmt",
	/// "x_realistic.elmt", "x_description.elmt", "x_table.elmt").
	Appearance appearanceOf(const QString &path);
	QString appearanceName(Appearance appearance);

	/// Another representation of a component.
	struct Reference {
		QPointer<Element> element;
		int folio = 0;          ///< number of the folio, starting at 1
		QString position;       ///< "3-C2": folio, column and row
		Appearance appearance = Appearance::Schematic;
	};

	/// What giving a label to a symbol means.
	struct Verdict {
		QList<Reference> others;  ///< the symbols which have the label
		QStringList parts;        ///< part numbers, if they disagree
		bool conflict = false;    ///< the part numbers disagree

		/// Text for the editor; empty if the label is free.
		QString message() const;
	};

	/// true if the references of the project @p project's symbols are
	/// of any interest: the setting is on.
	bool isEnabled(const QETProject *project);
	/// true if the information of the project's symbols is synchronised
	bool isSyncEnabled(const QETProject *project);
}

/**
	@brief The index of the components of one project.

	One per project, made when it is first asked for (of()). Kept up to date
	by Diagram (a symbol comes or goes) and by the symbols themselves (their
	information or position changes). The items showing the references are
	refreshed in the event loop, once, whatever the number of changes.
*/
class ComponentReferenceIndex : public QObject
{
	Q_OBJECT

	public:
		explicit ComponentReferenceIndex(QETProject *project);
		~ComponentReferenceIndex() override;

		static ComponentReferenceIndex *of(QETProject *project);
		static ComponentReferenceIndex *existing(const QETProject *project);

		QList<ComponentReferences::Reference> referencesOf(
				const Element *element);
		ComponentReferences::Verdict check(
				const Element *element,
				const QString &label,
				const QString &part);
		QString tooltipText(const Element *element);

		/**
			@brief What to change so that the symbols of a component
			agree, after @p element goes from @p old_info to
			@p new_info (its label may be the one of a component
			already drawn).
			A field edited is given to the others; at a new label,
			the empty fields are completed on both sides and the
			fields which differ are reported (conflicts) and
			settled as @p prefer says.
		*/
		ComponentReferences::SyncPlan planSync(
				Element *element,
				const DiagramContext &old_info,
				const DiagramContext &new_info,
				ComponentReferences::SyncPrefer prefer =
					ComponentReferences::SyncPrefer::Mine);
		/**
			@return the command which applies @p plan, to push
			after the one which changed @p element to @p new_info,
			or nullptr if there is nothing to do. One undo step.
		*/
		QUndoCommand *syncCommand(
				Element *element,
				const DiagramContext &new_info,
				const ComponentReferences::SyncPlan &plan);

		void watch(Element *element);
		void forget(Element *element);
		void positionChanged(const Element *element);
		void settingsChanged();
		void refreshNow();
		void flushNow();

		static bool isHidden(const Element *element);

	signals:
		void refreshed();

	private slots:
		void elementInfoChanged();
		void elementDestroyed(QObject *object);
		void flushItems();

	private:
		bool isComponent(Element *element) const;
		QString keyOf(Element *element) const;
		void rebuild();
		void update();
		void remove(const QObject *object);
		void connectTo(Element *element);
		void schedule();
		void dirtyGroup(const QString &key);
		ComponentReferences::Reference referenceTo(Element *element) const;
		DuplicateRefItem *itemFor(Element *element, bool create);

		QETProject *m_project;
		QHash<QString, QList<QObject *>> m_groups;
		QHash<const QObject *, QString> m_keys;
		QSet<QObject *> m_dirty_elements;
		QSet<QObject *> m_stale;
		QSet<QString> m_dirty_groups;
		bool m_rebuild = true;
		bool m_all_items = false;
		QTimer m_timer;
		QHash<const QObject *, QPointer<DuplicateRefItem>> m_items;
};

#endif // COMPONENTREFERENCES_H
