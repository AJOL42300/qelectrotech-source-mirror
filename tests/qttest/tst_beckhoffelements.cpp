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
					 QStringLiteral("drives_af1000/af1215.elmt"),
					 QStringLiteral("drives_ax5000/ax5203.elmt"),
					 QStringLiteral("twinsafe/el1918.elmt")})
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
						// only the Terminal column (5) is switched on
					QCOMPARE(hidden.size(), 1);
					QCOMPARE(hidden.at(0).toElement().attribute(
								 QStringLiteral("index")),
							 QStringLiteral("5"));
					QCOMPARE(hidden.at(0).toElement().attribute(
								 QStringLiteral("visible")),
							 QStringLiteral("true"));
					QCOMPARE(terminalNames(def).size(), 0);
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

		static QStringList terminalNames(const QDomElement &def)
		{
			QStringList names;
			const auto terminals = def.firstChildElement(
				QStringLiteral("description")).elementsByTagName(
				QStringLiteral("terminal"));
			for (int i = 0; i < terminals.size(); ++i) {
				names << terminals.at(i).toElement()
					.attribute(QStringLiteral("name"));
			}
			return names;
		}

			//address -> pins listed in the plcIO of an element
		static QMap<QString, QStringList> ioPins(const QDomElement &def)
		{
			QMap<QString, QStringList> map;
			const auto ios = def.elementsByTagName(QStringLiteral("plcIO"));
			for (int i = 0; i < ios.size(); ++i) {
				const auto io = ios.at(i).toElement();
				QStringList pins;
				const auto list = io.elementsByTagName(
					QStringLiteral("terminal"));
				for (int j = 0; j < list.size(); ++j) {
					pins << list.at(j).toElement().text();
				}
				map.insert(io.attribute(QStringLiteral("address")), pins);
			}
			return map;
		}

		void pinsFollowTheBeckhoffOrderNotTheChannel()
		{
			const auto el1008 = load(categoryDir().filePath(
				QStringLiteral("el1008.elmt"))).documentElement();
			const auto in = ioPins(el1008);
			QCOMPARE(in.value(QStringLiteral("I1")), QStringList{"1"});
			QCOMPARE(in.value(QStringLiteral("I2")), QStringList{"5"});
			QCOMPARE(in.value(QStringLiteral("I3")), QStringList{"2"});
			QCOMPARE(in.value(QStringLiteral("I8")), QStringList{"8"});

			const auto el2008 = load(categoryDir().filePath(
				QStringLiteral("el2008.elmt"))).documentElement();
			QCOMPARE(ioPins(el2008).value(QStringLiteral("O2")),
					 QStringList{"5"});

			const auto el5152 = load(categoryDir().filePath(
				QStringLiteral("el5152.elmt"))).documentElement();
			QCOMPARE(ioPins(el5152).value(QStringLiteral("A2")),
					 QStringList{"4"});

				// every listed pin is a terminal of the symbol and of its
				// realistic view
			for (const QString &key : {QStringLiteral("el1008"),
									   QStringLiteral("el2008"),
									   QStringLiteral("el5152"),
									   QStringLiteral("ek1914")}) {
				for (const QString &suffix : {QString(),
											  QStringLiteral("_realistic")}) {
					const auto def = load(categoryDir().filePath(
						key + suffix + QStringLiteral(".elmt")))
						.documentElement();
					const QStringList names = terminalNames(def);
					QVERIFY2(!names.isEmpty(), qPrintable(key + suffix));
					const auto map = ioPins(def);
					for (auto it = map.cbegin(); it != map.cend(); ++it) {
						for (const QString &pin : it.value()) {
							QVERIFY2(names.contains(pin),
									 qPrintable(key + suffix + QLatin1Char(' ')
												+ it.key() + QLatin1Char('=')
												+ pin));
						}
					}
				}
			}
		}

		void driveAxesFollowTheSecondDigitOfTheReference()
		{
			int checked = 0;
			for (const QString &family : {QStringLiteral("drives_af1000"),
										  QStringLiteral("drives_ax5000")})
			{
				const QDir dir(categoryDir().filePath(family));
				for (const QString &file : dir.entryList(
						 {QStringLiteral("*.elmt")}, QDir::Files))
				{
					if (file.contains(QLatin1String("_realistic"))
						|| file.contains(QLatin1String("_table"))) {
						continue;
					}
					const QString key = file.section(QLatin1Char('.'), 0, 0);
					const int axes = key.mid(2, 2).at(1).digitValue();
					QVERIFY2(axes == 1 || axes == 2, qPrintable(key));

					for (const QString &suffix : {
							 QString(), QStringLiteral("_realistic"),
							 QStringLiteral("_table")}) {
						const QString name = key + suffix + QStringLiteral(".elmt");
						QVERIFY2(dir.exists(name), qPrintable(name));
						const auto def = load(dir.filePath(name))
							.documentElement();
						QCOMPARE(info(def,
									  QStringLiteral("manufacturer_reference")),
								 key.toUpper());
						if (suffix == QLatin1String("_table")) {
							QVERIFY(terminalNames(def).isEmpty());
							continue;
						}
						const QStringList names = terminalNames(def);
						QVERIFY2(names.contains(QStringLiteral("EtherCAT IN")),
								 qPrintable(name));
						QVERIFY2(names.contains(QStringLiteral("M1-U")),
								 qPrintable(name));
						QCOMPARE(names.contains(QStringLiteral("M2-U")),
								 axes == 2);
						QCOMPARE(names.size(),
								 QSet<QString>(names.cbegin(),
											   names.cend()).size());
						const auto terms = def.elementsByTagName(
							QStringLiteral("terminal"));
						for (int i = 0; i < terms.size(); ++i) {
							const auto t = terms.at(i).toElement();
							const qreal x = t.attribute(QStringLiteral("x"))
								.toDouble();
							const qreal y = t.attribute(QStringLiteral("y"))
								.toDouble();
							QVERIFY2(qFuzzyIsNull(std::fmod(x, 10))
									 && qFuzzyIsNull(std::fmod(y, 10)),
									 qPrintable(name + QLatin1Char(' ')
												+ t.attribute(QStringLiteral(
													"name"))));
						}
					}
					++checked;
				}
			}
			QVERIFY(checked >= 19);
			for (const QString &ref : {QStringLiteral("drives_af1000/af1103"),
									   QStringLiteral("drives_af1000/af1215"),
									   QStringLiteral("drives_ax5000/ax5103"),
									   QStringLiteral("drives_ax5000/ax5203"),
									   QStringLiteral("drives_ax5000/ax5206")}) {
				QVERIFY2(categoryDir().exists(ref + QStringLiteral(".elmt")),
						 qPrintable(ref));
			}
		}

		void safetyCardsAreYellowAndEl1918HasSixteenTerminals()
		{
			const QDir dir(categoryDir().filePath(QStringLiteral("twinsafe")));
			for (const QString &key : {QStringLiteral("el1904"),
									   QStringLiteral("el1918"),
									   QStringLiteral("el2904"),
									   QStringLiteral("el6910"),
									   QStringLiteral("ek1960")}) {
				const QString file = key + QStringLiteral(".elmt");
				QFile f(dir.filePath(file));
				QVERIFY2(f.open(QIODevice::ReadOnly), qPrintable(file));
				QVERIFY2(f.readAll().contains("filling:yellow"),
						 qPrintable(file));
				QVERIFY2(!info(load(dir.filePath(file)).documentElement(),
							   QStringLiteral("description")).isEmpty(),
						 qPrintable(file));
			}

			const auto def = load(dir.filePath(
				QStringLiteral("el1918.elmt"))).documentElement();
			QStringList expected;
			for (int i = 1; i <= 16; ++i) {
				expected << QString::number(i);
			}
			QCOMPARE(terminalNames(def), expected);
			const auto pins = ioPins(def);
			QCOMPARE(pins.size(), 8);
			QCOMPARE(pins.value(QStringLiteral("SI1")),
					 (QStringList{"1", "9"}));
			QCOMPARE(pins.value(QStringLiteral("SI8")),
					 (QStringList{"8", "16"}));
			QCOMPARE(terminalNames(load(dir.filePath(
				QStringLiteral("el1918_realistic.elmt"))).documentElement())
				.size(), 16);
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
			QCOMPARE(folios.size(), 9);
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
										QStringLiteral("A1.30"),
										QStringLiteral("A2.10"),
										QStringLiteral("A2.20"),
										QStringLiteral("A2.30"),
										QStringLiteral("A3.10"),
										QStringLiteral("A3.20")}) {
				const auto list = appearances.value(card);
				QCOMPARE(list.size(), 3);
				QSet<int> onFolio;
				for (const QString &a : list) {
					onFolio.insert(a.section(QLatin1Char('@'), 1).toInt());
				}
				QCOMPARE(onFolio.size(), 3);
			}

				// 1 axis and 2 axes drives, and the yellow card with 16 pins
			const auto has = [&](const QString &card, const QString &file) {
				for (const QString &a : appearances.value(card)) {
					if (a.startsWith(file + QLatin1Char('@'))) {
						return true;
					}
				}
				return false;
			};
			QVERIFY(has(QStringLiteral("A2.10"), QStringLiteral("el1918.elmt")));
			QVERIFY(has(QStringLiteral("A2.20"), QStringLiteral("af1103.elmt")));
			QVERIFY(has(QStringLiteral("A2.30"), QStringLiteral("af1215.elmt")));
			QVERIFY(has(QStringLiteral("A3.10"), QStringLiteral("ax5103.elmt")));
			QVERIFY(has(QStringLiteral("A3.20"), QStringLiteral("ax5203.elmt")));

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
