// SPDX-License-Identifier: GPL-2.0-or-later
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "componentreferences.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/duplicaterefitem.h"
#include "qetgraphicsitem/element.h"
#include "qetmessagebox.h"
#include "qetproject.h"
#include "undocommand/changeelementinformationcommand.h"

#include <QDomDocument>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QUndoStack>

using namespace ComponentReferences;

class tst_componentreferences : public QObject
{
	Q_OBJECT
	QTemporaryDir m_definitions;
	int			  m_position = 0;

	// A symbol of the link type @p link, stored as @p name in the
	// project: "name_realistic" is the realistic appearance of "name".
	Element* symbol(
		Diagram*	   diagram,
		const QString& label,
		const QString& link = QStringLiteral("simple"),
		const QString& name = QString())
	{
		const QString file_name = name.isEmpty() ? link : name;
		const QString path = m_definitions.filePath(file_name + ".elmt");
		if (! QFile::exists(path))
		{
			QFile file(path);
			if (! file.open(QIODevice::WriteOnly)) return nullptr;
			file.write(
				QString(
					R"(<definition type="element" link_type="%1" uuid="%2" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Test symbol</name></names>
<kindInformations><kindInformation name="type">coil</kindInformation></kindInformations>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal x="0" y="-20" orientation="n" uuid="%3"/><terminal x="0" y="20" orientation="s" uuid="%4"/>
</description></definition>)")
					.arg(
						link,
						QUuid::createUuid().toString(),
						QUuid::createUuid().toString(),
						QUuid::createUuid().toString())
					.toUtf8());
		}
		auto* collection = diagram->project()->embeddedElementCollection();
		if (! collection->exist("import/" + file_name + ".elmt"))
		{
			QFile		 file(path);
			QDomDocument definition;
			if (! file.open(QIODevice::ReadOnly)
				|| ! definition.setContent(&file))
				return nullptr;
			if (! collection->addElementDefinition(
					"import",
					file_name,
					definition.documentElement()))
				return nullptr;
		}
		int		 state	 = 0;
		Element* element = ElementFactory::Instance()->createElement(
			ElementsLocation(
				"embed://import/" + file_name + ".elmt",
				diagram->project()),
			nullptr,
			&state);
		if (state || ! element) return nullptr;
		diagram->addItem(element);
		const int position = m_position++;
		element->setPos(100 + 80 * (position % 8), 150 + 100 * (position / 8));
		setInformation(element, "label", label);
		return element;
	}

	static void setInformation(
		Element*	   element,
		const QString& name,
		const QString& value)
	{
		auto info = element->elementInformations();
		info.addValue(name, value);
		if (name == QLatin1String("label"))
			info.remove(QStringLiteral("formula"));
		element->setElementInformations(info);
	}

	static QList<Element*> others(Element* element)
	{
		QList<Element*> list;
		auto* index = ComponentReferenceIndex::of(element->diagram()->project());
		for (const Reference& reference : index->referencesOf(element))
			list << reference.element.data();
		return list;
	}

	static DuplicateRefItem* itemOf(Element* element)
	{
		for (QGraphicsItem* child : element->childItems())
			if (child->type() == DuplicateRefItem::Type)
				return static_cast<DuplicateRefItem*>(child);
		return nullptr;
	}

	static void waitForRefresh(QETProject* project)
	{
		auto* index = ComponentReferenceIndex::of(project);
		QSignalSpy spy(index, &ComponentReferenceIndex::refreshed);
		QVERIFY(spy.wait(2000));
	}

  private slots:
	void initTestCase()
	{
		QVERIFY(m_definitions.isValid());
		QETProject::setBackupEnabled(false);
		QET::QetMessageBox::setNonInteractive(true);
	}

	void init() { m_position = 0; }

