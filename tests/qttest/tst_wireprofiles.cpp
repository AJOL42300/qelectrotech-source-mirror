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
#include "qetapptest.h"
#include "conductorproperties.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "properties/wireprofiles.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "qetresult.h"

#include <QFile>
#include <QTemporaryDir>

/**
	The wire colour meaning table of a project: its default lines, applying
	a line to several conductors in one undo step, and keeping the table in
	the project file.
*/
class tst_wireprofiles : public QObject
{
	Q_OBJECT

	Diagram *m_diagram = nullptr;
	QTemporaryDir m_dir;

	Element *place(const QPointF &pos)
	{
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/end.elmt"), m_diagram->project()),
			nullptr, &state);
		if (state || !element)
			return nullptr;
		m_diagram->addItem(element);
		element->setPos(pos);
		return element;
	}

		//A wire at height @a y
	Conductor *wire(qreal y)
	{
		Element *a = place(QPointF(100, y));
		Element *b = place(QPointF(300, y));
		if (!a || !b)
			return nullptr;
		auto *conductor = new Conductor(a->terminals().first(), b->terminals().first());
		m_diagram->addItem(conductor);
		return conductor;
	}

	private slots:
		void initTestCase()
		{
			QVERIFY(m_dir.isValid());
			m_diagram = QetAppTest::diagram();
			QVERIFY(m_diagram);
			QDomDocument doc;
			doc.setContent(QStringLiteral(
				R"(<definition type="element" link_type="simple" width="10" height="10" hotspot_x="0" hotspot_y="5">
<names><name lang="en">end</name></names><description>
<terminal x="0" y="0" orientation="e" name="1"/></description></definition>)"));
			QVERIFY(m_diagram->project()->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("end"), doc.documentElement()));
		}

		void theDefaultTable()
		{
			const WireProfiles table;
			QVERIFY(table.isDefault());
			QVERIFY(!table.profiles().isEmpty());

			QSet<QString> names;
			for (const WireProfile &profile : table.profiles()) {
				QVERIFY(!profile.name.isEmpty());
				QVERIFY(profile.color.isValid());
				QVERIFY2(!names.contains(profile.name), qPrintable(profile.name));
				names << profile.name;
			}

			WireProfile found;
			QVERIFY(table.find(QStringLiteral("+24 V DC"), &found));
			QCOMPARE(found.color, QColor(Qt::red));
			QCOMPARE(found.wire_color, QStringLiteral("RD"));
			QCOMPARE(found.section, QStringLiteral("0.75"));
			QCOMPARE(found.label, QStringLiteral("+24V"));
			QVERIFY(!found.isTwoColour());

			QVERIFY(table.find(QStringLiteral("Protective earth (PE)"), &found));
			QCOMPARE(found.wire_color, QStringLiteral("GNYE"));
			QVERIFY(found.isTwoColour());
			QVERIFY(!table.find(QStringLiteral("no such function")));
		}

		void aProfileIsAppliedToSeveralWiresInOneUndoStep()
		{
			Conductor *a = wire(100);
			Conductor *b = wire(200);
			QVERIFY(a && b);
			m_diagram->clearSelection();
			a->setSelected(true);
			b->setSelected(true);

			WireProfile pe;
			QVERIFY(WireProfiles().find(QStringLiteral("Protective earth (PE)"), &pe));
			WireProfile v24;
			QVERIFY(WireProfiles().find(QStringLiteral("+24 V DC"), &v24));

			const int steps = m_diagram->undoStack().count();
			QVERIFY(WireProfiles::applyToSelection(m_diagram, v24));
			QCOMPARE(m_diagram->undoStack().count(), steps + 1);
			for (Conductor *c : {a, b}) {
				const ConductorProperties p = c->properties();
				QCOMPARE(p.color, QColor(Qt::red));
				QCOMPARE(p.m_wire_color, QStringLiteral("RD"));
				QCOMPARE(p.m_wire_section, QStringLiteral("0.75"));
				QCOMPARE(p.m_function, QStringLiteral("+24 V DC"));
				QCOMPARE(p.text, QStringLiteral("+24V"));
				QVERIFY(p.m_show_text);
				QVERIFY(!p.m_bicolor);
			}

				//The same profile again changes nothing: no empty step
			QVERIFY(!WireProfiles::applyToSelection(m_diagram, v24));
			QCOMPARE(m_diagram->undoStack().count(), steps + 1);

			QVERIFY(WireProfiles::applyToSelection(m_diagram, pe));
			QVERIFY(a->properties().m_bicolor);
			QVERIFY(a->properties().m_dash_size >= 2);
			QCOMPARE(a->properties().m_color_2, pe.color_2);

			m_diagram->undoStack().undo();
			QCOMPARE(a->properties().m_wire_color, QStringLiteral("RD"));
			QVERIFY(!a->properties().m_bicolor);
			m_diagram->undoStack().undo();
			QCOMPARE(m_diagram->undoStack().count(), steps + 2);
			for (Conductor *c : {a, b}) {
				const ConductorProperties p = c->properties();
				QVERIFY(p.m_wire_color.isEmpty());
				QVERIFY(p.m_wire_section.isEmpty());
				QVERIFY(p.m_function.isEmpty());
				QVERIFY(p.text != QStringLiteral("+24V"));
			}
		}

		void nothingSelectedChangesNothing()
		{
			m_diagram->clearSelection();
			const int steps = m_diagram->undoStack().count();
			QVERIFY(!WireProfiles::applyToSelection(m_diagram, WireProfiles().profiles().first()));
			QCOMPARE(m_diagram->undoStack().count(), steps);
		}

		void aCableCoreKeepsItsColourCodeAndSection()
		{
			Conductor *c = wire(300);
			QVERIFY(c);
			ConductorProperties p = c->properties();
			p.m_wire_color = QStringLiteral("BN");
			p.m_wire_section = QStringLiteral("2.5");
			p.m_cable_uuid = QUuid::createUuid();
			c->setProperties(p);
			m_diagram->clearSelection();
			c->setSelected(true);

			WireProfile v24;
			QVERIFY(WireProfiles().find(QStringLiteral("+24 V DC"), &v24));
			QVERIFY(WireProfiles::applyToSelection(m_diagram, v24));
			QCOMPARE(c->properties().color, QColor(Qt::red));
			QCOMPARE(c->properties().m_wire_color, QStringLiteral("BN"));
			QCOMPARE(c->properties().m_wire_section, QStringLiteral("2.5"));
		}

		void theTableSurvivesXml()
		{
			WireProfiles table;
			QList<WireProfile> profiles = table.profiles();
			profiles.removeLast();
			profiles.first().section = QStringLiteral("1.5");
			WireProfile mine;
			mine.name = QStringLiteral("Brake <supply> & co");
			mine.wire_color = QStringLiteral("WHBN");
			mine.color = QColor(0x12, 0x34, 0x56);
			mine.color_2 = QColor(0xab, 0xcd, 0xef);
			mine.label = QStringLiteral("BRK");
			profiles << mine;
			table.setProfiles(profiles);
			QVERIFY(!table.isDefault());

			QDomDocument doc;
			doc.appendChild(table.toXml(doc));
			QCOMPARE(WireProfiles::fromXml(doc.documentElement()), table);

				//A project saved before the table existed has the default one
			QCOMPARE(WireProfiles::fromXml(QDomElement()), WireProfiles());
		}

		void theTableIsKeptWhenTheProjectIsSavedAndOpenedAgain()
		{
			const QString path = m_dir.filePath(QStringLiteral("profiles.qet"));
			QVERIFY(QFile::copy(QFINDTESTDATA("fixtures/every_feature.qet"), path));
			WireProfiles table;
			{
				QETProject project(path);
				QVERIFY(!project.isReadOnly());
					//Nothing is written while the table is the default one
				QVERIFY(project.wireProfiles().isDefault());
				QVERIFY(project.toXml().documentElement()
						.firstChildElement(QStringLiteral("wireprofiles")).isNull());

				QList<WireProfile> profiles = table.profiles();
				profiles.first().name = QStringLiteral("Supply A");
				profiles.first().section = QStringLiteral("1.5");
				table.setProfiles(profiles);
				project.setWireProfiles(table);
				QVERIFY(project.projectWasModified());
				QVERIFY(project.write().isOk());
			}
			QETProject reopened(path);
			QCOMPARE(reopened.wireProfiles(), table);
		}

		void theLegendListsEveryLine()
		{
			const WireProfiles table;
			const QString html = table.legendHtml();
			for (const WireProfile &profile : table.profiles())
				QVERIFY2(html.contains(profile.name.toHtmlEscaped()), qPrintable(profile.name));
			QVERIFY(html.contains(QStringLiteral("#ff0000")));

			const int items = m_diagram->items().count();
			const int steps = m_diagram->undoStack().count();
			WireProfiles::addLegend(m_diagram, QPointF(50, 400));
			QCOMPARE(m_diagram->items().count(), items + 1);
			QCOMPARE(m_diagram->undoStack().count(), steps + 1);
			m_diagram->undoStack().undo();
			QCOMPARE(m_diagram->items().count(), items);
		}
};

QET_APP_TEST_MAIN(tst_wireprofiles)
#include "tst_wireprofiles.moc"
