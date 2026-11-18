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
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "autoNum/autolabel.h"
#include "diagram.h"
#include "diagramevent/diagrameventaddelement.h"
#include "qetapp.h"
#include "qetapptest.h"
#include "qetgraphicsitem/element.h"
#include "qetmessagebox.h"
#include "qetproject.h"
#include "utils/qetsettings.h"

#include <QDomDocument>
#include <QGraphicsSceneMouseEvent>
#include <QUndoStack>
#include <QtTest>

/**
	The label an element is given when it is placed: its prefix and the
	first number the project has not used.
*/
class tst_autolabel : public QObject
{
	Q_OBJECT

	int m_position = 0;

		//A project known to the application, as the editor's are: the
		//placing tool finds the project's elements through it
	struct Registered
	{
		QETProject m_project;
		Registered() { QETApp::registerProject(&m_project); }
		~Registered() { QETApp::unregisterProject(&m_project); }
		QETProject *operator->() { return &m_project; }
		QETProject *get() { return &m_project; }
	};

		//An element definition @p file, in the folder @p folder of the
		//project's collection, with @p label as its own label if any
	static bool define(
		QETProject *project,
		const QString &folder,
		const QString &file,
		const QString &link_type,
		const QString &kind,
		const QString &label = QString())
	{
		auto *collection = project->embeddedElementCollection();
		QString parent;
		for (const QString &part : folder.split(QLatin1Char('/')))
		{
			if (!parent.isEmpty() && !collection->createDir(parent, part, NamesList()))
				return false;
			parent += (parent.isEmpty() ? QString() : QStringLiteral("/")) + part;
		}
		if (collection->exist(folder + QLatin1Char('/') + file + QStringLiteral(".elmt")))
			return true;
		const QString information = label.isEmpty()
			? QString()
			: QStringLiteral(
				"<elementInformations><elementInformation name=\"label\" "
				"show=\"1\">%1</elementInformation></elementInformations>")
				  .arg(label);
		QDomDocument definition;
		if (!definition.setContent(QString(
				R"(<definition type="element" link_type="%1" uuid="%2" width="40" height="40" hotspot_x="20" hotspot_y="20">
<names><name lang="en">%3</name></names>
%4
<kindInformations><kindInformation name="type">%5</kindInformation></kindInformations>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal x="0" y="10" orientation="s" uuid="%6"/></description></definition>)")
				.arg(link_type, QUuid::createUuid().toString(), file, information,
					 kind, QUuid::createUuid().toString())))
			return false;
		return collection->addElementDefinition(
			folder, file, definition.documentElement());
	}

	static QString path(const QString &folder, const QString &file)
	{
		return QStringLiteral("embed://") + folder + QLatin1Char('/') + file
			+ QStringLiteral(".elmt");
	}

		//One click with the placing tool: one element, the one it returns
	Element *place(Diagram *diagram, const QString &location_path)
	{
		ElementsLocation location(location_path, diagram->project());
		DiagramEventAddElement tool(location, diagram);
		const QList<Element *> before = diagram->elements();
		const QPointF at(100 + 80 * (m_position % 8), 150 + 100 * (m_position / 8));
		++m_position;
		for (QEvent::Type type : {QEvent::GraphicsSceneMouseMove,
								  QEvent::GraphicsSceneMousePress,
								  QEvent::GraphicsSceneMouseRelease})
		{
			QGraphicsSceneMouseEvent event(type);
			event.setButton(Qt::LeftButton);
			event.setButtons(type == QEvent::GraphicsSceneMousePress
								 ? Qt::LeftButton : Qt::NoButton);
			event.setScenePos(at);
			if (type == QEvent::GraphicsSceneMouseMove) tool.mouseMoveEvent(&event);
			else if (type == QEvent::GraphicsSceneMousePress) tool.mousePressEvent(&event);
			else tool.mouseReleaseEvent(&event);
		}
		for (Element *element : diagram->elements())
			if (!before.contains(element) && element != tool.pendingElement())
				return element;
		return nullptr;
	}

	static QString labelOf(Element *element)
	{
		return element->elementInformations()
			.value(QStringLiteral("label")).toString();
	}

  private slots:
	void initTestCase()
	{
		QETProject::setBackupEnabled(false);
		QET::QetMessageBox::setNonInteractive(true);
	}

	void init()
	{
		m_position = 0;
		QetSettings::setAutoLabelOnPlacement(true);
	}

	void cleanupTestCase() { QetSettings::setAutoLabelOnPlacement(true); }

	void nextFreeLabelTakesTheSmallestFreeNumber()
	{
		const QString k = QStringLiteral("K");
		QCOMPARE(autolabel::nextFreeLabel(k, {}), QStringLiteral("K1"));
		QCOMPARE(autolabel::nextFreeLabel(k, {"K1", "K2"}), QStringLiteral("K3"));
		QCOMPARE(autolabel::nextFreeLabel(k, {"K1", "K3"}), QStringLiteral("K2"));
			//Another prefix, a card number or a longer label is not K1
		QCOMPARE(autolabel::nextFreeLabel(k, {"Q1", "K1.10", "-K1", "K10"}),
				 QStringLiteral("K1"));
		QCOMPARE(autolabel::nextFreeLabel(QStringLiteral("A"), {"A1", "A1.10"}),
				 QStringLiteral("A2"));
		QVERIFY(autolabel::nextFreeLabel(QString(), {}).isEmpty());
	}