	void sameLabelOnThreeFolios()
	{
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		auto*	   third  = project.addNewDiagram();
		auto*	   a	  = symbol(first, "A1.10");
		auto*	   b	  = symbol(second, "A1.10");
		auto*	   c	  = symbol(third, "A1.10");
		auto*	   other  = symbol(third, "A1.11");
		QVERIFY(a && b && c && other);

		QCOMPARE(others(a), (QList<Element*>{b, c}));
		QCOMPARE(others(b), (QList<Element*>{a, c}));
		QCOMPARE(others(c), (QList<Element*>{a, b}));
		QVERIFY(others(other).isEmpty());

		auto* index = ComponentReferenceIndex::of(&project);
		const auto references = index->referencesOf(a);
		QCOMPARE(references.first().folio, 2);
		QCOMPARE(references.last().folio, 3);
	}

	void childrenAreNotRepresentations()
	{
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		auto*	   master = symbol(first, "-K1", "master");
		auto*	   slave  = symbol(second, "-K1", "slave");
		auto*	   copy	  = symbol(second, "-K1", "master");
		QVERIFY(master && slave && copy);

		// The child belongs to the cross reference of the parent
		QCOMPARE(others(master), (QList<Element*>{copy}));
		QVERIFY(others(slave).isEmpty());
	}

	void appearanceFromTheFileName()
	{
		QCOMPARE(
			appearanceOf("embed://import/card.elmt"),
			Appearance::Schematic);
		QCOMPARE(
			appearanceOf("embed://import/card_realistic.elmt"),
			Appearance::Realistic);
		QCOMPARE(
			appearanceOf("embed://import/card_description.elmt"),
			Appearance::Description);
		QCOMPARE(
			appearanceOf("embed://import/card_table.elmt"),
			Appearance::Table);

		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		auto*	   a = symbol(first, "A1.10", "simple", "card");
		auto*	   b = symbol(second, "A1.10", "simple", "card_realistic");
		QVERIFY(a && b);
		const auto references =
			ComponentReferenceIndex::of(&project)->referencesOf(a);
		QCOMPARE(references.size(), 1);
		QCOMPARE(references.first().appearance, Appearance::Realistic);
	}

	void relabelUpdatesBothSides()
	{
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		auto*	   a	  = symbol(first, "-K1");
		auto*	   b	  = symbol(second, "-K2");
		QVERIFY(others(a).isEmpty());

		setInformation(b, "label", "-K1");
		QCOMPARE(others(a), (QList<Element*>{b}));
		QCOMPARE(others(b), (QList<Element*>{a}));

		setInformation(b, "label", "-K3");
		QVERIFY(others(a).isEmpty());
		QVERIFY(others(b).isEmpty());
	}

	void secondThenThirdSymbolWithTheSameId()
	{
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		auto*	   third  = project.addNewDiagram();
		auto*	   a	  = symbol(first, "A1.10");
		auto*	   b	  = symbol(second, "A1.20");
		auto*	   c	  = symbol(third, "A1.30");
		QVERIFY(others(a).isEmpty());

		setInformation(b, "label", "A1.10");
		QCOMPARE(others(a), (QList<Element*>{b}));
		QCOMPARE(others(b), (QList<Element*>{a}));

		setInformation(c, "label", "A1.10");
		QCOMPARE(others(a), (QList<Element*>{b, c}));
		QCOMPARE(others(b), (QList<Element*>{a, c}));
		QCOMPARE(others(c), (QList<Element*>{a, b}));
	}

	void undoAndRedoKeepTheReferences()
	{
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		auto*	   a	  = symbol(first, "-K1");
		auto*	   b	  = symbol(second, "-K2");
		QVERIFY(a && b);

		const DiagramContext old_info = b->elementInformations();
		DiagramContext		 new_info = old_info;
		new_info.addValue("label", "-K1");
		new_info.remove("formula");
		project.undoStack()->push(
			new ChangeElementInformationCommand(b, old_info, new_info));
		QCOMPARE(others(a), (QList<Element*>{b}));

		project.undoStack()->undo();
		QVERIFY(others(a).isEmpty());
		QVERIFY(others(b).isEmpty());

		project.undoStack()->redo();
		QCOMPARE(others(a), (QList<Element*>{b}));
		QCOMPARE(others(b), (QList<Element*>{a}));
	}

