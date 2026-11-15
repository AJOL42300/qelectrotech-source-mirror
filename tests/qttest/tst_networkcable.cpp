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
#include "properties/cablecatalog.h"
#include "properties/networkcable.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"

#include <QImage>
#include <QPainter>
#include <QSet>
#include <QTemporaryDir>

/**
	Network and moulded cables: a conductor which has a cable type, a
	length and a branch mark. The type sets the sheath colour, the
	connectors are drawn at the ends, the project file only holds
	something new when a conductor is a cable, and a change is undone in
	one step.
*/
class tst_networkcable : public QObject
{
	Q_OBJECT

	Diagram *m_diagram = nullptr;

	Conductor *wire()
	{
		Element *elements[2] = {nullptr, nullptr};
		for (int i = 0; i < 2; ++i)
		{
			int state = 0;
			elements[i] = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/end.elmt"),
						 m_diagram->project()),
				nullptr, &state);
			if (state || !elements[i])
				return nullptr;
			m_diagram->addItem(elements[i]);
			elements[i]->setPos(100 + 200 * i, 100);
		}
		auto *conductor = new Conductor(elements[0]->terminals().first(),
						elements[1]->terminals().first());
		m_diagram->addItem(conductor);
		return conductor;
	}

	static int pixelsOf(Conductor *conductor, const QColor &colour)
	{
		const QRectF area =
			conductor->sceneBoundingRect().adjusted(-2, -2, 2, 2);
		QImage image(area.size().toSize() * 2, QImage::Format_RGB32);
		image.fill(Qt::white);
		QPainter painter(&image);
		conductor->scene()->render(&painter, QRectF(), area);
		painter.end();
		int count = 0;
		for (int y = 0; y < image.height(); ++y)
			for (int x = 0; x < image.width(); ++x) {
				const QColor c = image.pixelColor(x, y);
				if (qAbs(c.red() - colour.red()) < 40
				    && qAbs(c.green() - colour.green()) < 40
				    && qAbs(c.blue() - colour.blue()) < 40)
					++count;
			}
		return count;
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

		void aTypeSetsTheSheath()
		{
			ConductorProperties p;
			NetworkCables::apply(p, QStringLiteral("ethercat_p"));
			QCOMPARE(p.m_network_cable, QStringLiteral("ethercat_p"));
			QCOMPARE(p.color, QColor(220, 0, 0));
			QVERIFY(p.m_bicolor);
			QCOMPARE(p.m_color_2, QColor(0, 0, 0));
			QCOMPARE(p.m_conductor_type, QStringLiteral("EtherCAT P"));

			NetworkCables::apply(p, QStringLiteral("ethercat"));
			QVERIFY(!p.m_bicolor);

				//A motor cable keeps the designation typed on it
			p.m_conductor_type = QStringLiteral("4 G 1,5 mm²");
			NetworkCables::apply(p, QStringLiteral("motor"));
			QCOMPARE(p.m_conductor_type, QStringLiteral("4 G 1,5 mm²"));

			p.m_network_length = QStringLiteral("20");
			NetworkCables::apply(p, QString());
			QVERIFY(p.m_network_cable.isEmpty());
			QVERIFY(p.m_network_length.isEmpty());
		}

		void lengthAndLabel()
		{
			QCOMPARE(NetworkCables::lengthInMetres(QStringLiteral("1,5")), 1.5);
			QCOMPARE(NetworkCables::lengthInMetres(QStringLiteral(" 20 m ")), 20.0);
			QCOMPARE(NetworkCables::lengthInMetres(QStringLiteral("long")), 0.0);

			ConductorProperties p;
			QVERIFY(NetworkCables::label(p).isEmpty());
			NetworkCables::apply(p, QStringLiteral("y_m12"));
			p.m_network_branch = QStringLiteral("2");
			p.m_network_length = QStringLiteral("3");
			QCOMPARE(NetworkCables::label(p),
				 QStringLiteral("(2) M12 Y cable 3 m"));
		}

		void totals()
		{
			QList<ConductorProperties> list;
			ConductorProperties ordinary;
			list << ordinary;
			ConductorProperties a;
			NetworkCables::apply(a, QStringLiteral("ethercat"));
			a.m_network_length = QStringLiteral("2,5");
			list << a << a;
			ConductorProperties y1;
			NetworkCables::apply(y1, QStringLiteral("y_m12"));
			y1.m_network_branch = QStringLiteral("1");
			ConductorProperties y2 = y1;
			y2.m_network_branch = QStringLiteral("2");
			list << y1 << y2;
			ConductorProperties motor;
			NetworkCables::apply(motor, QStringLiteral("motor"));
			motor.m_network_length = QStringLiteral("20");
			list << motor;

			const QList<NetworkCables::Total> totals = NetworkCables::totals(list);
			QCOMPARE(totals.size(), 3);
			QCOMPARE(totals[0].id, QStringLiteral("ethercat"));
			QCOMPARE(totals[0].count, 2);
			QCOMPARE(totals[0].metres, 5.0);
			QCOMPARE(totals[0].connectors, 4);
				//One connector on the box, one on each valve
			QCOMPARE(totals[1].id, QStringLiteral("y_m12"));
			QCOMPARE(totals[1].connectors, 3);
			QCOMPARE(totals[2].id, QStringLiteral("motor"));
			QCOMPARE(totals[2].metres, 20.0);
			QCOMPARE(totals[2].connectors, 0);
		}

		void oldProjectsAreSavedAsBefore()
		{
			ConductorProperties plain;
			QDomDocument doc;
			QDomElement e = doc.createElement(QStringLiteral("conductor"));
			plain.toXml(e);
			QVERIFY(!e.hasAttribute(QStringLiteral("network_cable")));
			QVERIFY(!e.hasAttribute(QStringLiteral("network_length")));
			QVERIFY(!e.hasAttribute(QStringLiteral("network_branch")));
			ConductorProperties read;
			read.fromXml(e);
			QVERIFY(read.m_network_cable.isEmpty());
			QDomElement again = doc.createElement(QStringLiteral("conductor"));
			read.toXml(again);
			QCOMPARE(again.attributes().count(), e.attributes().count());
		}

		void savedAndOpenedAgain()
		{
			ConductorProperties p;
			NetworkCables::apply(p, QStringLiteral("y_m12"));
			p.m_network_length = QStringLiteral("1,5");
			p.m_network_branch = QStringLiteral("1");
			QDomDocument doc;
			QDomElement e = doc.createElement(QStringLiteral("conductor"));
			p.toXml(e);
			ConductorProperties read;
			read.fromXml(e);
			QCOMPARE(read.m_network_cable, QStringLiteral("y_m12"));
			QCOMPARE(read.m_network_length, QStringLiteral("1,5"));
			QCOMPARE(read.m_network_branch, QStringLiteral("1"));
			QDomElement again = doc.createElement(QStringLiteral("conductor"));
			read.toXml(again);
			QCOMPARE(again.attributes().count(), e.attributes().count());
		}

		void aCableIsDrawnAndUndone()
		{
			Conductor *conductor = wire();
			QVERIFY(conductor);
			const QColor green(0, 150, 60);
			QCOMPARE(pixelsOf(conductor, green), 0);
			const QRectF plain_rect = conductor->boundingRect();

			ConductorProperties before = conductor->properties();
			ConductorProperties after = before;
			NetworkCables::apply(after, QStringLiteral("ethercat"));
			QVariant old_value, new_value;
			old_value.setValue(before);
			new_value.setValue(after);
			m_diagram->undoStack().push(new QPropertyUndoCommand(
				conductor, "properties", old_value, new_value));
			QVERIFY(pixelsOf(conductor, green) > 20);
			QVERIFY(conductor->boundingRect().width() > plain_rect.width());

			m_diagram->undoStack().undo();
			QVERIFY(conductor->properties().m_network_cable.isEmpty());
			QCOMPARE(pixelsOf(conductor, green), 0);
			QCOMPARE(conductor->boundingRect(), plain_rect);
		}

		void theBuiltInCataloguePointsAtOneCablePerReference()
		{
			const QList<CableCatalog::Entry> list =
				CableCatalog::builtin();
			QVERIFY(!list.isEmpty());
			QSet<QString> keys;
			QSet<QString> makers;
			for (const CableCatalog::Entry &e : list) {
				QVERIFY(!e.reference.isEmpty());
				QVERIFY(!e.manufacturer.isEmpty());
				QVERIFY(NetworkCables::find(
					CableCatalog::networkTypeId(e)));
				QVERIFY(!keys.contains(e.manufacturer + e.reference));
				keys << e.manufacturer + e.reference;
				makers << e.manufacturer;
			}
			QCOMPARE(makers.size(), 3);
		}

		void aCatalogueCableFillsTheProperties()
		{
			CableCatalog::Entry beckhoff;
			for (const CableCatalog::Entry &e : CableCatalog::builtin()) {
				if (e.reference == QLatin1String("ZK7000-0102-0020"))
					beckhoff = e;
			}
			QCOMPARE(beckhoff.usage, QStringLiteral("ethercat_p"));

			ConductorProperties p;
			CableCatalog::apply(p, beckhoff);
			QCOMPARE(p.m_network_cable, QStringLiteral("ethercat_p"));
			QCOMPARE(p.m_network_reference,
				 QStringLiteral("ZK7000-0102-0020"));
			QCOMPARE(p.m_network_length, QStringLiteral("2"));
			QCOMPARE(p.m_wire_section, QStringLiteral("0,34"));

				//The type chosen first is kept
			ConductorProperties q;
			NetworkCables::apply(q, QStringLiteral("motor"));
			CableCatalog::apply(q, beckhoff);
			QCOMPARE(q.m_network_cable, QStringLiteral("motor"));

			NetworkCables::apply(q, QString());
			QVERIFY(q.m_network_reference.isEmpty());

			QDomDocument doc;
			QDomElement e = doc.createElement(QStringLiteral("conductor"));
			p.toXml(e);
			ConductorProperties read;
			read.fromXml(e);
			QCOMPARE(read.m_network_reference, p.m_network_reference);
			ConductorProperties plain;
			QDomElement f = doc.createElement(QStringLiteral("conductor"));
			plain.toXml(f);
			QVERIFY(!f.hasAttribute(QStringLiteral("network_reference")));
		}

		void totalsPerReference()
		{
			ConductorProperties a;
			NetworkCables::apply(a, QStringLiteral("ethernet"));
			a.m_network_reference = QStringLiteral("REF-A");
			a.m_network_length = QStringLiteral("2,5");
			ConductorProperties plain;
			NetworkCables::apply(plain, QStringLiteral("ethernet"));
			const QList<NetworkCables::ReferenceTotal> totals =
				NetworkCables::referenceTotals(
					{a, a, plain, ConductorProperties()});
			QCOMPARE(totals.size(), 1);
			QCOMPARE(totals[0].reference, QStringLiteral("REF-A"));
			QCOMPARE(totals[0].count, 2);
			QCOMPARE(totals[0].metres, 5.0);
		}

		void theCatalogueIsKeptInJson()
		{
			QTemporaryDir dir;
			QVERIFY(dir.isValid());
			const QString path = dir.filePath(QStringLiteral("c.json"));
			QVERIFY(CableCatalog::readJson(path).isEmpty());
			QVERIFY(CableCatalog::writeJson(path, CableCatalog::builtin()));
			const QList<CableCatalog::Entry> back =
				CableCatalog::readJson(path);
			const QList<CableCatalog::Entry> list =
				CableCatalog::builtin();
			QCOMPARE(back.size(), list.size());
			QCOMPARE(back.first().reference, list.first().reference);
			QCOMPARE(back.last().length, list.last().length);

			CableCatalog::Entry changed = list.first();
			changed.designation = QStringLiteral("mine");
			const QList<CableCatalog::Entry> merged =
				CableCatalog::merged(list, {changed});
			QCOMPARE(merged.size(), list.size());
			QCOMPARE(merged.first().designation, QStringLiteral("mine"));
		}

		void aCsvExportIsImported()
		{
			QTemporaryDir dir;
			QVERIFY(dir.isValid());
			QFile file(dir.filePath(QStringLiteral("lapp.csv")));
			QVERIFY(file.open(QIODevice::WriteOnly));
			file.write("Artikelnummer;Bezeichnung;Adernzahl;"
				   "Querschnitt;Au\xC3\x9F" "endurchmesser;"
				   "Hersteller\r\n"
				   "\"1119304\";\"OELFLEX CLASSIC 110 4G1,5\";4;"
				   "1,5 mm\xC2\xB2;7,2;LAPP\r\n"
				   "0027969;\"OELFLEX SERVO; \"\"FD\"\" 796\";"
				   "0;1,5;;LAPP\r\n"
				   ";no reference;1;1;1;LAPP\r\n");
			file.close();
			QString error;
			const QList<CableCatalog::Entry> list =
				CableCatalog::readCsv(file.fileName(), &error);
			QVERIFY2(error.isEmpty(), qPrintable(error));
			QCOMPARE(list.size(), 2);
			QCOMPARE(list[0].reference, QStringLiteral("1119304"));
			QCOMPARE(list[0].conductors, 4);
			QCOMPARE(list[0].section, QStringLiteral("1,5"));
			QCOMPARE(list[0].diameter, QStringLiteral("7,2"));
			QCOMPARE(list[0].manufacturer, QStringLiteral("LAPP"));
			QCOMPARE(list[1].designation,
				 QStringLiteral("OELFLEX SERVO; \"FD\" 796"));
			QCOMPARE(list[1].usage, QStringLiteral("servo"));

			QFile bad(dir.filePath(QStringLiteral("bad.csv")));
			QVERIFY(bad.open(QIODevice::WriteOnly));
			bad.write("colour;size\nred;1\n");
			bad.close();
			QVERIFY(CableCatalog::readCsv(bad.fileName(), &error).isEmpty());
			QVERIFY(!error.isEmpty());
		}
};

QET_APP_TEST_MAIN(tst_networkcable)

#include "tst_networkcable.moc"
