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
/*
	The Beckhoff EtherCAT symbols shipped in extra-elements: each one is
	well formed, carries its own uuid, has terminals with a name, a
	direction and a uuid of their own, and the Beckhoff ones come with
	their manufacturer and designation filled in.
*/
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QLineF>
#include <QMap>
#include <QPointF>
#include <QSet>
#include <QtTest>
#include <cmath>

class TstBeckhoffElements : public QObject
{
	Q_OBJECT

	private:
		static QDir categoryDir()
		{
			return QDir(QStringLiteral(QET_EXTRA_ELEMENTS_DIR)
						+ QStringLiteral("/15_beckhoff_ethercat"));
		}

		static QDomDocument load(const QString &path)
		{
			QDomDocument doc;
			QFile file(path);
			if (file.open(QIODevice::ReadOnly)) {
				doc.setContent(&file);
			}
			return doc;
		}

		static QString info(const QDomElement &definition,
							const QString &name)
		{
			const auto list = definition
				.firstChildElement(QStringLiteral("elementInformations"))
				.elementsByTagName(QStringLiteral("elementInformation"));
			for (int i = 0; i < list.size(); ++i) {
				const auto e = list.at(i).toElement();
				if (e.attribute(QStringLiteral("name")) == name) {
					return e.text();
				}
			}
			return QString();
		}

		static QStringList ioAddresses(const QDomElement &definition)
		{
			QStringList list;
			const auto ios = definition.elementsByTagName(
				QStringLiteral("plcIO"));
			for (int i = 0; i < ios.size(); ++i) {
				list << ios.at(i).toElement().attribute(
					QStringLiteral("address"));
			}
			return list;
		}

		static QStringList cardKeys()
		{
			return {QStringLiteral("el1008"), QStringLiteral("el2008"),
					QStringLiteral("el5152"), QStringLiteral("ek1914")};
		}

	private slots:
		void directoryHasEnglishName()
		{
			const auto doc = load(
				categoryDir().filePath(QStringLiteral("qet_directory")));
			QCOMPARE(doc.documentElement().tagName(),
					 QStringLiteral("qet-directory"));
			const auto names = doc.elementsByTagName(QStringLiteral("name"));
			bool english = false;
			for (int i = 0; i < names.size(); ++i) {
				english |= names.at(i).toElement()
					.attribute(QStringLiteral("lang")) == QLatin1String("en");
			}
			QVERIFY(english);
		}

		void elementsAreWellFormed()
		{
			const auto files = categoryDir().entryList(
				{QStringLiteral("*.elmt")}, QDir::Files);
			QVERIFY(files.size() >= 5);

			QSet<QString> element_uuids;
			for (const QString &file : files)
			{
				QVERIFY2(!file.contains(QLatin1Char(' ')), qPrintable(file));
				const auto doc = load(categoryDir().filePath(file));
				const auto def = doc.documentElement();
				QVERIFY2(def.tagName() == QLatin1String("definition"),
						 qPrintable(file));
				QCOMPARE(def.attribute(QStringLiteral("type")),
						 QStringLiteral("element"));

				const QString uuid = def.firstChildElement(
					QStringLiteral("uuid")).attribute(QStringLiteral("uuid"));
				QVERIFY2(!uuid.isEmpty(), qPrintable(file));
				QVERIFY2(!element_uuids.contains(uuid), qPrintable(file));
				element_uuids.insert(uuid);

				QVERIFY2(!info(def, QStringLiteral("description")).isEmpty(),
						 qPrintable(file));

				QSet<QString> names, term_uuids;
				const auto terminals = def.firstChildElement(
					QStringLiteral("description")).elementsByTagName(
					QStringLiteral("terminal"));
				for (int i = 0; i < terminals.size(); ++i)
				{
					const auto t = terminals.at(i).toElement();
					const QString name = t.attribute(QStringLiteral("name"));
					const QString term_uuid =
						t.attribute(QStringLiteral("uuid"));
					QVERIFY2(!name.isEmpty() && !names.contains(name),
							 qPrintable(file + QLatin1Char(' ') + name));
					QVERIFY2(!term_uuid.isEmpty()
							 && !term_uuids.contains(term_uuid),
							 qPrintable(file + QLatin1Char(' ') + name));
					QVERIFY2((QStringList{QStringLiteral("n"),
										  QStringLiteral("e"),
										  QStringLiteral("s"),
										  QStringLiteral("w")})
								 .contains(t.attribute(
									 QStringLiteral("orientation"))),
							 qPrintable(file + QLatin1Char(' ') + name));
					names.insert(name);
					term_uuids.insert(term_uuid);
				}
			}
		}