	void removedSymbolIsForgotten()
	{
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		auto*	   a	  = symbol(first, "-K1");
		auto*	   b	  = symbol(second, "-K1");
		QCOMPARE(others(a), (QList<Element*>{b}));

		second->removeItem(b);
		QVERIFY(others(a).isEmpty());
		second->addItem(b);
		QCOMPARE(others(a), (QList<Element*>{b}));
		delete b;
		QVERIFY(others(a).isEmpty());
	}

	void conflictOfPartNumbers()
	{
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		auto*	   a	  = symbol(first, "A1.10");
		auto*	   b	  = symbol(second, "A1.20");
		setInformation(a, "manufacturer_reference", "EL1008");
		auto* index = ComponentReferenceIndex::of(&project);

		// Free label: nothing to say
		auto verdict = index->check(b, "A1.30", "EL2008");
		QVERIFY(verdict.others.isEmpty());
		QVERIFY(verdict.message().isEmpty());

		// Same component drawn again: legitimate, with or without part
		verdict = index->check(b, "A1.10", "EL1008");
		QCOMPARE(verdict.others.size(), 1);
		QVERIFY(! verdict.conflict);
		QVERIFY(! verdict.message().isEmpty());
		verdict = index->check(b, "A1.10", QString());
		QVERIFY(! verdict.conflict);

		// Another part number: probably two components
		verdict = index->check(b, "A1.10", "EL2008");
		QVERIFY(verdict.conflict);
		QCOMPARE(verdict.parts, (QStringList{"EL1008", "EL2008"}));
		QVERIFY(verdict.message().contains("EL2008"));
	}

	void settingsAreSerialized()
	{
		QDomDocument document;
		auto		 root = document.createElement("project");
		document.appendChild(root);

		// Not written while the default: old files stay as they were
		QVERIFY(! settingsFromXml(root).enabled);
		settingsToXml(Settings(), root);
		QVERIFY(root.firstChildElement(xmlTag()).isNull());

		Settings settings;
		settings.enabled   = true;
		settings.style	   = Style::Detailed;
		settings.placement = Placement::Right;
		settings.sync_information = true;
		settingsToXml(settings, root);
		QVERIFY(! root.firstChildElement(xmlTag()).isNull());
		QCOMPARE(settingsFromXml(root), settings);
	}

	// What the information editor does: the edit of the symbol and the
	// synchronisation, in one undo step.
	SyncPlan relabel(
		QETProject*	   project,
		Element*	   element,
		const QString& label,
		SyncPrefer	   prefer = SyncPrefer::Mine)
	{
		auto*		   index = ComponentReferenceIndex::of(project);
		DiagramContext old_info = element->elementInformations();
		DiagramContext new_info = old_info;
		new_info.addValue("label", label);
		new_info.remove("formula");
		SyncPlan plan = index->planSync(element, old_info, new_info, prefer);
		editWith(project, element, old_info, new_info, plan);
		return plan;
	}

	void editWith(
		QETProject*			project,
		Element*			element,
		const DiagramContext& old_info,
		const DiagramContext& new_info,
		const SyncPlan&		plan)
	{
		auto*	  index = ComponentReferenceIndex::of(project);
		QUndoStack* stack = project->undoStack();
		stack->beginMacro("edit");
		stack->push(
			new ChangeElementInformationCommand(element, old_info, new_info));
		const DiagramContext done = element->elementInformations();
		const auto after = index->planSync(element, old_info, done);
		if (auto* sync = index->syncCommand(element, done,
				plan.conflicts.isEmpty() ? after : plan))
			stack->push(sync);
		stack->endMacro();
	}

	static void enableSync(QETProject* project)
	{
		Settings settings;
		settings.sync_information = true;
		project->setComponentReferences(settings);
	}

	static QString info(Element* element, const QString& key)
	{
		return element->elementInformations().value(key).toString();
	}

	void synchronisationIsOffByDefault()
	{
		QETProject project;
		auto*	   a = symbol(project.addNewDiagram(), "A1.10");
		auto*	   b = symbol(project.addNewDiagram(), "A1.20");
		setInformation(a, "designation", "Input module");
		const auto plan = relabel(&project, b, "A1.10");
		QVERIFY(plan.isEmpty());
		QVERIFY(info(b, "designation").isEmpty());
	}

