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
#include "undocommand/changewireprofilescommand.h"

#include <QUndoStack>

/**
	The standard wires of a project: saved and opened again, and a change
	of a profile reaches every conductor that names it, in one undo step.
*/
class tst_wireprofiles : public QObject
{
	Q_OBJECT

	Diagram *m_diagram = nullptr;

	static WireProfiles profiles(const QList<WireProfile> &list)
	{
		WireProfiles result;
		result.setProfiles(list);
		return result;
	}

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

	Conductor *wire(const QString &profile, qreal y)
	{
		Element *a = place(QPointF(100, y));
		Element *b = place(QPointF(300, y));
		if (!a || !b)
			return nullptr;
		auto *conductor = new Conductor(a->terminals().first(), b->terminals().first());
		m_diagram->addItem(conductor);
		ConductorProperties properties = conductor->properties();
		properties.m_wire_profile = profile;
		conductor->setProperties(properties);
		return conductor;
	}

	private slots:
		void initTestCase()
		{
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

		void listIsSavedAndOpenedAgain()
		{
			const WireProfiles list = profiles({
				{QStringLiteral("Control"), QStringLiteral("BK"), QStringLiteral("1.5")},
				{QStringLiteral("Earth"), QStringLiteral("GNYE"), QStringLiteral("2.5")}});
			QDomDocument doc;
			QDomElement e = list.toXml(doc);
			QCOMPARE(WireProfiles::fromXml(e), list);

				//Nameless and repeated names are left out, no element is
				//no profile
			QDomElement bad = doc.createElement(QStringLiteral("wire_profiles"));
			QDomElement nameless = doc.createElement(QStringLiteral("wire_profile"));
			bad.appendChild(nameless);
			bad.appendChild(e.firstChildElement().cloneNode().toElement());
			bad.appendChild(e.firstChildElement().cloneNode().toElement());
			QCOMPARE(WireProfiles::fromXml(bad).profiles().size(), 1);
			QVERIFY(WireProfiles::fromXml(QDomElement()).isEmpty());
		}

		void projectKeepsTheListAndSavesNothingWithout()
		{
			QETProject *project = m_diagram->project();
			QVERIFY(project->toXml().documentElement()
					.firstChildElement(QStringLiteral("wire_profiles")).isNull());

			project->setWireProfiles(profiles({
				{QStringLiteral("Control"), QStringLiteral("BK"), QStringLiteral("1.5")}}));
			QVERIFY(!project->toXml().documentElement()
					.firstChildElement(QStringLiteral("wire_profiles")).isNull());
			project->setWireProfiles(WireProfiles());
		}

		void conductorProfileNameIsOnlyWrittenWhenSet()
		{
			ConductorProperties properties;
			QDomDocument doc;
			QDomElement e = doc.createElement(QStringLiteral("conductor"));
			properties.toXml(e);
			QVERIFY(!e.hasAttribute(QStringLiteral("wire_profile")));

			properties.m_wire_profile = QStringLiteral("Control");
			QDomElement f = doc.createElement(QStringLiteral("conductor"));
			properties.toXml(f);
			QCOMPARE(f.attribute(QStringLiteral("wire_profile")), QStringLiteral("Control"));
			ConductorProperties read;
			read.fromXml(f);
			QCOMPARE(read.m_wire_profile, QStringLiteral("Control"));
				//A file saved before has none
			ConductorProperties old;
			old.fromXml(e);
			QVERIFY(old.m_wire_profile.isEmpty());
		}

		void changingAProfileChangesItsConductors()
		{
			QETProject *project = m_diagram->project();
			project->setWireProfiles(profiles({
				{QStringLiteral("Control"), QStringLiteral("BK"), QStringLiteral("1.5")},
				{QStringLiteral("Power"), QStringLiteral("RD"), QStringLiteral("4")}}));
			Conductor *control = wire(QStringLiteral("Control"), 100);
			Conductor *power = wire(QStringLiteral("Power"), 200);
			Conductor *none = wire(QString(), 300);
			QVERIFY(control && power && none);

			QUndoStack *stack = project->undoStack();
			const int before = stack->count();

				//Control becomes "Command" in blue 0.75, Power goes away
			QHash<QString, QString> renamed;
			renamed.insert(QStringLiteral("Control"), QStringLiteral("Command"));
			stack->push(new ChangeWireProfilesCommand(project,
				profiles({{QStringLiteral("Command"), QStringLiteral("BU"), QStringLiteral("0.75")}}),
				renamed));
			QCOMPARE(stack->count(), before + 1);

			QCOMPARE(control->properties().m_wire_profile, QStringLiteral("Command"));
			QCOMPARE(control->properties().m_wire_color, QStringLiteral("BU"));
			QCOMPARE(control->properties().m_wire_section, QStringLiteral("0.75"));
			QCOMPARE(control->properties().color, QColor(0x00, 0x00, 0xFF));
				//Removed: the values stay, the name goes
			QVERIFY(power->properties().m_wire_profile.isEmpty());
			QCOMPARE(power->properties().m_wire_color, QString());
			QVERIFY(none->properties().m_wire_profile.isEmpty());

			stack->undo();
			QCOMPARE(project->wireProfiles().profiles().size(), 2);
			QCOMPARE(control->properties().m_wire_profile, QStringLiteral("Control"));
			QCOMPARE(control->properties().m_wire_color, QString());
			QCOMPARE(power->properties().m_wire_profile, QStringLiteral("Power"));

			stack->redo();
			QCOMPARE(control->properties().m_wire_section, QStringLiteral("0.75"));
			project->setWireProfiles(WireProfiles());
		}

		void aCableCoreKeepsItsValues()
		{
			ConductorProperties properties;
			properties.m_wire_profile = QStringLiteral("Control");
			properties.m_wire_color = QStringLiteral("BN");
			properties.m_wire_section = QStringLiteral("2.5");
			properties.m_cable_uuid = QUuid::createUuid();
			const ConductorProperties result = ChangeWireProfilesCommand::applied(
				properties,
				profiles({{QStringLiteral("Control"), QStringLiteral("BU"), QStringLiteral("1")}}),
				{});
			QCOMPARE(result.m_wire_profile, QStringLiteral("Control"));
			QCOMPARE(result.m_wire_color, QStringLiteral("BN"));
			QCOMPARE(result.m_wire_section, QStringLiteral("2.5"));
		}
};

QET_APP_TEST_MAIN(tst_wireprofiles)
#include "tst_wireprofiles.moc"