		void ioCardsAreMastersWithAnIoTable()
		{
			const QStringList types{
				QStringLiteral("entree_digitale"),
				QStringLiteral("sortie_digitale"),
				QStringLiteral("entree_analogique"),
				QStringLiteral("sortie_analogique"),
				QStringLiteral("entree_universelle"),
				QStringLiteral("sortie_universelle")};
			for (const QString &file : {
					 QStringLiteral("el1xxx_generic.elmt"),
					 QStringLiteral("el2xxx_generic.elmt"),
					 QStringLiteral("el3xxx_generic.elmt"),
					 QStringLiteral("el4xxx_generic.elmt"),
					 QStringLiteral("epp2338.elmt")})
			{
				const auto def = load(categoryDir().filePath(file))
					.documentElement();
				QCOMPARE(def.attribute(QStringLiteral("link_type")),
						 QStringLiteral("master"));
				const auto ios = def.elementsByTagName(
					QStringLiteral("plcIO"));
				QVERIFY2(ios.size() >= 4, qPrintable(file));
				for (int i = 0; i < ios.size(); ++i)
				{
					const auto io = ios.at(i).toElement();
					QVERIFY2(types.contains(io.attribute(
								 QStringLiteral("type"))),
							 qPrintable(file));
					QVERIFY2(!io.attribute(QStringLiteral("address"))
							 .isEmpty(), qPrintable(file));
				}
			}
		}

		void beckhoffProductsAreFilledIn()
		{
			for (const QString &file : {
					 QStringLiteral("el1xxx_generic.elmt"),
					 QStringLiteral("el2xxx_generic.elmt"),
					 QStringLiteral("epp2338.elmt"),
					 QStringLiteral("el3xxx_generic.elmt"),
					 QStringLiteral("el4xxx_generic.elmt"),
					 QStringLiteral("el9011.elmt"),
					 QStringLiteral("af1000_2axes_generic.elmt")})
			{
				const auto def = load(categoryDir().filePath(file))
					.documentElement();
				QCOMPARE(info(def, QStringLiteral("manufacturer")),
						 QStringLiteral("Beckhoff"));
				QVERIFY2(!info(def, QStringLiteral("designation")).isEmpty(),
						 qPrintable(file));
			}
		}

		void appearancesShareTheDataOfTheSymbol()
		{
			const QStringList info_names{
				QStringLiteral("manufacturer"),
				QStringLiteral("manufacturer_reference"),
				QStringLiteral("designation")};
			for (const QString &key : {
					 QStringLiteral("el1008"), QStringLiteral("el2008"),
					 QStringLiteral("el2652"), QStringLiteral("el5152"),
					 QStringLiteral("ek1914"), QStringLiteral("epp2338")})
			{
				const auto symbol = load(categoryDir().filePath(
					key + QStringLiteral(".elmt"))).documentElement();
				QVERIFY2(!symbol.isNull(), qPrintable(key));
				for (const QString &suffix : {QStringLiteral("_table"),
											  QStringLiteral("_realistic")})
				{
					const QString file = key + suffix + QStringLiteral(".elmt");
					if (!categoryDir().exists(file)) {
						QVERIFY2(suffix == QLatin1String("_realistic"),
								 qPrintable(file));
						continue;
					}
					const auto def = load(categoryDir().filePath(file))
						.documentElement();
					QCOMPARE(def.attribute(QStringLiteral("link_type")),
							 QStringLiteral("master"));
					for (const QString &n : info_names) {
						QCOMPARE(info(def, n), info(symbol, n));
					}
					QCOMPARE(ioAddresses(def), ioAddresses(symbol));
				}
			}
		}