	void emptyFieldsAreCompleted()
	{
		QETProject project;
		enableSync(&project);
		auto* a = symbol(project.addNewDiagram(), "A1.10");
		auto* b = symbol(project.addNewDiagram(), "A1.20");
		setInformation(a, "manufacturer_reference", "EL1008");
		setInformation(a, "comment", "Spare");
		setInformation(b, "designation", "Input module");

		const auto plan = relabel(&project, b, "A1.10");
		QVERIFY(plan.conflicts.isEmpty());
		QCOMPARE(info(a, "designation"), QString("Input module"));
		QCOMPARE(info(b, "manufacturer_reference"), QString("EL1008"));
		QCOMPARE(info(b, "comment"), QString("Spare"));
		QCOMPARE(info(b, "label"), QString("A1.10"));

		// One undo gives everything back
		project.undoStack()->undo();
		QCOMPARE(info(b, "label"), QString("A1.20"));
		QVERIFY(info(a, "designation").isEmpty());
		QVERIFY(info(b, "manufacturer_reference").isEmpty());
		project.undoStack()->redo();
		QCOMPARE(info(a, "designation"), QString("Input module"));
		QCOMPARE(info(b, "comment"), QString("Spare"));
	}

	void thirdSymbolFollowsTheOthers()
	{
		QETProject project;
		enableSync(&project);
		auto* a = symbol(project.addNewDiagram(), "A1.10");
		auto* b = symbol(project.addNewDiagram(), "A1.20");
		auto* c = symbol(project.addNewDiagram(), "A1.30");
		setInformation(a, "manufacturer", "Beckhoff");
		relabel(&project, b, "A1.10");
		QCOMPARE(info(b, "manufacturer"), QString("Beckhoff"));
		relabel(&project, c, "A1.10");
		QCOMPARE(info(c, "manufacturer"), QString("Beckhoff"));
		QCOMPARE(others(c), (QList<Element*>{a, b}));
	}

	void differentFieldsAreReported()
	{
		QETProject project;
		enableSync(&project);
		auto* a = symbol(project.addNewDiagram(), "A1.10");
		auto* b = symbol(project.addNewDiagram(), "A1.20");
		setInformation(a, "designation", "Old name");
		setInformation(b, "designation", "New name");

		auto plan = relabel(&project, b, "A1.10", SyncPrefer::Mine);
		QCOMPARE(plan.conflicts, QStringList{"designation"});
		QCOMPARE(info(a, "designation"), QString("New name"));
		project.undoStack()->undo();
		QCOMPARE(info(a, "designation"), QString("Old name"));

		plan = relabel(&project, b, "A1.10", SyncPrefer::Theirs);
		QCOMPARE(plan.conflicts, QStringList{"designation"});
		QCOMPARE(info(a, "designation"), QString("Old name"));
		QCOMPARE(info(b, "designation"), QString("Old name"));
	}

	void editedFieldIsGivenToTheOthers()
	{
		QETProject project;
		enableSync(&project);
		auto* a = symbol(project.addNewDiagram(), "A1.10");
		auto* b = symbol(project.addNewDiagram(), "A1.10");
		auto* c = symbol(project.addNewDiagram(), "A1.10");
		const int steps = project.undoStack()->count();

		DiagramContext old_info = a->elementInformations();
		DiagramContext new_info = old_info;
		new_info.addValue("comment", "Spare part");
		auto plan = ComponentReferenceIndex::of(&project)
				->planSync(a, old_info, new_info);
		QVERIFY(plan.conflicts.isEmpty());
		editWith(&project, a, old_info, new_info, plan);
		QCOMPARE(info(b, "comment"), QString("Spare part"));
		QCOMPARE(info(c, "comment"), QString("Spare part"));
		QCOMPARE(project.undoStack()->count(), steps + 1);

		project.undoStack()->undo();
		QVERIFY(info(a, "comment").isEmpty());
		QVERIFY(info(c, "comment").isEmpty());
	}

