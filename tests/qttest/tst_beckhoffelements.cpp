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
#include <QRegularExpression>
#include <algorithm>
#include <cmath>

class TstBeckhoffElements : public QObject
{
	Q_OBJECT

	private:
		static QDir categoryDir()
		{
			return QDir(QStringLiteral(QET_EXTRA_ELEMENTS_DIR)
						+ QStringLiteral("/10_electric/20_manufacturers_articles/beckhoff"));
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
						// pin (5), label (2), TwinCAT ID (1) only
					QCOMPARE(hidden.size(), 6);
					const QStringList shown{QStringLiteral("1"),
											QStringLiteral("2"),
											QStringLiteral("5")};
					for (int i = 0; i < hidden.size(); ++i) {
						const auto c = hidden.at(i).toElement();
						QCOMPARE(c.attribute(QStringLiteral("visible")),
								 shown.contains(c.attribute(
									 QStringLiteral("index")))
								 ? QStringLiteral("true")
								 : QStringLiteral("false"));
					}
					QCOMPARE(def.elementsByTagName(
								 QStringLiteral("columnOrder")).at(0)
								 .toElement().text(),
							 QStringLiteral("5,2,1"));
					QVERIFY(load(elementPath(file)).toString().contains(
						QStringLiteral("ID TwinCAT")));
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
					// the lowest block, rows 6 to 8 of both columns
				QCOMPARE(yellow, (n - 1) % 8 >= 5);
			}
		}

		void ek1914HasTheProportionsOfItsHousing()
		{
				// 44 mm x 100 mm housing (public data sheet), like the
				// EK1100: the drawing was too narrow
			const auto def = load(appearance(QStringLiteral("ek1914"),
				QStringLiteral("_realistic"))).documentElement();
			QCOMPARE(def.attribute(QStringLiteral("height")).toInt(), 330);
			const auto parts = def.firstChildElement(
				QStringLiteral("description"));
			qreal x0 = 1e9, x1 = -1e9, y0 = 1e9, y1 = -1e9;
			for (const QString &tag : {QStringLiteral("line"),
										QStringLiteral("rect")}) {
				const auto list = parts.elementsByTagName(tag);
				for (int i = 0; i < list.size(); ++i) {
					const auto e = list.at(i).toElement();
					const qreal ax = tag == QLatin1String("line")
						? e.attribute(QStringLiteral("x1")).toDouble()
						: e.attribute(QStringLiteral("x")).toDouble();
					const qreal bx = tag == QLatin1String("line")
						? e.attribute(QStringLiteral("x2")).toDouble()
						: ax + e.attribute(QStringLiteral("width"))
						.toDouble();
					const qreal ay = tag == QLatin1String("line")
						? e.attribute(QStringLiteral("y1")).toDouble()
						: e.attribute(QStringLiteral("y")).toDouble();
					const qreal by = tag == QLatin1String("line")
						? e.attribute(QStringLiteral("y2")).toDouble()
						: ay + e.attribute(QStringLiteral("height"))
						.toDouble();
					x0 = qMin(x0, qMin(ax, bx));
					x1 = qMax(x1, qMax(ax, bx));
					y0 = qMin(y0, qMin(ay, by));
					y1 = qMax(y1, qMax(ay, by));
				}
			}
			const qreal ratio = (x1 - x0) / (y1 - y0);
			QVERIFY2(ratio > 0.42 && ratio < 0.52,
					 qPrintable(QString::number(ratio)));
			const auto pins = pinPoints(parts);
			for (auto it = pins.constBegin(); it != pins.constEnd(); ++it) {
				QVERIFY2(it.value().x() > x0 && it.value().x() < x1,
						 qPrintable(it.key()));
			}
		}

		void ledsHaveNoResistorAndOutputsAreRoundGenerators()
		{
				// a resistor was a 4 point polygon: none is left; the
				// outputs of the EL2008 are circles of 9 with a "+" (more than the 8 terminals)
			for (const QString &key : {QStringLiteral("el1008"),
					QStringLiteral("el2008"), QStringLiteral("el5152")}) {
				const auto parts = load(appearance(key, QString()))
					.documentElement().firstChildElement(
					QStringLiteral("description"));
				const auto polys = parts.elementsByTagName(
					QStringLiteral("polygon"));
				for (int i = 0; i < polys.size(); ++i) {
					QVERIFY2(polys.at(i).toElement().attribute(
						QStringLiteral("x4")).isEmpty(), qPrintable(key));
				}
			}
			const auto parts = load(appearance(QStringLiteral("el2008"),
				QString())).documentElement().firstChildElement(
				QStringLiteral("description"));
			int generators = 0;
			const auto circles = parts.elementsByTagName(
				QStringLiteral("circle"));
			for (int i = 0; i < circles.size(); ++i) {
				generators += qFuzzyCompare(circles.at(i).toElement()
					.attribute(QStringLiteral("diameter")).toDouble(), 9.0);
			}
			QVERIFY2(generators > 8, qPrintable(QString::number(generators)));
		}
		void ek1914NumbersAreOutsideTheBlockNextToTheirTerminal()
		{
			const auto parts = load(appearance(QStringLiteral("ek1914"),
				QStringLiteral("_realistic"))).documentElement()
				.firstChildElement(QStringLiteral("description"));
			const auto pins = pinPoints(parts);
			bool yellowBlock = false;
			const auto rects = parts.elementsByTagName(QStringLiteral("rect"));
			for (int i = 0; i < rects.size(); ++i) {
				const auto r = rects.at(i).toElement();
				const QRectF box(r.attribute(QStringLiteral("x")).toDouble(),
								 r.attribute(QStringLiteral("y")).toDouble(),
								 r.attribute(QStringLiteral("width"))
								 .toDouble(),
								 r.attribute(QStringLiteral("height"))
								 .toDouble());
				if (r.attribute(QStringLiteral("style"))
						.contains(QLatin1String("filling:yellow"))
						&& box.contains(pins.value(QStringLiteral("6")))
						&& box.contains(pins.value(QStringLiteral("16")))
						&& !box.contains(pins.value(QStringLiteral("5")))) {
					yellowBlock = true;
				}
			}
				// the whole lower block is yellow, not only the circles
			QVERIFY(yellowBlock);

			int seen = 0;
			const auto texts = parts.elementsByTagName(QStringLiteral("text"));
			for (int i = 0; i < texts.size(); ++i) {
				const auto t = texts.at(i).toElement();
				const QString name = t.attribute(QStringLiteral("text"));
				bool number = false;
				const int pin = name.toInt(&number);
				if (!number || pin < 1 || pin > 16) {
					continue;
				}
				++seen;
				const QPointF p = pins.value(name);
				const qreal x = t.attribute(QStringLiteral("x")).toDouble();
				const qreal y = t.attribute(QStringLiteral("y")).toDouble();
					// 7 pt digits: outside the circles (radius 4.25), a
					// small gap from the circle, on the line of the terminal
				if (pin <= 8) {
					QVERIFY2(x + 4 * name.size() < p.x() - 4.25,
							 qPrintable(name));
					QVERIFY2(p.x() - x < 25, qPrintable(name));
				} else {
					QVERIFY2(x > p.x() + 4.25, qPrintable(name));
					QVERIFY2(x - p.x() < 25, qPrintable(name));
				}
				QVERIFY2(qAbs(y + 5.5 - p.y()) < 4, qPrintable(name));
			}
			QCOMPARE(seen, 16);
		}

		void el1918HasTheHousingOfTheOtherCardsWithSixteenPins()
		{
			const auto def = load(appearance(QStringLiteral("el1918"),
				QStringLiteral("_realistic"))).documentElement();
			const auto el = pinPoints(def.firstChildElement(
				QStringLiteral("description")));
				// same housing as the EL1004
			const auto ref = load(appearance(QStringLiteral("el1004"),
				QStringLiteral("_realistic"))).documentElement();
			QCOMPARE(def.attribute(QStringLiteral("width")),
					 ref.attribute(QStringLiteral("width")));
			QCOMPARE(def.attribute(QStringLiteral("height")),
					 ref.attribute(QStringLiteral("height")));
			QCOMPARE(el.size(), 16);
			for (int n = 1; n <= 16; ++n) {
				const QString name = QString::number(n);
				QVERIFY2(el.contains(name), qPrintable(name));
					// 1..8 in the left column, 9..16 in the right one
				QCOMPARE(el.value(name).x(), n <= 8 ? -5.0 : 10.0);
				const int row = (n - 1) % 8;
				QCOMPARE(el.value(name).y(),
						 el.value(QStringLiteral("1")).y() + 25.0 * row);
			}
		}

		void everyCardWithTerminalsHasTheSameHeight()
		{
			for (const QString &key : {QStringLiteral("el1004"),
									   QStringLiteral("el1008"),
									   QStringLiteral("el1904"),
									   QStringLiteral("el1918"),
									   QStringLiteral("el2911"),
									   QStringLiteral("ek1914"),
									   QStringLiteral("el5152")}) {
				const auto def = load(appearance(key,
					QStringLiteral("_realistic"))).documentElement();
				QCOMPARE(def.attribute(QStringLiteral("height")),
						 QStringLiteral("330"));
				QCOMPARE(def.attribute(QStringLiteral("hotspot_y")),
						 QStringLiteral("160"));
			}
		}

		void schematicViewsShowThePowerSupplyAndTheContacts()
		{
			int checked = 0;
			for (const QString &key : cardKeys()) {
				const QString path = elementPath(key + QStringLiteral(".elmt"));
				if (!QFile::exists(path)) {
					continue;
				}
				const QString xml = load(path).toString();
					// the rails carry no text any more: square on the left
					// where the power contacts are, round on the right
				QVERIFY2(!xml.contains(QStringLiteral("text=\"+24 V\"")),
						 qPrintable(key));
				const bool left = key != QStringLiteral("el2652")
					&& key != QStringLiteral("el2911")
					&& key != QStringLiteral("ek1914");
				if (key == QStringLiteral("el2652")) {
					QVERIFY2(!xml.contains(QRegularExpression(
								 QStringLiteral("<rect[^>]*filling:red"))),
							 qPrintable(key));
				} else {
					QVERIFY2(xml.contains(QStringLiteral("color:red"))
							 && xml.contains(QStringLiteral("color:blue")),
							 qPrintable(key));
					const QRegularExpression square(
						QStringLiteral("<rect[^>]*filling:red"));
					QCOMPARE(xml.contains(square), left);
				}				++checked;
			}
			QCOMPARE(checked, cardKeys().size());
		}

		void realisticTerminalsAreOnlyACircleWithTheLabelAbove()
		{
			int checked = 0;
			for (const QString &key : {QStringLiteral("el1004"),
									   QStringLiteral("el1008"),
									   QStringLiteral("el2004"),
									   QStringLiteral("el2008"),
									   QStringLiteral("el1904"),
									   QStringLiteral("el2904"),
									   QStringLiteral("el2911")}) {
				const auto parts = load(appearance(key,
					QStringLiteral("_realistic"))).documentElement()
					.firstChildElement(QStringLiteral("description"));
				const auto pins = pinPoints(parts);
				const auto lines = parts.elementsByTagName(
					QStringLiteral("line"));
				const auto texts = parts.elementsByTagName(
					QStringLiteral("text"));
				for (auto it = pins.cbegin(); it != pins.cend(); ++it) {
					// the terminals 9 and 13 of the EL2911 are not used
					if (key == QLatin1String("el2911")
							&& (it.key() == QLatin1String("9")
								|| it.key() == QLatin1String("13"))) {
						continue;
					}
					const QPointF p = it.value();
						// no frame left above the circle
					for (int i = 0; i < lines.size(); ++i) {
						const auto l = lines.at(i).toElement();
						const qreal x1 = l.attribute(QStringLiteral("x1"))
							.toDouble();
						const qreal x2 = l.attribute(QStringLiteral("x2"))
							.toDouble();
						const qreal y1 = l.attribute(QStringLiteral("y1"))
							.toDouble();
						const qreal y2 = l.attribute(QStringLiteral("y2"))
							.toDouble();
						const bool frame =
							qMin(x1, x2) >= p.x() - 6
							&& qMax(x1, x2) <= p.x() + 6.5
							&& qMin(y1, y2) >= p.y() - 25.5
							&& qMax(y1, y2) <= p.y() - 9.5;
						QVERIFY2(!frame, qPrintable(key + QLatin1Char(' ')
							+ it.key()));
					}
						// the label of the terminal is above the circle
					bool label = false;
					for (int i = 0; i < texts.size(); ++i) {
						const auto t = texts.at(i).toElement();
						const qreal x = t.attribute(QStringLiteral("x"))
							.toDouble();
						const qreal y = t.attribute(QStringLiteral("y"))
							.toDouble();
						label |= x > p.x() - 22 && x < p.x() + 2
							&& y > p.y() - 30 && y < p.y() - 6;
					}
					QVERIFY2(label, qPrintable(key + QLatin1Char(' ')
												+ it.key()));
					++checked;
				}
			}
			QVERIFY(checked >= 40);
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

		static QMap<QString, QPointF> pinPoints(const QDomElement &parts)
		{
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
			return pins;
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
			QStringList cards;
			for (const QString &family : {QStringLiteral("drives_af1000"),
										  QStringLiteral("drives_ax5000")})
			{
				const QDir familyDir(categoryDir().filePath(family));
				for (const QString &card : familyDir.entryList(
						 QDir::Dirs | QDir::NoDotAndDotDot)) {
					cards << family + QLatin1Char('/') + card;
				}
			}
			for (const QString &cardPath : cards)
			{
				const QDir dir(categoryDir().filePath(cardPath));
				for (const QString &file : dir.entryList(
						 {QStringLiteral("*.elmt")}, QDir::Files))
				{
					if (file.contains(QLatin1String("_realistic"))
						|| file.contains(QLatin1String("_description"))
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
			for (const QString &ref : {QStringLiteral("drives_af1000/af1103/af1103"),
									   QStringLiteral("drives_af1000/af1215/af1215"),
									   QStringLiteral("drives_ax5000/ax5103/ax5103"),
									   QStringLiteral("drives_ax5000/ax5203/ax5203"),
									   QStringLiteral("drives_ax5000/ax5206/ax5206")}) {
				QVERIFY2(categoryDir().exists(ref + QStringLiteral(".elmt")),
						 qPrintable(ref));
			}
		}

		void safetyCardsAreYellowAndEl1918HasSixteenTerminals()
		{
			const auto dirOf = [](const QString &key) {
				return QDir(categoryDir().filePath(
					QStringLiteral("twinsafe/") + key));
			};
			for (const QString &key : {QStringLiteral("el1904"),
									   QStringLiteral("el1918"),
									   QStringLiteral("el2904"),
									   QStringLiteral("el6910")}) {
				const QDir dir = dirOf(key);
				const QString file = key + QStringLiteral(".elmt");
				QVERIFY2(!info(load(dir.filePath(file)).documentElement(),
							   QStringLiteral("description")).isEmpty(),
						 qPrintable(file));
					// no generic view any more: symbol, realistic, table
				QVERIFY2(!dir.exists(key + QStringLiteral("_generic.elmt")),
						 qPrintable(key));
				QFile f(dir.filePath(key + QStringLiteral("_realistic.elmt")));
				if (!f.exists()) {
					QVERIFY2(key == QLatin1String("el6910"),
							 qPrintable(f.fileName()));
					continue;
				}
				QVERIFY2(f.open(QIODevice::ReadOnly),
						 qPrintable(f.fileName()));
				QVERIFY2(f.readAll().contains("filling:yellow"),
						 qPrintable(f.fileName()));
			}

			const QDir dir = dirOf(QStringLiteral("el1918"));
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
			QSet<QString> catalogue;
			QStringList titles;
			for (int i = 0; i < folios.size(); ++i, ++folio) {
				const bool inCatalogue = folios.at(i).toElement().attribute(
					QStringLiteral("title")).startsWith(
					QStringLiteral("Catalogue"));
				titles << folios.at(i).toElement().attribute(
					QStringLiteral("title"));
				const auto els = folios.at(i).toElement()
					.firstChildElement(QStringLiteral("elements"))
					.elementsByTagName(QStringLiteral("element"));
				for (int j = 0; j < els.size(); ++j) {
					const auto e = els.at(j).toElement();
					const QString file = e.attribute(QStringLiteral("type"))
						.section(QLatin1Char('/'), -1);
					QVERIFY2(embedded.contains(file), qPrintable(file));
					used.insert(e.attribute(QStringLiteral("uuid")));
					if (inCatalogue) {
						catalogue.insert(file);
					}
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

				// the folios are named by card category: the islands
				// (13 folios) come first, then one catalogue per category
				// with the symbol of every card of the library
			QVERIFY(folios.size() > 13);
			QVERIFY(titles.at(0).contains(QStringLiteral("Coupleurs")));
			QVERIFY(titles.at(0).contains(QStringLiteral("Entrées TOR")));
			QVERIFY(titles.at(4).contains(QStringLiteral("TwinSAFE")));
			QVERIFY(titles.at(4).contains(QStringLiteral("Variateurs")));
			QVERIFY(titles.at(10).contains(QStringLiteral("EPP")));
			QVERIFY(titles.at(13).startsWith(QStringLiteral("Catalogue")));
			QVERIFY(titles.filter(QStringLiteral("Catalogue - Sorties TOR"))
					.size() == 1);
			int cards = 0;
			for (const QString &file : allFiles()) {
				const QFileInfo info(file);
				if (info.completeBaseName() == QFileInfo(info.path())
						.fileName()) {
					++cards;
					QVERIFY2(catalogue.contains(info.fileName()),
							 qPrintable(file));
				}
			}
			QVERIFY(cards >= 40);

			QCOMPARE(appearances.value(QStringLiteral("A1.00")).size(), 1);
			for (const QString &card : {QStringLiteral("A1.10"),
										QStringLiteral("A1.20"),
										QStringLiteral("A1.30"),
										QStringLiteral("A2.10"),
										QStringLiteral("A2.20"),
										QStringLiteral("A2.30"),
										QStringLiteral("A3.10"),
										QStringLiteral("A3.20"),
										QStringLiteral("A4.10")}) {
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
				// the EtherCAT P island: junction head and EPP box
			QVERIFY(has(QStringLiteral("A4.00"), QStringLiteral("ek1322.elmt")));
			QVERIFY(has(QStringLiteral("A4.00"),
					   QStringLiteral("ek1322_realistic.elmt")));
			QVERIFY(has(QStringLiteral("A4.10"), QStringLiteral("epp2338.elmt")));

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

		void everyRealisticViewHasADescriptionViewWithTheSameTerminals()
		{
			int checked = 0;
			for (const QString &file : allFiles()) {
				const QString suffix = QStringLiteral("_realistic.elmt");
				if (!file.endsWith(suffix)) {
					continue;
				}
				const QString key = QFileInfo(file).fileName()
					.chopped(suffix.size());
				const QString path = appearance(
					key, QStringLiteral("_description"));
				QVERIFY2(!path.isEmpty(), qPrintable(key));
				QCOMPARE(QFileInfo(path).path(),
						 QFileInfo(elementPath(file)).path());

				const auto real = load(elementPath(file)).documentElement();
				const auto desc = load(path).documentElement();
				QVERIFY2(!desc.isNull(), qPrintable(key));
				QCOMPARE(desc.attribute(QStringLiteral("link_type")),
						 real.attribute(QStringLiteral("link_type")));
				QCOMPARE(pinPoints(desc), pinPoints(real));
				QCOMPARE(terminalNames(desc), terminalNames(real));
				QCOMPARE(ioAddresses(desc), ioAddresses(real));
				QCOMPARE(info(desc, QStringLiteral("manufacturer_reference")),
						 info(real, QStringLiteral("manufacturer_reference")));

					// the legends fit in the declared box
				const qreal hx = desc.attribute(
					QStringLiteral("hotspot_x")).toDouble();
				const qreal hy = desc.attribute(
					QStringLiteral("hotspot_y")).toDouble();
				const qreal w = desc.attribute(
					QStringLiteral("width")).toDouble();
				const qreal h = desc.attribute(
					QStringLiteral("height")).toDouble();
				QVERIFY2(w >= real.attribute(QStringLiteral("width"))
						 .toDouble(), qPrintable(key));
				const auto texts = desc.firstChildElement(
					QStringLiteral("description")).elementsByTagName(
					QStringLiteral("text"));
				QVERIFY2(texts.size() > real.firstChildElement(
					QStringLiteral("description")).elementsByTagName(
					QStringLiteral("text")).size(), qPrintable(key));
				for (int i = 0; i < texts.size(); ++i) {
					const auto t = texts.at(i).toElement();
					const qreal x = t.attribute(QStringLiteral("x"))
						.toDouble();
					const qreal y = t.attribute(QStringLiteral("y"))
						.toDouble();
					QVERIFY2(x >= -hx && x <= w - hx
							 && y >= -hy && y <= h - hy,
							 qPrintable(key + QLatin1Char(' ')
										+ t.attribute(QStringLiteral("text"))));
				}
				++checked;
			}
			QCOMPARE(checked, 46);
		}

		void schematicViewsAreDrawnInADottedOutlineAtTheRealisticPins()
		{
			int pinned = 0;
			int outlined = 0;
			for (const QString &file : allFiles()) {
				if (file.endsWith(QLatin1String("_realistic.elmt"))
					|| file.endsWith(QLatin1String("_description.elmt"))
					|| file.endsWith(QLatin1String("_table.elmt"))
					|| file.endsWith(QLatin1String("qet_directory"))) {
					continue;
				}
				const auto sym = load(elementPath(file)).documentElement()
					.firstChildElement(QStringLiteral("description"));
					// not a plain rectangle: the dotted outline of the card
				bool dotted = false;
				for (auto n = sym.firstChildElement(); !n.isNull();
					 n = n.nextSiblingElement()) {
					dotted |= n.attribute(QStringLiteral("style")).contains(
						QLatin1String("line-style:dotted"));
				}
				QVERIFY2(dotted, qPrintable(file));
				++outlined;

				const QString key = QFileInfo(file).fileName().chopped(5);
				const QString real = appearance(
					key, QStringLiteral("_realistic"));
				if (real.isEmpty() || key.startsWith(QLatin1String("af"))
					|| key.startsWith(QLatin1String("ax"))) {
					continue;
				}
				const auto rdesc = load(real).documentElement()
					.firstChildElement(QStringLiteral("description"));
				QCOMPARE(pinPoints(sym), pinPoints(rdesc));
				++pinned;
			}
			QVERIFY(outlined >= 30);
			QVERIFY(pinned >= 15);
		}

		void genericCardsHaveARealisticViewAndATable()
		{
			for (const QString &key : {QStringLiteral("el1xxx_generic"),
									   QStringLiteral("el2xxx_generic"),
									   QStringLiteral("el3xxx_generic"),
									   QStringLiteral("el4xxx_generic"),
									   QStringLiteral("el2652")}) {
				for (const QString &suffix : {QStringLiteral("_realistic"),
											  QStringLiteral("_description"),
											  QStringLiteral("_table")}) {
					QVERIFY2(!appearance(key, suffix).isEmpty(),
							 qPrintable(key + suffix));
				}
			}
		}

		void generic16CardsHave16TerminalsInEveryAppearance()
		{
			for (const QString &key : {QStringLiteral("el1xxx_generic_16"),
									   QStringLiteral("el2xxx_generic_16"),
									   QStringLiteral("el3xxx_generic_16"),
									   QStringLiteral("el4xxx_generic_16")}) {
				const auto symbol = load(appearance(key, QString()))
					.documentElement();
				const auto real = load(appearance(
					key, QStringLiteral("_realistic"))).documentElement();
				QCOMPARE(pinPoints(real.firstChildElement(
							 QStringLiteral("description"))).size(), 16);
				QCOMPARE(pinPoints(symbol.firstChildElement(
							 QStringLiteral("description"))),
						 pinPoints(real.firstChildElement(
							 QStringLiteral("description"))));
				for (const QString &suffix : {QStringLiteral("_description"),
											  QStringLiteral("_table")}) {
					const auto def = load(appearance(key, suffix))
						.documentElement();
					QVERIFY2(!def.isNull(), qPrintable(key + suffix));
					if (suffix == QStringLiteral("_table")) {
						QCOMPARE(def.elementsByTagName(
									 QStringLiteral("plcIO")).size(), 16);
					}
				}
			}
		}

		void descriptionViewsDrawTheLedsAndTheLegendsOfTheChannels()
		{
			const auto desc = load(appearance(
				QStringLiteral("el1004"), QStringLiteral("_description")))
				.documentElement();
			const auto real = load(appearance(
				QStringLiteral("el1004"), QStringLiteral("_realistic")))
				.documentElement();
			QStringList legends;
			const auto texts = desc.elementsByTagName(QStringLiteral("text"));
			for (int i = 0; i < texts.size(); ++i) {
				legends << texts.at(i).toElement()
					.attribute(QStringLiteral("text"));
			}
				// the LEDs are named after the channel type, not "Signal"
			for (int n = 1; n <= 4; ++n) {
				QVERIFY(legends.contains(QStringLiteral("Input %1").arg(n)));
			}
			QVERIFY(!legends.contains(QStringLiteral("Signal 1")));
			QVERIFY(legends.contains(QStringLiteral("1: I1 - Input 1")));
			QVERIFY(legends.contains(QStringLiteral("5: I2 - Input 2")));
				// one green LED per channel on top of the realistic view
			int green = 0;
			const auto circles = desc.elementsByTagName(
				QStringLiteral("circle"));
			for (int i = 0; i < circles.size(); ++i) {
				if (circles.at(i).toElement().attribute(
						QStringLiteral("style")).contains(
						QLatin1String("filling:green"))) {
					++green;
				}
			}
			QVERIFY(green >= 4);
			QVERIFY(desc.elementsByTagName(QStringLiteral("circle")).size()
					> real.elementsByTagName(QStringLiteral("circle")).size());
		}
		void el1918DescriptionHasTheEightInputAndFourDiagLeds()
		{
				// manual of the EL1918: Input1/Input2 ... Input7/Input8 on
				// four rows (odd on the left), then Diag1/Diag2 and
				// Diag3/Diag4 on two rows
			const auto desc = load(appearance(
				QStringLiteral("el1918"), QStringLiteral("_description")))
				.documentElement();
			QMap<QString, QPointF> legends;
			const auto texts = desc.elementsByTagName(QStringLiteral("text"));
			for (int i = 0; i < texts.size(); ++i) {
				const auto t = texts.at(i).toElement();
				legends.insert(t.attribute(QStringLiteral("text")),
							   QPointF(t.attribute(QStringLiteral("x"))
									   .toDouble(),
									   t.attribute(QStringLiteral("y"))
									   .toDouble()));
			}
			for (int n = 1; n <= 8; ++n) {
				QVERIFY(legends.contains(
					QStringLiteral("Safe input %1").arg(n)));
			}
			for (int n = 1; n <= 4; ++n) {
				QVERIFY(legends.contains(QStringLiteral("Diag %1").arg(n)));
			}
			QVERIFY(!legends.contains(QStringLiteral("Diagnostic")));
				// the odd inputs are on the left, the even ones on the right
				// of the same row
			for (int n = 1; n <= 7; n += 2) {
				const auto a = legends.value(
					QStringLiteral("Safe input %1").arg(n));
				const auto b = legends.value(
					QStringLiteral("Safe input %1").arg(n + 1));
				QVERIFY(a.x() < b.x());
				QCOMPARE(a.y(), b.y());
			}
			QVERIFY(legends.value(QStringLiteral("Diag 1")).y()
					> legends.value(QStringLiteral("Safe input 7")).y());
			QVERIFY(legends.value(QStringLiteral("Diag 3")).y()
					> legends.value(QStringLiteral("Diag 1")).y());
			int red = 0;
			const auto circles = desc.elementsByTagName(
				QStringLiteral("circle"));
			for (int i = 0; i < circles.size(); ++i) {
				if (circles.at(i).toElement().attribute(
						QStringLiteral("style")).contains(
						QLatin1String("filling:red"))) {
					++red;
				}
			}
			QCOMPARE(red, 3);
		}

		void epp2338HasASchematicAndARealisticView()
		{
			const QString key = QStringLiteral("epp2338");
			const auto symbol = load(elementPath(
				key + QStringLiteral(".elmt"))).documentElement();
			const auto real = load(appearance(
				key, QStringLiteral("_realistic"))).documentElement();
			QVERIFY(!symbol.isNull());
			QVERIFY(!real.isNull());

				// each M12 socket is one terminal (X01.4 was the channel 1
				// of X01 before): 4 sockets + 2 EtherCAT P sockets
			QMap<QString, QPointF> pins;
			const auto terminals = real.firstChildElement(
				QStringLiteral("description")).elementsByTagName(
				QStringLiteral("terminal"));
			for (int i = 0; i < terminals.size(); ++i) {
				const auto t = terminals.at(i).toElement();
				pins.insert(t.attribute(QStringLiteral("name")),
							QPointF(t.attribute(QStringLiteral("x"))
									.toDouble(),
									t.attribute(QStringLiteral("y"))
									.toDouble()));
			}
			QStringList expected{QStringLiteral("IN X50"),
								 QStringLiteral("OUT X51")};
			for (int s = 1; s <= 4; ++s) {
				expected << QStringLiteral("X0%1").arg(s);
			}
			QCOMPARE(QStringList(pins.keys()).size(), expected.size());
			for (const QString &name : expected) {
				QVERIFY2(pins.contains(name), qPrintable(name));
			}
			QCOMPARE(terminalNames(symbol).size(), expected.size());
			for (const QString &name : expected) {
				QVERIFY2(terminalNames(symbol).contains(name),
						 qPrintable(name));
			}

				// each pin of the realistic view is the centre of a circle,
				// on the 5 px grid
			const auto circles = real.firstChildElement(
				QStringLiteral("description")).elementsByTagName(
				QStringLiteral("circle"));
			for (auto it = pins.cbegin(); it != pins.cend(); ++it) {
				bool centre = false;
				for (int i = 0; i < circles.size(); ++i) {
					const auto c = circles.at(i).toElement();
					const qreal d = c.attribute(
						QStringLiteral("diameter")).toDouble();
					centre |= QLineF(
						QPointF(c.attribute(QStringLiteral("x")).toDouble()
								+ d / 2,
								c.attribute(QStringLiteral("y")).toDouble()
								+ d / 2), it.value()).length() < 0.05;
				}
				QVERIFY2(centre, qPrintable(it.key()));
				QVERIFY2(qFuzzyIsNull(std::fmod(it.value().x(), 5))
						 && qFuzzyIsNull(std::fmod(it.value().y(), 5)),
						 qPrintable(it.key()));
			}
				// two channels share the terminal of their M12 socket:
				// X01(1) (pin 4) and X01(2) (pin 2) are on the terminal X01
			const auto ios = real.elementsByTagName(
				QStringLiteral("plcIO"));
			QVERIFY(ios.size() == 8);
			QMap<QString, QStringList> byPin;
			for (int i = 0; i < ios.size(); ++i) {
				const auto io = ios.at(i).toElement();
				byPin[io.firstChildElement(QStringLiteral("terminal"))
					  .text()] << io.attribute(QStringLiteral("address"));
			}
			QCOMPARE(byPin.size(), 4);
			QCOMPARE(byPin.value(QStringLiteral("X01")),
					 (QStringList{QStringLiteral("X01(1)"),
								  QStringLiteral("X01(2)")}));
			QCOMPARE(byPin.value(QStringLiteral("X04")),
					 (QStringList{QStringLiteral("X04(1)"),
								  QStringLiteral("X04(2)")}));
			QCOMPARE(ioAddresses(real), ioAddresses(symbol));

				// the conductors leave the M8 sockets by the top and the
				// M12 sockets by the right, in every view
			for (const QString &suffix : {QString(),
					QStringLiteral("_realistic"),
					QStringLiteral("_description")}) {
				const auto view = suffix.isEmpty() ? symbol
					: load(appearance(key, suffix)).documentElement();
				const auto terms = view.firstChildElement(
					QStringLiteral("description")).elementsByTagName(
					QStringLiteral("terminal"));
				QCOMPARE(terms.size(), 6);
				for (int i = 0; i < terms.size(); ++i) {
					const auto t = terms.at(i).toElement();
					const QString name = t.attribute(QStringLiteral("name"));
					QCOMPARE(t.attribute(QStringLiteral("orientation")),
							 name.startsWith(QLatin1Char('X'))
							 ? QStringLiteral("e") : QStringLiteral("n"));
				}
			}

				// the reference is inside the box, under the sockets
			const auto texts = real.firstChildElement(
				QStringLiteral("description")).elementsByTagName(
				QStringLiteral("text"));
			bool seen = false;
			for (int i = 0; i < texts.size(); ++i) {
				const auto t = texts.at(i).toElement();
				if (t.attribute(QStringLiteral("text"))
						== QLatin1String("EPP2338-0002")) {
					seen = true;
					QVERIFY(t.attribute(QStringLiteral("y")).toDouble()
							> pins.value(QStringLiteral("OUT X51")).y()
							+ 15);
				}
			}
			QVERIFY(seen);

				// the schematic is dotted: outline of the module
			QVERIFY(symbol.firstChildElement(QStringLiteral("description"))
					.elementsByTagName(QStringLiteral("rect")).size() > 4);
			bool dotted = false;
			const auto rects = symbol.firstChildElement(
				QStringLiteral("description")).elementsByTagName(
				QStringLiteral("rect"));
			for (int i = 0; i < rects.size(); ++i) {
				dotted |= rects.at(i).toElement()
					.attribute(QStringLiteral("style"))
					.contains(QLatin1String("line-style:dotted"));
			}
			QVERIFY(dotted);
		}

		void el9410PowerFeedHasEightTerminalsAndNoChannel()
		{
			const QString key = QStringLiteral("el9410");
			const auto symbol = load(elementPath(
				key + QStringLiteral(".elmt"))).documentElement();
			const auto real = load(appearance(
				key, QStringLiteral("_realistic"))).documentElement();
			const auto desc = load(appearance(
				key, QStringLiteral("_description"))).documentElement();
			const auto table = load(appearance(
				key, QStringLiteral("_table"))).documentElement();
			for (const auto &def : {symbol, real, desc, table}) {
				QVERIFY(!def.isNull());
				QCOMPARE(info(def, QStringLiteral("manufacturer_reference")),
						 QStringLiteral("EL9410"));
			}

				// the housing of the other terminals, 2 columns of 4
			const auto ref = load(appearance(QStringLiteral("el1004"),
				QStringLiteral("_realistic"))).documentElement();
			QCOMPARE(real.attribute(QStringLiteral("height")),
					 ref.attribute(QStringLiteral("height")));
			const auto pins = pinPoints(real.firstChildElement(
				QStringLiteral("description")));
			QCOMPARE(pins.size(), 8);
			for (int n = 1; n <= 8; ++n) {
				const QString name = QString::number(n);
				QVERIFY2(pins.contains(name), qPrintable(name));
				QCOMPARE(pins.value(name).x(), n <= 4 ? -5.0 : 10.0);
			}
			QCOMPARE(terminalNames(symbol), terminalNames(real));
			QCOMPARE(terminalNames(desc), terminalNames(real));
			QCOMPARE(pinPoints(symbol.firstChildElement(
				QStringLiteral("description"))), pins);

				// the feed pins carry the potentials, the PE none
			QCOMPARE(real.elementsByTagName(QStringLiteral("plcIO")).size(),
					 0);
			QCOMPARE(symbol.elementsByTagName(QStringLiteral("plcIO")).size(),
					 0);
			int red = 0, blue = 0;
			const auto terminals = real.elementsByTagName(
				QStringLiteral("terminal"));
			for (int i = 0; i < terminals.size(); ++i) {
				const QString pot = terminals.at(i).toElement().attribute(
					QStringLiteral("potential"));
				red += pot == QLatin1String("24V");
				blue += pot == QLatin1String("0V");
			}
			QCOMPARE(red, 3);
			QCOMPARE(blue, 3);

				// the table: pin | label | (empty) TwinCAT ID, 8 rows
			const auto ios = table.elementsByTagName(QStringLiteral("plcIO"));
			QCOMPARE(ios.size(), 8);
			for (int i = 0; i < ios.size(); ++i) {
				const auto io = ios.at(i).toElement();
				QVERIFY(io.attribute(QStringLiteral("address")).isEmpty());
				QVERIFY(!io.attribute(QStringLiteral("functionText"))
						.isEmpty());
			}
			QVERIFY(load(appearance(key, QStringLiteral("_table")))
					.toString().contains(QStringLiteral(
						"<columnOrder>5,2,1</columnOrder>")));

				// the description names the LEDs and the legends
			QStringList legends;
			const auto texts = desc.elementsByTagName(QStringLiteral("text"));
			for (int i = 0; i < texts.size(); ++i) {
				legends << texts.at(i).toElement()
					.attribute(QStringLiteral("text"));
			}
			for (const QString &text : {QStringLiteral("Power (E-bus)"),
										QStringLiteral("Diag. (E-bus)"),
										QStringLiteral("Run"),
										QStringLiteral("2: Infeed 24 V"),
										QStringLiteral("3: Infeed 0 V"),
										QStringLiteral("4: PE")}) {
				QVERIFY2(legends.contains(text), qPrintable(text));
			}
		}
		void ek1100AndCu1128HaveFourViewsAndTheirSockets()
		{
			const struct {
				QString key;
				QString reference;
				QStringList pins;
				int ios;
			} hubs[] = {
				{QStringLiteral("ek1100"), QStringLiteral("EK1100"),
				 {QStringLiteral("EtherCAT IN"),
				  QStringLiteral("EtherCAT OUT"), QStringLiteral("1"),
				  QStringLiteral("2"), QStringLiteral("3"),
				  QStringLiteral("4")}, 6},
				{QStringLiteral("cu1128"), QStringLiteral("CU1128"),
				 {QStringLiteral("P1"), QStringLiteral("P2"),
				  QStringLiteral("P3"), QStringLiteral("P4"),
				  QStringLiteral("P5"), QStringLiteral("P6"),
				  QStringLiteral("P7"), QStringLiteral("P8"),
				  QStringLiteral("1"), QStringLiteral("2"),
				  QStringLiteral("3")}, 11}};
			for (const auto &hub : hubs) {
				const auto symbol = load(elementPath(
					hub.key + QStringLiteral(".elmt"))).documentElement();
				const auto real = load(appearance(
					hub.key, QStringLiteral("_realistic"))).documentElement();
				const auto desc = load(appearance(
					hub.key, QStringLiteral("_description"))).documentElement();
				const auto table = load(appearance(
					hub.key, QStringLiteral("_table"))).documentElement();
				for (const auto &def : {symbol, real, desc, table}) {
					QVERIFY2(!def.isNull(), qPrintable(hub.key));
					QCOMPARE(info(def, QStringLiteral("manufacturer")),
							 QStringLiteral("Beckhoff"));
					QCOMPARE(info(def,
								  QStringLiteral("manufacturer_reference")),
							 hub.reference);
				}
				const auto pins = pinPoints(real.firstChildElement(
					QStringLiteral("description")));
				QCOMPARE(pins.size(), hub.pins.size());
				for (const QString &name : hub.pins) {
					QVERIFY2(pins.contains(name), qPrintable(name));
				}
				QCOMPARE(terminalNames(symbol), terminalNames(real));
				QCOMPARE(terminalNames(desc), terminalNames(real));
				QCOMPARE(pinPoints(symbol.firstChildElement(
					QStringLiteral("description"))), pins);

					// a coupler or a junction has no channel: empty TwinCAT ID
				const auto ios = table.elementsByTagName(
					QStringLiteral("plcIO"));
				QCOMPARE(ios.size(), hub.ios);
				for (int i = 0; i < ios.size(); ++i) {
					QVERIFY(ios.at(i).toElement().attribute(
						QStringLiteral("address")).isEmpty());
				}
			}

				// EK1100: 44 mm, as high as the other housings; CU1128:
				// 122 mm, so about 2.8 times as wide
			const auto ek = load(appearance(QStringLiteral("ek1100"),
				QStringLiteral("_realistic"))).documentElement();
			const auto cu = load(appearance(QStringLiteral("cu1128"),
				QStringLiteral("_realistic"))).documentElement();
			const auto el = load(appearance(QStringLiteral("el1004"),
				QStringLiteral("_realistic"))).documentElement();
			QCOMPARE(ek.attribute(QStringLiteral("height")),
					 el.attribute(QStringLiteral("height")));
			QCOMPARE(cu.attribute(QStringLiteral("height")),
					 el.attribute(QStringLiteral("height")));
			QVERIFY(cu.attribute(QStringLiteral("width")).toInt()
					> 2.5 * ek.attribute(QStringLiteral("width")).toInt());

				// the supply of the CU1128 uses the colour code
			const auto cuTerminals = cu.elementsByTagName(
				QStringLiteral("terminal"));
			QStringList potentials;
			for (int i = 0; i < cuTerminals.size(); ++i) {
				potentials << cuTerminals.at(i).toElement().attribute(
					QStringLiteral("potential"));
			}
			QVERIFY(potentials.contains(QStringLiteral("24V")));
			QVERIFY(potentials.contains(QStringLiteral("0V")));
			QVERIFY(potentials.contains(QStringLiteral("PE")));
		}
		void everyTableHasAFilledTitleBandWithAlignedTexts()
		{
			int checked = 0;
			for (const QString &file : allFiles()) {
				const QString suffix = QStringLiteral("_table.elmt");
				if (!file.endsWith(suffix)) {
					continue;
				}
				const auto root = load(elementPath(file)).documentElement();
				const auto desc = root.firstChildElement(
					QStringLiteral("description"));
				const auto rect = desc.firstChildElement(
					QStringLiteral("rect"));
				QVERIFY2(rect.attribute(QStringLiteral("style"))
						 .contains(QStringLiteral("filling:lightgray")),
						 qPrintable(file));
				QSet<QString> infos;
				QSet<QString> ys;
				for (auto n = desc.firstChildElement(
						 QStringLiteral("dynamic_text"));
					 !n.isNull();
					 n = n.nextSiblingElement(
						 QStringLiteral("dynamic_text"))) {
					infos << n.firstChildElement(QStringLiteral("info_name"))
							.text();
					ys << n.attribute(QStringLiteral("y"));
				}
				QVERIFY2(infos.contains(QStringLiteral("label")),
						 qPrintable(file));
				QVERIFY2(infos.contains(QStringLiteral("designation")),
						 qPrintable(file));
				if (!info(root, QStringLiteral("manufacturer_reference"))
						 .isEmpty()) {
					QVERIFY2(infos.contains(
								 QStringLiteral("manufacturer_reference")),
							 qPrintable(file));
				}
				// the band lies above the table
				QVERIFY(rect.attribute(QStringLiteral("y")).toDouble() < 0);
				++checked;
			}
			QVERIFY(checked >= 40);
		}
		void supplyLabelsAreColouredByFunction()
		{
			const QRegularExpression positive(QStringLiteral(
				"^(\\+?24( ?V)?(_[A-C])?|Us|Up|DC\\+)$"));
			const QRegularExpression negative(QStringLiteral(
				"^0 ?V[1-4]?$"));
			int red = 0;
			int blue = 0;
			int orange = 0;
			for (const QString &file : allFiles()) {
				if (file.endsWith(QStringLiteral("_table.elmt"))) {
					continue;
				}
				const auto texts = load(elementPath(file)).elementsByTagName(
					QStringLiteral("text"));
				for (int i = 0; i < texts.size(); ++i) {
					const auto e = texts.at(i).toElement();
					const QString text = e.attribute(QStringLiteral("text"));
					const QString colour = e.attribute(QStringLiteral("color"));
					const QByteArray where = (file + QLatin1Char(' ') + text)
						.toUtf8();
					if (positive.match(text).hasMatch()) {
						QVERIFY2(colour == QStringLiteral("#ff0000"),
								 where.constData());
						++red;
					}
					else if (negative.match(text).hasMatch()) {
						QVERIFY2(colour == QStringLiteral("#0000ff"),
								 where.constData());
						++blue;
					}
					else if (text == QStringLiteral("PE")) {
						QVERIFY2(colour == QStringLiteral("#b87800"),
								 where.constData());
						++orange;
					}
				}
			}
			QVERIFY(red > 50);
			QVERIFY(blue > 50);
			QVERIFY(orange > 30);
		}
		void everyRealisticSupplyLabelHasItsColouredStroke()
		{
			int checked = 0;
			for (const QString &file : allFiles()) {
				if (!file.endsWith(QStringLiteral("_realistic.elmt"))) {
					continue;
				}
				const auto root = load(elementPath(file)).documentElement();
				const auto desc = root.firstChildElement(
					QStringLiteral("description"));
				int strokes = 0;
				int labels = 0;
				for (auto n = desc.firstChildElement(); !n.isNull();
					 n = n.nextSiblingElement()) {
					if (n.tagName() == QLatin1String("rect")
						&& n.attribute(QStringLiteral("height")).toDouble() < 2
						&& n.attribute(QStringLiteral("style")).contains(
							QRegularExpression(QStringLiteral(
								"filling:(red|blue|orange)")))) {
						++strokes;
					}
					const QString c = n.attribute(QStringLiteral("color"));
					if (n.tagName() == QLatin1String("text")
						&& (c == QStringLiteral("#ff0000")
							|| c == QStringLiteral("#0000ff")
							|| c == QStringLiteral("#b87800"))) {
						++labels;
					}
				}
				QVERIFY2(strokes >= labels, qPrintable(file));
				checked += labels > 0;
			}
			QVERIFY(checked >= 25);
		}
		void ek1322HasTwoM8SocketsAndFourFeedPins()
		{
			const QString key = QStringLiteral("ek1322");
			const auto symbol = load(elementPath(
				key + QStringLiteral(".elmt"))).documentElement();
			const auto real = load(appearance(
				key, QStringLiteral("_realistic"))).documentElement();
			const auto desc = load(appearance(
				key, QStringLiteral("_description"))).documentElement();
			const auto table = load(appearance(
				key, QStringLiteral("_table"))).documentElement();
			for (const auto &def : {symbol, real, desc, table}) {
				QVERIFY(!def.isNull());
				QCOMPARE(info(def, QStringLiteral("manufacturer_reference")),
						 QStringLiteral("EK1322"));
			}

				// same height as the other cards, one pin per M8 socket
			const auto ref = load(appearance(QStringLiteral("el1004"),
				QStringLiteral("_realistic"))).documentElement();
			QCOMPARE(real.attribute(QStringLiteral("height")),
					 ref.attribute(QStringLiteral("height")));
			const auto pins = pinPoints(real.firstChildElement(
				QStringLiteral("description")));
			QCOMPARE(pins.size(), 6);
			for (const QString &name : {QStringLiteral("X1"),
										QStringLiteral("X2"),
										QStringLiteral("1"),
										QStringLiteral("2"),
										QStringLiteral("3"),
										QStringLiteral("4")}) {
				QVERIFY2(pins.contains(name), qPrintable(name));
			}
			QCOMPARE(terminalNames(symbol), terminalNames(real));
			QCOMPARE(terminalNames(desc), terminalNames(real));
			QCOMPARE(pinPoints(symbol.firstChildElement(
				QStringLiteral("description"))), pins);

				// Us on pins 1-2, Up on pins 3-4: 24 V then 0 V
			QCOMPARE(real.elementsByTagName(QStringLiteral("plcIO")).size(),
					 0);
			const auto terminals = real.elementsByTagName(
				QStringLiteral("terminal"));
			for (int i = 0; i < terminals.size(); ++i) {
				const auto t = terminals.at(i).toElement();
				const QString name = t.attribute(QStringLiteral("name"));
				const QString pot = t.attribute(QStringLiteral("potential"));
				if (name == QLatin1String("1") || name == QLatin1String("3")) {
					QCOMPARE(pot, QStringLiteral("24V"));
				} else if (name == QLatin1String("2")
						   || name == QLatin1String("4")) {
					QCOMPARE(pot, QStringLiteral("0V"));
				} else {
					QVERIFY(pot.isEmpty());
				}
			}

				// the table: pin | label | (empty) TwinCAT ID, 6 rows
			const auto ios = table.elementsByTagName(QStringLiteral("plcIO"));
			QCOMPARE(ios.size(), 6);
			for (int i = 0; i < ios.size(); ++i) {
				const auto io = ios.at(i).toElement();
				QVERIFY(io.attribute(QStringLiteral("address")).isEmpty());
				QVERIFY(!io.attribute(QStringLiteral("functionText"))
						.isEmpty());
			}
			QVERIFY(load(appearance(key, QStringLiteral("_table")))
					.toString().contains(QStringLiteral(
						"<columnOrder>5,2,1</columnOrder>")));

				// the description names the LEDs and the terminals
			QStringList legends;
			const auto texts = desc.elementsByTagName(QStringLiteral("text"));
			for (int i = 0; i < texts.size(); ++i) {
				legends << texts.at(i).toElement()
					.attribute(QStringLiteral("text"));
			}
			for (const QString &text : {QStringLiteral("LED Run"),
										QStringLiteral("LED L/A X1"),
										QStringLiteral("LED L/A X2"),
										QStringLiteral("1: +24 V DC (Us)"),
										QStringLiteral("4: 0 V (Up)")}) {
				QVERIFY2(legends.contains(text), qPrintable(text));
			}
		}
};

QTEST_GUILESS_MAIN(TstBeckhoffElements)
#include "tst_beckhoffelements.moc"