	void prefixByKindAndFolder()
	{
		using autolabel::fallbackPrefix;
		const QString none;
		QCOMPARE(fallbackPrefix(ElementData::Master, ElementData::Coil, none), QStringLiteral("K"));
		QCOMPARE(fallbackPrefix(ElementData::Master, ElementData::Protection, none), QStringLiteral("Q"));
		QCOMPARE(fallbackPrefix(ElementData::Master, ElementData::Commutator, none), QStringLiteral("S"));
		QCOMPARE(fallbackPrefix(ElementData::Master, ElementData::PLC, none), QStringLiteral("A"));
		QCOMPARE(fallbackPrefix(ElementData::Terminal, ElementData::Coil, none), QStringLiteral("X"));
		QVERIFY(fallbackPrefix(ElementData::Simple, ElementData::Coil, none).isEmpty());
		QCOMPARE(fallbackPrefix(ElementData::Simple, ElementData::Coil,
								QStringLiteral("15_beckhoff_ethercat")),
				 QStringLiteral("A"));
		QCOMPARE(autolabel::collectionFolder(
					 QStringLiteral("embed://import/15_beckhoff_ethercat/el1xxx/el1008.elmt")),
				 QStringLiteral("15_beckhoff_ethercat"));
		QVERIFY(autolabel::collectionFolder(QStringLiteral("embed://import/coil.elmt")).isEmpty());
	}

	void placedElementsAreLabelled()
	{
		Registered project;
		Diagram *folio = project->addNewDiagram();
		QVERIFY(define(project.get(), "import", "coil", "master", "coil"));
		QVERIFY(define(project.get(), "import", "plain", "simple", "simple"));

		Element *first = place(folio, path("import", "coil"));
		QVERIFY(first);
		QCOMPARE(labelOf(first), QStringLiteral("K1"));
		QCOMPARE(first->actualLabel(), QStringLiteral("K1"));
		Element *second = place(folio, path("import", "coil"));
		QVERIFY(second);
		QCOMPARE(labelOf(second), QStringLiteral("K2"));

			//A symbol which says nothing about what it is stays as it is
		Element *plain = place(folio, path("import", "plain"));
		QVERIFY(plain);
		QVERIFY(labelOf(plain).isEmpty());
	}

	void freedNumberIsUsedAgain()
	{
		Registered project;
		Diagram *folio = project->addNewDiagram();
		QVERIFY(define(project.get(), "import", "coil", "master", "coil"));
		Element *k1 = place(folio, path("import", "coil"));
		Element *k2 = place(folio, path("import", "coil"));
		Element *k3 = place(folio, path("import", "coil"));
		QVERIFY(k1 && k2 && k3);
		QCOMPARE(labelOf(k3), QStringLiteral("K3"));

		auto info = k2->elementInformations();
		info.addValue(QStringLiteral("label"), QStringLiteral("F7"));
		k2->setElementInformations(info);
		Element *again = place(folio, path("import", "coil"));
		QVERIFY(again);
		QCOMPARE(labelOf(again), QStringLiteral("K2"));
	}

	void givenLabelIsNotOverwritten()
	{
		Registered project;
		Diagram *folio = project->addNewDiagram();
		QVERIFY(define(project.get(), "import", "named", "master", "coil", "K7"));
		Element *named = place(folio, path("import", "named"));
		QVERIFY(named);
		QCOMPARE(labelOf(named), QStringLiteral("K7"));
		QVERIFY(!named->giveAutoLabel());
		QCOMPARE(labelOf(named), QStringLiteral("K7"));
	}

	void slaveKeepsToItsMaster()
	{
		Registered project;
		Diagram *folio = project->addNewDiagram();
		QVERIFY(define(project.get(), "import", "contact", "slave", "simple"));
		Element *contact = place(folio, path("import", "contact"));
		QVERIFY(contact);
		QVERIFY(labelOf(contact).isEmpty());
	}

	void beckhoffCardsAreA()
	{
		Registered project;
		Diagram *folio = project->addNewDiagram();
		const QString folder = QStringLiteral("import/15_beckhoff_ethercat/el1xxx_digital_in");
		QVERIFY(define(project.get(), folder, "el1008", "master", "plc"));
		Element *card = place(folio, path(folder, "el1008"));
		Element *other = place(folio, path(folder, "el1008"));
		QVERIFY(card && other);
		QCOMPARE(labelOf(card), QStringLiteral("A1"));
		QCOMPARE(labelOf(other), QStringLiteral("A2"));
	}

	void settingOffLeavesTheLabelEmpty()
	{
		Registered project;
		Diagram *folio = project->addNewDiagram();
		QVERIFY(define(project.get(), "import", "coil", "master", "coil"));
		QetSettings::setAutoLabelOnPlacement(false);
		Element *coil = place(folio, path("import", "coil"));
		QVERIFY(coil);
		QVERIFY(labelOf(coil).isEmpty());
	}

	void undoTakesTheLabelWithTheElement()
	{
		Registered project;
		Diagram *folio = project->addNewDiagram();
		QVERIFY(define(project.get(), "import", "coil", "master", "coil"));
		Element *first = place(folio, path("import", "coil"));
		QVERIFY(first);
		QCOMPARE(labelOf(first), QStringLiteral("K1"));
		const int elements = folio->elements().size();

		Element *second = place(folio, path("import", "coil"));
		QVERIFY(second);
		QCOMPARE(labelOf(second), QStringLiteral("K2"));
		folio->undoStack().undo();
		QCOMPARE(folio->elements().size(), elements);

			//K2 is free again
		Element *third = place(folio, path("import", "coil"));
		QVERIFY(third);
		QCOMPARE(labelOf(third), QStringLiteral("K2"));
	}
};

QET_APP_TEST_MAIN(tst_autolabel)
#include "tst_autolabel.moc"