		void symbolsHideTheirIoTableTheTableAppearanceShowsIt()
		{
			const auto files = categoryDir().entryList(
				{QStringLiteral("*.elmt")}, QDir::Files);
			for (const QString &file : files) {
				const auto def = load(categoryDir().filePath(file))
					.documentElement();
				if (def.attribute(QStringLiteral("link_type"))
					!= QLatin1String("master")) {
					continue;
				}
				const auto hidden = def.elementsByTagName(
					QStringLiteral("column"));
				const auto parts = def.firstChildElement(
					QStringLiteral("description")).elementsByTagName(
					QStringLiteral("plc_table"));
				if (file.endsWith(QLatin1String("_table.elmt"))) {
					QCOMPARE(parts.size(), 1);
					QCOMPARE(hidden.size(), 0);
					QCOMPARE(def.elementsByTagName(
								 QStringLiteral("terminal")).size(),
							 0);
				} else {
					QCOMPARE(parts.size(), 0);
					QCOMPARE(hidden.size(), 5);
					for (int i = 0; i < hidden.size(); ++i) {
						QCOMPARE(hidden.at(i).toElement().attribute(
									 QStringLiteral("visible")),
								 QStringLiteral("false"));
					}
				}
			}
		}

		void realisticTerminalsAreAtTheCentreOfTheCirclesOnTheGrid()
		{
			for (const QString &key : cardKeys())
			{
				const QString file = key + QStringLiteral("_realistic.elmt");
				const auto def = load(categoryDir().filePath(file))
					.documentElement();
				QVERIFY2(!def.isNull(), qPrintable(file));
				const auto parts = def.firstChildElement(
					QStringLiteral("description"));

				QList<QPointF> centres;
				const auto circles = parts.elementsByTagName(
					QStringLiteral("circle"));
				for (int i = 0; i < circles.size(); ++i) {
					const auto c = circles.at(i).toElement();
					const qreal d = c.attribute(
						QStringLiteral("diameter")).toDouble();
					if (d < 4) {
						continue; // decorative, like a LED
					}
					centres << QPointF(
						c.attribute(QStringLiteral("x")).toDouble() + d / 2,
						c.attribute(QStringLiteral("y")).toDouble() + d / 2);
				}

				QList<QPointF> points;
				const auto terminals = parts.elementsByTagName(
					QStringLiteral("terminal"));
				for (int i = 0; i < terminals.size(); ++i) {
					const auto t = terminals.at(i).toElement();
					points << QPointF(
						t.attribute(QStringLiteral("x")).toDouble(),
						t.attribute(QStringLiteral("y")).toDouble());
				}

					// every circle of a contact has its terminal at its centre
				QVERIFY2(centres.size() >= 8, qPrintable(file));
				for (const QPointF &c : centres) {
					qreal nearest = 1e9;
					for (const QPointF &p : points) {
						nearest = qMin(nearest,
									   QLineF(c, p).length());
					}
					QVERIFY2(nearest < 0.01, qPrintable(
						file + QStringLiteral(": ")
						+ QString::number(c.x()) + QLatin1Char(',')
						+ QString::number(c.y())));
				}

					// the grid used is written in the note of the element
				const QString note = def.firstChildElement(
					QStringLiteral("informations")).text();
				const int at = note.indexOf(QLatin1String("grid "));
				QVERIFY2(at >= 0, qPrintable(file));
				const int grid = note.mid(at + 5).split(
					QLatin1Char(' ')).first().toInt();
				QVERIFY2(grid == 10 || grid == 5, qPrintable(file));
				if (key != QLatin1String("ek1914")) {
					QCOMPARE(grid, 10); // 5 px only when 10 px is impossible
				}
				for (const QPointF &p : points) {
					QVERIFY2(qFuzzyIsNull(std::fmod(p.x(), grid))
							 && qFuzzyIsNull(std::fmod(p.y(), grid)),
							 qPrintable(file + QStringLiteral(": ")
										+ QString::number(p.x())
										+ QLatin1Char(',')
										+ QString::number(p.y())));
				}
			}
		}

