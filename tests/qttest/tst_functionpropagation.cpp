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
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "functionpropagation.h"
#include "qetapptest.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "undocommand/changeelementdatacommand.h"
#include "undocommand/changeelementinformationcommand.h"
#include <QDomDocument>
#include <QFile>
#include <QSettings>
#include <QSignalSpy>
#include <QtTest>

/**
	The function of a channel of an EPP module follows the cables, the T
	couplers and the Y distributors to the valves, and back, channel by
	channel, when the setting is on.
*/
class tst_functionpropagation : public QObject
{
	Q_OBJECT

	static QString path(const QString &relative)
	{
		return QStringLiteral(QET_EXTRA_ELEMENTS_DIR "/") + relative;
	}

	static Element *place(Diagram *diagram,
			      const QString &relative,
			      const QPointF &pos)
	{
		const QString name = QFileInfo(relative).baseName();
		auto *collection =
			diagram->project()->embeddedElementCollection();
		if (!collection->exist(
			    QStringLiteral("import/%1.elmt").arg(name))) {
			QFile file(path(relative));
			if (!file.open(QIODevice::ReadOnly)) {
				return nullptr;
			}
			QDomDocument definition;
			if (!definition.setContent(&file)) {
				return nullptr;
			}
			if (!collection->addElementDefinition(
				    QStringLiteral("import"), name,
				    definition.documentElement())) {
				return nullptr;
			}
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(
				QStringLiteral("embed://import/%1.elmt")
					.arg(name),
				diagram->project()),
			nullptr, &state);
		if (state || !element) {
			return nullptr;
		}
		diagram->addItem(element);
		element->setPos(pos);
		return element;
	}

	static Terminal *terminal(Element *element, const QString &name)
	{
		for (Terminal *t : element->terminals()) {
			if (t->name() == name) {
				return t;
			}
		}
		return nullptr;
	}

	static void wire(Diagram *diagram,
			 Element *a, const QString &ta,
			 Element *b, const QString &tb)
	{
		Terminal *first = terminal(a, ta);
		Terminal *second = terminal(b, tb);
		QVERIFY(first && second);
		diagram->addItem(new Conductor(first, second));
	}

	static const QString epp()
	{
		return QStringLiteral(
			"10_electric/20_manufacturers_articles/beckhoff/epp_box/epp2338/epp2338.elmt");
	}
	static const QString cable()
	{
		return QStringLiteral(
			"18_murrelektronik_m12/m12_cable/m12_cable_1m.elmt");
	}
	static const QString tee()
	{
		return QStringLiteral(
			"18_murrelektronik_m12/m12_t_coupler/m12_t_coupler.elmt");
	}
	static const QString wye()
	{
		return QStringLiteral("18_murrelektronik_m12/"
				      "m12_y_distributor/m12_y_0_3m.elmt");
	}
	static const QString coil()
	{
		return QStringLiteral("18_murrelektronik_m12/"
				      "m12_valve_coil/valve_coil_bi_18.elmt");
	}

	static void setEnabled(bool on)
	{
		QSettings().setValue(FunctionPropagation::settingsKey(), on);
	}

	static QString functionOfChannel(Element *module, int index)
	{
		return module->elementData().m_plc_master_data.ios
				.at(index).functionText;
	}

	static QString functionOfValve(Element *valve)
	{
		return valve->elementInformations()
				.value(QStringLiteral("function")).toString();
	}

		/// Sets the function of a channel of the module as the user does
	static void editChannel(Diagram *diagram,
				Element *module,
				int index,
				const QString &text)
	{
		ElementData data = module->elementData();
		data.m_plc_master_data.ios[index].functionText = text;
		diagram->undoStack().push(
			new ChangeElementDataCommand(module, data));
	}

	static void editValve(Diagram *diagram,
			      Element *valve,
			      const QString &text)
	{
		DiagramContext before = valve->elementInformations();
		DiagramContext after = before;
		after.addValue(QStringLiteral("function"), text);
		diagram->undoStack().push(
			new ChangeElementInformationCommand(
				valve, before, after));
	}