	void labelFunctionAndQuantityAreNotShared()
	{
		QETProject project;
		enableSync(&project);
		auto* a = symbol(project.addNewDiagram(), "A1.10");
		auto* b = symbol(project.addNewDiagram(), "A1.20");
		setInformation(a, "function", "Valve");
		setInformation(a, "quantity", "2");
		setInformation(a, "formula", "");
		relabel(&project, b, "A1.10");
		QVERIFY(info(b, "function").isEmpty());
		QVERIFY(info(b, "quantity").isEmpty());
		QCOMPARE(info(a, "function"), QString("Valve"));
		QVERIFY(isSyncedKey("designation"));
		QVERIFY(isSyncedKey("auxiliary1"));
		QVERIFY(! isSyncedKey("label"));
		QVERIFY(! isSyncedKey("function"));
		QVERIFY(! isSyncedKey("quantity"));
		QVERIFY(! isSyncedKey("formula"));
		QVERIFY(! isSyncedKey("plant"));
		QVERIFY(! isSyncedKey("location"));
	}

	void projectKeepsItsSettings()
	{
		QETProject project;
		QVERIFY(! project.componentReferences().enabled);
		Settings settings;
		settings.enabled = true;
		settings.style	 = Style::Detailed;
		project.setComponentReferences(settings);
		QCOMPARE(project.componentReferences(), settings);

		QDomDocument document = project.toXml();
		QCOMPARE(settingsFromXml(document.documentElement()), settings);

		// A project which keeps the default writes nothing for it
		QETProject plain;
		QVERIFY(plain.toXml().documentElement()
				.firstChildElement(xmlTag()).isNull());
	}

	void itemShowsTheOtherPlaces()
	{
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		auto*	   third  = project.addNewDiagram();
		auto*	   a	  = symbol(first, "A1.10", "simple", "card");
		auto*	   b	  = symbol(second, "A1.10", "simple", "card_realistic");
		auto*	   c	  = symbol(third, "A1.10", "simple", "card_table");
		QVERIFY(a && b && c);

		// Off by default: nothing is drawn
		QVERIFY(! itemOf(a));
		Settings settings;
		settings.enabled = true;
		project.setComponentReferences(settings);
		waitForRefresh(&project);

		auto* item = itemOf(a);
		QVERIFY(item);
		QCOMPARE(item->referenceCount(), 2);
		QVERIFY(item->isVisible() || ! item->text().isEmpty());
		QVERIFY(item->text().contains("2"));
		QVERIFY(item->text().contains("3"));
		QVERIFY(item->text().contains(translatedName(Appearance::Table)));

		// A label change reaches the item of the others
		setInformation(c, "label", "A1.11");
		waitForRefresh(&project);
		QCOMPARE(itemOf(a)->referenceCount(), 1);
		QVERIFY(! itemOf(a)->text().contains("3"));

		// The symbol alone has no more reference to show
		QVERIFY(! itemOf(c) || itemOf(c)->referenceCount() == 0);

		// Hidden for one symbol
		setInformation(b, hideKey(), "true");
		waitForRefresh(&project);
		QVERIFY(! itemOf(b) || ! itemOf(b)->isVisible());
	}

	void detailedStyleHasOneLinePerReference()
	{
		QETProject project;
		auto*	   first  = project.addNewDiagram();
		auto*	   second = project.addNewDiagram();
		auto*	   third  = project.addNewDiagram();
		auto*	   a	  = symbol(first, "A1.10");
		symbol(second, "A1.10");
		symbol(third, "A1.10");

		Settings settings;
		settings.enabled = true;
		project.setComponentReferences(settings);
		waitForRefresh(&project);
		const QString compact = itemOf(a)->text();
		QVERIFY(! compact.contains('\n'));

		settings.style = Style::Detailed;
		project.setComponentReferences(settings);
		waitForRefresh(&project);
		QCOMPARE(itemOf(a)->text().count('\n'), 1);
	}

  private:
	static QString translatedName(Appearance appearance)
	{
		return appearanceName(appearance);
	}
};

QET_APP_TEST_MAIN(tst_componentreferences)
#include "tst_componentreferences.moc"