		void driveHasBusAndPowerTerminals()
		{
			const auto def = load(categoryDir().filePath(
				QStringLiteral("af1000_2axes_generic.elmt"))).documentElement();
			QSet<QString> names;
			const auto terminals = def.elementsByTagName(
				QStringLiteral("terminal"));
			for (int i = 0; i < terminals.size(); ++i) {
				names.insert(terminals.at(i).toElement()
							 .attribute(QStringLiteral("name")));
			}
			for (const QString &n : {
					 QStringLiteral("EtherCAT IN"),
					 QStringLiteral("EtherCAT OUT"),
					 QStringLiteral("L1"), QStringLiteral("L2"),
					 QStringLiteral("L3"), QStringLiteral("PE"),
					 QStringLiteral("A1-U"), QStringLiteral("A2-W")}) {
				QVERIFY2(names.contains(n), qPrintable(n));
			}
		}

		void exampleProjectShowsEachCardThreeTimes()
		{
			const auto doc = load(QStringLiteral(QET_EXAMPLES_DIR)
				+ QStringLiteral("/beckhoff_islands_example.qet"));
			const auto project = doc.documentElement();
			QVERIFY(project.tagName() == QLatin1String("project"));

			QSet<QString> embedded;
			QSet<QString> terminalUuids;
			const auto defs = project.firstChildElement(
				QStringLiteral("collection")).elementsByTagName(
				QStringLiteral("element"));
			for (int i = 0; i < defs.size(); ++i) {
				const auto def = defs.at(i).toElement();
				embedded.insert(def.attribute(QStringLiteral("name")));
				const auto terms = def.elementsByTagName(
					QStringLiteral("terminal"));
				for (int j = 0; j < terms.size(); ++j) {
					terminalUuids.insert(terms.at(j).toElement()
						.attribute(QStringLiteral("uuid")));
				}
			}

				//label -> "<file>@<folio>" of each appearance
			QMap<QString, QStringList> appearances;
			QSet<QString> used;
			int folio = 0;
			const auto folios = project.elementsByTagName(
				QStringLiteral("diagram"));
			QCOMPARE(folios.size(), 3);
			for (int i = 0; i < folios.size(); ++i, ++folio) {
				const auto els = folios.at(i).toElement()
					.firstChildElement(QStringLiteral("elements"))
					.elementsByTagName(QStringLiteral("element"));
				for (int j = 0; j < els.size(); ++j) {
					const auto e = els.at(j).toElement();
					const QString file = e.attribute(QStringLiteral("type"))
						.section(QLatin1Char('/'), -1);
					QVERIFY2(embedded.contains(file), qPrintable(file));
					used.insert(e.attribute(QStringLiteral("uuid")));
					const auto labels = e.firstChildElement(
						QStringLiteral("elementInformations"))
						.elementsByTagName(
							QStringLiteral("elementInformation"));
					for (int k = 0; k < labels.size(); ++k) {
						const auto l = labels.at(k).toElement();
						if (l.attribute(QStringLiteral("name"))
							== QLatin1String("label")) {
							appearances[l.text()] << file
								+ QLatin1Char('@') + QString::number(folio);
						}
					}
				}
			}

			QCOMPARE(appearances.value(QStringLiteral("A1.00")).size(), 1);
			for (const QString &card : {QStringLiteral("A1.10"),
										QStringLiteral("A1.20"),
										QStringLiteral("A1.30")}) {
				const auto list = appearances.value(card);
				QCOMPARE(list.size(), 3);
				QSet<int> onFolio;
				for (const QString &a : list) {
					onFolio.insert(a.section(QLatin1Char('@'), 1).toInt());
				}
				QCOMPARE(onFolio.size(), 3);
			}

				//every wire joins two terminals of elements of the project
			const auto wires = project.elementsByTagName(
				QStringLiteral("conductor"));
			QVERIFY(wires.size() > 0);
			for (int i = 0; i < wires.size(); ++i) {
				const auto c = wires.at(i).toElement();
				for (const QString &end : {QStringLiteral("1"),
										   QStringLiteral("2")}) {
					QVERIFY(used.contains(c.attribute(
						QStringLiteral("element") + end)));
					QVERIFY(terminalUuids.contains(c.attribute(
						QStringLiteral("terminal") + end)));
				}
			}
		}
};

QTEST_GUILESS_MAIN(TstBeckhoffElements)
#include "tst_beckhoffelements.moc"