		/// The module X01 -> cable -> T -> two valves
	struct Chain
	{
		QETProject project;
		Diagram *diagram = nullptr;
		Element *module = nullptr;
		Element *cable = nullptr;
		Element *tee = nullptr;
		Element *valve1 = nullptr;
		Element *valve2 = nullptr;
		bool ok = false;
	};

	static void build(Chain &chain, bool linked = true)
	{
		chain.diagram = chain.project.addNewDiagram();
		Diagram *d = chain.diagram;
		chain.module = place(d, epp(), {0, 0});
		chain.cable = place(d, cable(), {300, 0});
		chain.tee = place(d, tee(), {600, 0});
		chain.valve1 = place(d, coil(), {900, -100});
		chain.valve2 = place(d, coil(), {900, 100});
		if (!(chain.module && chain.cable && chain.tee
		      && chain.valve1 && chain.valve2)) {
			return;
		}
		wire(d, chain.module, "X01", chain.cable, "X1");
		if (linked) {
			wire(d, chain.cable, "X2", chain.tee, "X1");
		}
		wire(d, chain.tee, "X2", chain.valve1, "X1");
		wire(d, chain.tee, "X3", chain.valve2, "X1");
		chain.ok = true;
	}

private slots:
	void initTestCase() { QETProject::setBackupEnabled(false); }
	void init() { setEnabled(true); }
	void cleanup() { setEnabled(false); }

	void theChannelsFollowTheTeeWithoutMixing()
	{
		Chain c;
		build(c);
		QVERIFY(c.ok);
			// Channels A and B of X01 are the two first channels
		editChannel(c.diagram, c.module, 0, "Valve A");
		QCOMPARE(functionOfValve(c.valve1), QStringLiteral("Valve A"));
		QCOMPARE(functionOfValve(c.valve2), QString());

		editChannel(c.diagram, c.module, 1, "Valve B");
		QCOMPARE(functionOfValve(c.valve1), QStringLiteral("Valve A"));
		QCOMPARE(functionOfValve(c.valve2), QStringLiteral("Valve B"));
	}

	void theSourceKeepsBeingFollowed()
	{
		Chain c;
		build(c);
		QVERIFY(c.ok);
		editChannel(c.diagram, c.module, 0, "Valve A");
		editChannel(c.diagram, c.module, 0, "Valve A2");
		QCOMPARE(functionOfValve(c.valve1), QStringLiteral("Valve A2"));
	}

	void aValveGivesItsFunctionToTheModule()
	{
		Chain c;
		build(c);
		QVERIFY(c.ok);
		editValve(c.diagram, c.valve2, "Clamp");
		QCOMPARE(functionOfChannel(c.module, 1), QStringLiteral("Clamp"));
		QVERIFY(functionOfChannel(c.module, 0) != "Clamp");
		QCOMPARE(functionOfValve(c.valve1), QString());
	}

	void aYDistributorServesTwoValves()
	{
		QETProject project;
		Diagram *d = project.addNewDiagram();
		Element *module = place(d, epp(), {0, 0});
		Element *y = place(d, wye(), {300, 0});
		Element *v1 = place(d, coil(), {600, -100});
		Element *v2 = place(d, coil(), {600, 100});
		QVERIFY(module && y && v1 && v2);
		wire(d, module, "X02", y, "X1");
		wire(d, y, "X2", v1, "X1");
		wire(d, y, "X3", v2, "X1");
			// The second socket: channels three and four
		editChannel(d, module, 2, "Press");
		editChannel(d, module, 3, "Release");
		QCOMPARE(functionOfValve(v1), QStringLiteral("Press"));
		QCOMPARE(functionOfValve(v2), QStringLiteral("Release"));
	}

