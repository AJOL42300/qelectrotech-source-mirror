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
#include <QDirIterator>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QLineF>
#include <QMap>
#include <QPointF>
#include <QSet>
#include <QtTest>
#include <algorithm>
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
			return {QStringLiteral("el1004"), QStringLiteral("el1008"),
					QStringLiteral("el2004"), QStringLiteral("el2008"),
					QStringLiteral("el2522"), QStringLiteral("el2574"),
					QStringLiteral("el5152"), QStringLiteral("el9100"),
					QStringLiteral("ek1914"), QStringLiteral("el2911"),
					QStringLiteral("el1904"), QStringLiteral("el1918"),
					QStringLiteral("el2904")};
		}

			// every .elmt of the set, relative to the category: the
			// library is sorted in one folder per family
		static QStringList allFiles()
		{
			QStringList list;
			QDirIterator it(categoryDir().path(),
							{QStringLiteral("*.elmt")}, QDir::Files,
							QDirIterator::Subdirectories);
			while (it.hasNext()) {
				list << categoryDir().relativeFilePath(it.next());
			}
			list.sort();
			return list;
		}

			// full path of a file given by its name or its relative path
		static QString elementPath(const QString &name)
		{
			if (QFileInfo::exists(categoryDir().filePath(name))) {
				return categoryDir().filePath(name);
			}
			const QString base = QFileInfo(name).fileName();
			for (const QString &file : allFiles()) {
				if (QFileInfo(file).fileName() == base) {
					return categoryDir().filePath(file);
				}
			}
			return categoryDir().filePath(name);
		}

			// path of an appearance of a card, or an empty string
		static QString appearance(const QString &key, const QString &suffix)
		{
			const QString path =
				elementPath(key + suffix + QStringLiteral(".elmt"));
			return QFileInfo::exists(path) ? path : QString();
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
			const auto files = allFiles();
			QVERIFY(files.size() >= 5);

			QSet<QString> element_uuids;
			for (const QString &file : files)
			{
				QVERIFY2(!file.contains(QLatin1Char(' ')), qPrintable(file));
				const auto doc = load(elementPath(file));
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
				const auto def = load(elementPath(file))
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
				const auto def = load(elementPath(file))
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
					 QStringLiteral("ek1914"), QStringLiteral("epp2338"),
					 QStringLiteral("el1004"), QStringLiteral("el2522")})
			{
				const auto symbol = load(elementPath(
					key + QStringLiteral(".elmt"))).documentElement();
				QVERIFY2(!symbol.isNull(), qPrintable(key));
				for (const QString &suffix : {QStringLiteral("_table"),
											  QStringLiteral("_realistic")})
				{
					const QString file = key + suffix + QStringLiteral(".elmt");
					if (!QFileInfo::exists(elementPath(file))) {
						QVERIFY2(suffix != QLatin1String("_table"),
								 qPrintable(file));
						continue;
					}
					const auto def = load(elementPath(file))
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
			const auto files = allFiles();
			for (const QString &file : files) {
				const auto def = load(elementPath(file))
					.documentElement();
				if (def.attribute(QStringLiteral("link_type"))
					!= QLatin1String("master")) {
					continue;
				}
				const auto hidden = def.elementsByTagName(
					QStringLiteral("columnVisibility")).at(0).toElement()
					.elementsByTagName(QStringLiteral("column"));
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
				const auto def = load(elementPath(file))
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
					if (d < 7) {
						continue; // decorative, like a LED
					}
					centres << QPointF(
						c.attribute(QStringLiteral("x")).toDouble() + d / 2,
						c.attribute(QStringLiteral("y")).toDouble() + d / 2);
				}
					// the screws of the cards are drawn with arcs
				const auto arcs = parts.elementsByTagName(
					QStringLiteral("arc"));
				for (int i = 0; i < arcs.size(); ++i) {
					const auto a = arcs.at(i).toElement();
					const qreal w = a.attribute(
						QStringLiteral("width")).toDouble();
					const qreal h = a.attribute(
						QStringLiteral("height")).toDouble();
					if (qAbs(w) < 10) {
						continue;
					}
					centres << QPointF(
						a.attribute(QStringLiteral("x")).toDouble() + w / 2,
						a.attribute(QStringLiteral("y")).toDouble() + h / 2);
				}

				QList<QPointF> points;
				const auto terminals = parts.elementsByTagName(
					QStringLiteral("terminal"));
				for (int i = 0; i < terminals.size(); ++i) {
					const auto t = terminals.at(i).toElement();
						// the RJ45 sockets of the coupler are not circles
					if (!t.attribute(QStringLiteral("name"))
							.startsWith(QLatin1String("EtherCAT"))) {
						points << QPointF(
							t.attribute(QStringLiteral("x")).toDouble(),
							t.attribute(QStringLiteral("y")).toDouble());
					}
				}

					// every terminal is at the centre of a circle
				QVERIFY2(points.size() >= 8, qPrintable(file));
				for (const QPointF &p : points) {
					qreal nearest = 1e9;
					for (const QPointF &c : centres) {
						nearest = qMin(nearest, QLineF(c, p).length());
					}
					QVERIFY2(nearest < 0.05, qPrintable(
						file + QStringLiteral(": ")
						+ QString::number(p.x()) + QLatin1Char(',')
						+ QString::number(p.y())));
				}

					// the grid used is written in the note of the element
				const QString note = def.firstChildElement(
					QStringLiteral("informations")).text();
				const int at = note.indexOf(QLatin1String("grid "));
				QVERIFY2(at >= 0, qPrintable(file));
				const int grid = note.mid(at + 5).split(
					QLatin1Char(' ')).first().toInt();
					// every realistic view is at the scale of the EK1914
					// (1.00), where the 10 px grid is impossible (rows 14.5
					// and 21 apart): the terminals are on the 5 px grid
				QVERIFY2(grid == 10 || grid == 5, qPrintable(file));
				QVERIFY2(note.contains(QLatin1String("scale 1.00")),
						 qPrintable(file));
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

		void ek1914BottomTerminalsAreYellow()
		{
			const auto def = load(appearance(QStringLiteral("ek1914"),
				QStringLiteral("_realistic"))).documentElement();
			const auto parts = def.firstChildElement(
				QStringLiteral("description"));
			QMap<QString, QPointF> pins;
			const auto terminals = parts.elementsByTagName(
				QStringLiteral("terminal"));
			for (int i = 0; i < terminals.size(); ++i) {
				const auto t = terminals.at(i).toElement();
				pins.insert(t.attribute(QStringLiteral("name")),
							QPointF(t.attribute(QStringLiteral("x"))
									.toDouble(),
									t.attribute(QStringLiteral("y"))
									.toDouble()));
			}
			const auto circles = parts.elementsByTagName(
				QStringLiteral("circle"));
			for (int n = 1; n <= 16; ++n) {
				const QPointF p = pins.value(QString::number(n));
				bool yellow = false;
				bool found = false;
				for (int i = 0; i < circles.size(); ++i) {
					const auto c = circles.at(i).toElement();
					const qreal d = c.attribute(
						QStringLiteral("diameter")).toDouble();
					if (d < 8) {
						continue; // the outer circle of the screw
					}
					const QPointF centre(
						c.attribute(QStringLiteral("x")).toDouble() + d / 2,
						c.attribute(QStringLiteral("y")).toDouble() + d / 2);
					if (QLineF(centre, p).length() < 2.5) {
						found = true;
						yellow |= c.attribute(QStringLiteral("style"))
							.contains(QLatin1String("filling:yellow"));
					}
				}
				QVERIFY2(found, qPrintable(QString::number(n)));
					// the lowest block, the terminals 11 to 16
				QCOMPARE(yellow, n >= 11);
			}
		}

		void referenceIsInsideTheCardAtTheBottom()
		{
			for (const QString &key : {QStringLiteral("el1004"),
									   QStringLiteral("el2911"),
									   QStringLiteral("ek1914"),
									   QStringLiteral("el1918")}) {
				const auto def = load(appearance(key,
					QStringLiteral("_realistic"))).documentElement();
				const auto parts = def.firstChildElement(
					QStringLiteral("description"));
				qreal minX = 1e9, maxY = -1e9;
				const auto terminals = parts.elementsByTagName(
					QStringLiteral("terminal"));
				for (int i = 0; i < terminals.size(); ++i) {
					const auto t = terminals.at(i).toElement();
					if (t.attribute(QStringLiteral("name"))
							.startsWith(QLatin1String("EtherCAT"))) {
						continue;
					}
					minX = qMin(minX, t.attribute(QStringLiteral("x"))
								.toDouble());
					maxY = qMax(maxY, t.attribute(QStringLiteral("y"))
								.toDouble());
				}
				bool seen = false;
				const auto texts = parts.elementsByTagName(
					QStringLiteral("text"));
				for (int i = 0; i < texts.size(); ++i) {
					const auto t = texts.at(i).toElement();
					if (t.attribute(QStringLiteral("text"))
							!= key.toUpper()) {
						continue;
					}
					seen = true;
					const qreal x = t.attribute(QStringLiteral("x"))
						.toDouble();
					const qreal y = t.attribute(QStringLiteral("y"))
						.toDouble();
					if (key == QLatin1String("ek1914")) {
							// in the body, on the left of the terminals,
							// above the bottom of the card
						QVERIFY(x < minX - 20);
						QVERIFY(y < maxY + 20);
					} else if (key == QLatin1String("el1918")) {
						QVERIFY(x < minX);
						QVERIFY(y > maxY);
					} else {
							// under the first column of terminals, in
							// the first card
						QVERIFY2(x <= minX, qPrintable(key));
						QVERIFY2(y >= maxY, qPrintable(key));
					}
				}
				QVERIFY2(seen, qPrintable(key));
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
			const auto el1008 = load(elementPath(
				QStringLiteral("el1008.elmt"))).documentElement();
			const auto in = ioPins(el1008);
			QCOMPARE(in.value(QStringLiteral("I1")), QStringList{"1"});
			QCOMPARE(in.value(QStringLiteral("I2")), QStringList{"5"});
			QCOMPARE(in.value(QStringLiteral("I3")), QStringList{"2"});
			QCOMPARE(in.value(QStringLiteral("I8")), QStringList{"8"});

			const auto el2008 = load(elementPath(
				QStringLiteral("el2008.elmt"))).documentElement();
			QCOMPARE(ioPins(el2008).value(QStringLiteral("O2")),
					 QStringList{"5"});

			const auto el5152 = load(elementPath(
				QStringLiteral("el5152.elmt"))).documentElement();
			QCOMPARE(ioPins(el5152).value(QStringLiteral("A2")),
					 QStringList{"5"});

			const auto el1004 = load(elementPath(
				QStringLiteral("el1004.elmt"))).documentElement();
			QCOMPARE(ioPins(el1004).value(QStringLiteral("I2")),
					 QStringList{"5"});
			QCOMPARE(ioPins(el1004).value(QStringLiteral("I3")),
					 QStringList{"3"});

				// every listed pin is a terminal of the symbol and of its
				// realistic view
			for (const QString &key : {QStringLiteral("el1004"),
									   QStringLiteral("el1008"),
									   QStringLiteral("el2008"),
									   QStringLiteral("el5152"),
									   QStringLiteral("ek1914")}) {
				for (const QString &suffix : {QString(),
											  QStringLiteral("_realistic")}) {
					const auto def = load(elementPath(
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
				const QDir dir(elementPath(family));
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
			const QDir dir(elementPath(QStringLiteral("twinsafe")));
			for (const QString &key : {QStringLiteral("el1904"),
									   QStringLiteral("el1918"),
									   QStringLiteral("el2904"),
									   QStringLiteral("el6910"),
									   QStringLiteral("ek1960")}) {
				const QString file = key + QStringLiteral(".elmt");
				QVERIFY2(!info(load(dir.filePath(file)).documentElement(),
							   QStringLiteral("description")).isEmpty(),
						 qPrintable(file));
					// no generic view any more: symbol, realistic, table
				QVERIFY2(!dir.exists(key + QStringLiteral("_generic.elmt")),
						 qPrintable(key));
				QFile f(dir.filePath(key + QStringLiteral("_realistic.elmt")));
				if (!f.exists()) {
					QVERIFY2(key == QLatin1String("el6910")
							 || key == QLatin1String("ek1960"),
							 qPrintable(f.fileName()));
					continue;
				}
				QVERIFY2(f.open(QIODevice::ReadOnly),
						 qPrintable(f.fileName()));
				QVERIFY2(f.readAll().contains("filling:yellow"),
						 qPrintable(f.fileName()));
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
				// public Beckhoff order: the input n is on the terminal 2k,
				// the clock output Sn+ on the terminal 2k-1, with the order
				// of the inputs 1, 3, 5, 7, 2, 4, 6, 8
			QCOMPARE(pins.value(QStringLiteral("SI1")), QStringList{"2"});
			QCOMPARE(pins.value(QStringLiteral("SI3")), QStringList{"4"});
			QCOMPARE(pins.value(QStringLiteral("SI2")), QStringList{"10"});
			QCOMPARE(pins.value(QStringLiteral("SI8")), QStringList{"16"});
			const auto real = load(dir.filePath(
				QStringLiteral("el1918_realistic.elmt"))).documentElement();
			QCOMPARE(terminalNames(real).size(), 16);
			QCOMPARE(ioPins(real), pins);
		}

		void tablesUseRobotoCondensedWithRoomForEveryColumn()
		{
			int checked = 0;
			for (const QString &key : cardKeys() + QStringList{
					 QStringLiteral("el2652"), QStringLiteral("epp2338")}) {
				const QString path = appearance(key, QStringLiteral("_table"));
				if (path.isEmpty()) {
					QVERIFY2(key == QLatin1String("el9100"), qPrintable(key));
					continue;
				}
				QFile f(path);
				QVERIFY(f.open(QIODevice::ReadOnly));
				const QString text = QString::fromUtf8(f.readAll());
				QVERIFY2(text.contains(QLatin1String("Roboto Condensed")),
						 qPrintable(key));
				QVERIFY2(text.contains(QLatin1String("rowHeight=\"12.00\"")),
						 qPrintable(key));
				QVERIFY2(text.contains(QLatin1String("<columnWidths>")),
						 qPrintable(key));
				++checked;
			}
			QVERIFY(checked >= 10);
		}

		void newCardsHaveAllTheirAppearances()
		{
			for (const QString &key : {QStringLiteral("el1004"),
									   QStringLiteral("el2004"),
									   QStringLiteral("el2522"),
									   QStringLiteral("el2574")}) {
				for (const QString &suffix : {QString(),
											  QStringLiteral("_realistic"),
											  QStringLiteral("_table")}) {
					QVERIFY2(!appearance(key, suffix).isEmpty(),
							 qPrintable(key + suffix));
				}
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
				QVERIFY2(list.size() >= 3, qPrintable(card));
				QSet<int> onFolio;
				for (const QString &a : list) {
					onFolio.insert(a.section(QLatin1Char('@'), 1).toInt());
				}
				QCOMPARE(onFolio.size(), list.size());
			}

				// nothing of the collection of the user is embedded
			QFile raw(QStringLiteral(QET_EXAMPLES_DIR)
					  + QStringLiteral("/beckhoff_islands_example.qet"));
			QVERIFY(raw.open(QIODevice::ReadOnly));
			const QByteArray bytes = raw.readAll();
			QVERIFY(!bytes.contains("Entreprise"));
			QVERIFY(!bytes.contains("mcatech"));
			QVERIFY(!bytes.contains("company"));

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

		void a3TitleBlocksAreShippedAndAppliedToTheExample()
		{
			const QString dir = QStringLiteral(QET_EXAMPLES_DIR)
				+ QStringLiteral("/../titleblocks/");
			const QMap<QString, QString> files{
				{QStringLiteral("A3 landscape"),
				 QStringLiteral("A3_landscape.titleblock")},
				{QStringLiteral("A3 portrait"),
				 QStringLiteral("A3_portrait.titleblock")}};
			QMap<QString, QString> grids;
			for (auto it = files.cbegin(); it != files.cend(); ++it) {
				const auto doc = load(dir + it.value());
				const auto root = doc.documentElement();
				QCOMPARE(root.tagName(),
						 QStringLiteral("titleblocktemplate"));
				QCOMPARE(root.attribute(QStringLiteral("name")), it.key());
					// no logo is embedded, the first cell stays empty
				QCOMPARE(root.firstChildElement(QStringLiteral("logos"))
						 .childNodes().size(), 0);
				const auto grid = root.firstChildElement(
					QStringLiteral("grid"));
				QCOMPARE(grid.attribute(QStringLiteral("rows")),
						 QStringLiteral("25;25;"));
				QVERIFY(grid.elementsByTagName(QStringLiteral("field"))
						.size() >= 6);
				QVERIFY(grid.elementsByTagName(QStringLiteral("logo"))
						.isEmpty());
				grids.insert(it.key(),
							 grid.attribute(QStringLiteral("cols")));
			}

			const auto doc = load(QStringLiteral(QET_EXAMPLES_DIR)
				+ QStringLiteral("/beckhoff_islands_example.qet"));
			const auto project = doc.documentElement();
			const auto templates = project.firstChildElement(
				QStringLiteral("titleblocktemplates"))
				.elementsByTagName(QStringLiteral("titleblocktemplate"));
			QCOMPARE(templates.size(), 2);
			for (int i = 0; i < templates.size(); ++i) {
				const auto t = templates.at(i).toElement();
				QCOMPARE(t.firstChildElement(QStringLiteral("grid"))
						 .attribute(QStringLiteral("cols")),
						 grids.value(t.attribute(QStringLiteral("name"))));
			}

			int portrait = 0;
			const auto folios = project.elementsByTagName(
				QStringLiteral("diagram"));
			for (int i = 0; i < folios.size(); ++i) {
				const auto d = folios.at(i).toElement();
				const QString name = d.attribute(
					QStringLiteral("titleblocktemplate"));
				QVERIFY2(files.contains(name), qPrintable(name));
				QCOMPARE(d.attribute(
					QStringLiteral("titleblocktemplateCollection")),
					QStringLiteral("embedded"));
				const int w = d.attribute(QStringLiteral("cols")).toInt()
					* d.attribute(QStringLiteral("colsize")).toInt();
				const int h = d.attribute(QStringLiteral("rows")).toInt()
					* d.attribute(QStringLiteral("rowsize")).toInt();
				const bool isPortrait =
					name == QLatin1String("A3 portrait");
				QVERIFY(isPortrait ? h > w : w > h);
				portrait += isPortrait ? 1 : 0;
					// A3 is 420 x 297 mm: about 2.9 px per mm
				QVERIFY(qAbs(std::max(w, h) - 420 * 2.9) < 80);
				QVERIFY(qAbs(std::min(w, h) - 297 * 2.9) < 80);
					// the origin of every element is inside the frame,
					// above the title block
				const auto els = d.firstChildElement(
					QStringLiteral("elements"))
					.elementsByTagName(QStringLiteral("element"));
				for (int j = 0; j < els.size(); ++j) {
					const auto e = els.at(j).toElement();
					const qreal x = e.attribute(QStringLiteral("x")).toDouble();
					const qreal y = e.attribute(QStringLiteral("y")).toDouble();
					QVERIFY(x > 0 && x < w);
					QVERIFY(y > 0 && y < h - 50);
				}
			}
			QVERIFY(portrait >= 1);
			QVERIFY(portrait < folios.size());
		}
};

QTEST_GUILESS_MAIN(TstBeckhoffElements)
#include "tst_beckhoffelements.moc"