	void aBrokenChainPropagatesNothing()
	{
		Chain c;
		build(c, false);
		QVERIFY(c.ok);
		editChannel(c.diagram, c.module, 0, "Valve A");
		QCOMPARE(functionOfValve(c.valve1), QString());
	}

	void aSocketWithoutRouteReachesNothing()
	{
			// A plain wire between the module and a valve crosses no
			// symbol that routes the channels
		QETProject project;
		Diagram *d = project.addNewDiagram();
		Element *module = place(d, epp(), {0, 0});
		Element *valve = place(d, coil(), {300, 0});
		QVERIFY(module && valve);
		wire(d, module, "X03", valve, "X1");
		editChannel(d, module, 4, "Direct");
		QCOMPARE(functionOfValve(valve), QString());
	}

	void aFollowerInSyncFollowsTheNewValue()
	{
		Chain c;
		build(c);
		QVERIFY(c.ok);
		editValve(c.diagram, c.valve1, "Own");
		QCOMPARE(functionOfChannel(c.module, 0), QStringLiteral("Own"));

		QSignalSpy spy(FunctionPropagation::notifier(),
			       &FunctionPropagation::conflictFound);
		editChannel(c.diagram, c.module, 0, "Module");
		QCOMPARE(functionOfValve(c.valve1), QStringLiteral("Module"));
		QCOMPARE(spy.count(), 0);
	}

	void aConflictAtTheOtherEndIsNotOverwritten()
	{
		Chain c;
		build(c);
		QVERIFY(c.ok);
			// The valve holds a function that the module does not
		setEnabled(false);
		editValve(c.diagram, c.valve1, "Valve");
		editChannel(c.diagram, c.module, 0, "Module");
		setEnabled(true);

		QSignalSpy spy(FunctionPropagation::notifier(),
			       &FunctionPropagation::conflictFound);
		editChannel(c.diagram, c.module, 0, "Module 2");
		QCOMPARE(spy.count(), 1);
		QCOMPARE(functionOfChannel(c.module, 0),
			 QStringLiteral("Module 2"));
		QCOMPARE(functionOfValve(c.valve1), QStringLiteral("Valve"));
	}

	void oneUndoRestoresEveryEnd()
	{
		Chain c;
		build(c);
		QVERIFY(c.ok);
		QUndoStack &stack = c.diagram->undoStack();
		const int before = stack.count();
		editChannel(c.diagram, c.module, 0, "Valve A");
		QCOMPARE(stack.count(), before + 1);
		QCOMPARE(functionOfValve(c.valve1), QStringLiteral("Valve A"));

		stack.undo();
		QCOMPARE(functionOfValve(c.valve1), QString());
		QVERIFY(functionOfChannel(c.module, 0) != "Valve A");

		stack.redo();
		QCOMPARE(functionOfChannel(c.module, 0),
			 QStringLiteral("Valve A"));
		QCOMPARE(functionOfValve(c.valve1), QStringLiteral("Valve A"));
	}

	void oneUndoRestoresTheModuleAfterAValveEdit()
	{
		Chain c;
		build(c);
		QVERIFY(c.ok);
		const QString original = functionOfChannel(c.module, 0);
		editValve(c.diagram, c.valve1, "Clamp");
		QCOMPARE(functionOfChannel(c.module, 0),
			 QStringLiteral("Clamp"));
		c.diagram->undoStack().undo();
		QCOMPARE(functionOfChannel(c.module, 0), original);
		QCOMPARE(functionOfValve(c.valve1), QString());
	}

	void nothingFollowsWhenTheSettingIsOff()
	{
		setEnabled(false);
		Chain c;
		build(c);
		QVERIFY(c.ok);
		editChannel(c.diagram, c.module, 0, "Valve A");
		QCOMPARE(functionOfValve(c.valve1), QString());
		editValve(c.diagram, c.valve2, "Clamp");
		QVERIFY(functionOfChannel(c.module, 1) != "Clamp");
	}
};

QET_APP_TEST_MAIN(tst_functionpropagation)
#include "tst_functionpropagation.moc"
