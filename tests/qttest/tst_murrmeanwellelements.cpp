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
	The Murrelektronik MICO and Mean Well DIN-rail symbols shipped in
	extra-elements: every product has its own folder with four
	appearances (schematic, realistic, description, pin table) that share
	the same terminals, every drawing stays inside the frame of the
	element, and the example project only uses symbols of the library.
*/
#include <QDir>
#include <QDirIterator>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QSet>
#include <QtTest>
#include <cmath>

class TstMurrMeanwellElements : public QObject
{
	Q_OBJECT

	private:
		struct Pin
		{
			double x, y;
			QString orientation;
			bool operator==(const Pin &o) const
			{
				return x == o.x && y == o.y && orientation == o.orientation;
			}
		};

		static QString root(const QString &name)
		{
			return QStringLiteral(QET_EXTRA_ELEMENTS_DIR)
				+ QLatin1Char('/') + name;
		}

		static QStringList roots()
		{
			return {QStringLiteral("16_murrelektronik_mico"),
					QStringLiteral("17_meanwell_psu")};
		}

		static QDomElement load(const QString &path)
		{
			QDomDocument doc;
			QFile file(path);
			if (file.open(QIODevice::ReadOnly)) {
				doc.setContent(&file);
			}
			return doc.documentElement();
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

		static QDomNodeList parts(const QDomElement &definition,
								  const QString &tag)
		{
			return definition.firstChildElement(
				QStringLiteral("description")).elementsByTagName(tag);
		}

			// the product folders of a category: path -> its .elmt files
		static QMap<QString, QStringList> products(const QString &category)
		{
			QMap<QString, QStringList> map;
			QDirIterator it(root(category), {QStringLiteral("*.elmt")},
							QDir::Files, QDirIterator::Subdirectories);
			while (it.hasNext()) {
				const QFileInfo f(it.next());
				map[f.path()] << f.filePath();
			}
			return map;
		}

		static QMap<QString, Pin> pins(const QDomElement &definition)
		{
			QMap<QString, Pin> map;
			const auto list = parts(definition, QStringLiteral("terminal"));
			for (int i = 0; i < list.size(); ++i) {
				const auto t = list.at(i).toElement();
				map.insert(t.attribute(QStringLiteral("name")),
						   {t.attribute(QStringLiteral("x")).toDouble(),
							t.attribute(QStringLiteral("y")).toDouble(),
							t.attribute(QStringLiteral("orientation"))});
			}
			return map;
		}

		static QStringList addresses(const QDomElement &definition)
		{
			QStringList list;
			const auto ios = definition.elementsByTagName(
				QStringLiteral("plcIO"));
			for (int i = 0; i < ios.size(); ++i) {
				list << ios.at(i).toElement().attribute(
					QStringLiteral("address"))
					+ QLatin1Char('|') + ios.at(i).toElement()
					.firstChildElement(QStringLiteral("terminal")).text();
			}
			return list;
		}

	private slots:
		void directoriesHaveAnEnglishName()
		{
			int count = 0;
			for (const QString &category : roots()) {
				QDirIterator it(root(category), {QStringLiteral("qet_directory")},
								QDir::Files, QDirIterator::Subdirectories);
				while (it.hasNext()) {
					const auto doc = load(it.next());
					QCOMPARE(doc.tagName(), QStringLiteral("qet-directory"));
					const auto names =
						doc.elementsByTagName(QStringLiteral("name"));
					QStringList langs;
					for (int i = 0; i < names.size(); ++i) {
						QVERIFY(!names.at(i).toElement().text().isEmpty());
						langs << names.at(i).toElement().attribute(
							QStringLiteral("lang"));
					}
					QVERIFY2(langs.contains(QStringLiteral("en"))
							 && langs.contains(QStringLiteral("fr")),
							 qPrintable(it.filePath()));
					++count;
				}
			}
			QVERIFY(count >= 30);
		}

		void everyProductHasFourAppearances()
		{
			int count = 0;
			for (const QString &category : roots()) {
				const auto map = products(category);
				for (auto it = map.cbegin(); it != map.cend(); ++it) {
					const QString stem = QFileInfo(it.key()).fileName();
					QVERIFY2(QFileInfo::exists(
								 it.key() + QStringLiteral("/qet_directory")),
							 qPrintable(it.key()));
					QStringList expected;
					for (const QString &suffix :
						 {QString(), QStringLiteral("_realistic"),
						  QStringLiteral("_description"),
						  QStringLiteral("_table")}) {
						expected << it.key() + QLatin1Char('/') + stem
							+ suffix + QStringLiteral(".elmt");
					}
					QStringList got = it.value();
					got.sort();
					expected.sort();
					QCOMPARE(got, expected);
					++count;
				}
			}
			QVERIFY(count >= 25);
		}

		void elementsAreWellFormed()
		{
			QSet<QString> element_uuids;
			for (const QString &category : roots()) {
				const QString maker = category.startsWith(QLatin1String("16"))
					? QStringLiteral("Murrelektronik")
					: QStringLiteral("Mean Well");
				const auto map = products(category);
				for (const QStringList &files : map) {
					for (const QString &file : files) {
						const auto def = load(file);
						const QByteArray f = file.toUtf8();
						QVERIFY2(def.tagName() == QLatin1String("definition"),
								 f.constData());
						QCOMPARE(def.attribute(QStringLiteral("link_type")),
								 QStringLiteral("master"));
						const QString uuid = def.firstChildElement(
							QStringLiteral("uuid")).attribute(
							QStringLiteral("uuid"));
						QVERIFY2(!uuid.isEmpty()
								 && !element_uuids.contains(uuid),
								 f.constData());
						element_uuids.insert(uuid);
						QCOMPARE(info(def, QStringLiteral("manufacturer")),
								 maker);
						QVERIFY2(!info(def, QStringLiteral(
									 "manufacturer_reference")).isEmpty(),
								 f.constData());
						QVERIFY2(!info(def, QStringLiteral(
									 "designation")).isEmpty(),
								 f.constData());
						const auto names = def.firstChildElement(
							QStringLiteral("names")).elementsByTagName(
							QStringLiteral("name"));
						QVERIFY2(names.size() >= 2, f.constData());

						QSet<QString> term_names, term_uuids;
						const auto terms = parts(def,
												 QStringLiteral("terminal"));
						for (int i = 0; i < terms.size(); ++i) {
							const auto t = terms.at(i).toElement();
							const QString name =
								t.attribute(QStringLiteral("name"));
							const QString tu =
								t.attribute(QStringLiteral("uuid"));
							QVERIFY2(!name.isEmpty()
									 && !term_names.contains(name),
									 f.constData());
							QVERIFY2(!tu.isEmpty() && !term_uuids.contains(tu),
									 f.constData());
							QVERIFY2((QStringList{
										  QStringLiteral("n"),
										  QStringLiteral("e"),
										  QStringLiteral("s"),
										  QStringLiteral("w")})
									 .contains(t.attribute(QStringLiteral(
										 "orientation"))), f.constData());
							term_names.insert(name);
							term_uuids.insert(tu);
						}
					}
				}
			}
			QVERIFY(element_uuids.size() >= 100);
		}

		void appearancesShareTheSameTerminals()
		{
			for (const QString &category : roots()) {
				const auto map = products(category);
				for (auto it = map.cbegin(); it != map.cend(); ++it) {
					const QString base = it.key() + QLatin1Char('/')
						+ QFileInfo(it.key()).fileName();
					const auto schematic =
						load(base + QStringLiteral(".elmt"));
					const auto ref = pins(schematic);
					QVERIFY2(ref.size() >= 4, qPrintable(base));
					for (const QString &suffix :
						 {QStringLiteral("_realistic"),
						  QStringLiteral("_description")}) {
						QVERIFY2(pins(load(base + suffix
									  + QStringLiteral(".elmt"))) == ref,
								 qPrintable(base + suffix));
					}
					const auto table = load(base + QStringLiteral("_table.elmt"));
					QVERIFY2(pins(table).isEmpty(), qPrintable(base));
					QCOMPARE(addresses(table), addresses(schematic));
					QCOMPARE(addresses(table).size(), ref.size());
					for (const QString &suffix :
						 {QStringLiteral("_realistic"),
						  QStringLiteral("_description")}) {
						for (const QString &n : {QStringLiteral("manufacturer"),
								QStringLiteral("manufacturer_reference"),
								QStringLiteral("designation")}) {
							QCOMPARE(info(load(base + suffix
										  + QStringLiteral(".elmt")), n),
									 info(schematic, n));
						}
					}
				}
			}
		}

		void tableAppearanceShowsPinLabelAndId()
		{
			for (const QString &category : roots()) {
				const auto map = products(category);
				for (const QStringList &files : map) {
					for (const QString &file : files) {
						const auto def = load(file);
						const bool table =
							file.endsWith(QLatin1String("_table.elmt"));
						QCOMPARE(parts(def, QStringLiteral("plc_table"))
								 .size(), table ? 1 : 0);
						const auto cols = def.elementsByTagName(
							QStringLiteral("columnVisibility")).at(0)
							.toElement().elementsByTagName(
							QStringLiteral("column"));
						QCOMPARE(cols.size(), 6);
						for (int i = 0; i < cols.size(); ++i) {
							const auto c = cols.at(i).toElement();
							const int index =
								c.attribute(QStringLiteral("index")).toInt();
							const bool shown = table
								&& (index == 1 || index == 2 || index == 5);
							QCOMPARE(c.attribute(QStringLiteral("visible")),
									 shown ? QStringLiteral("true")
										   : QStringLiteral("false"));
						}
					}
				}
			}
		}

		void drawingsStayInTheFrameAndTerminalsOnTheGrid()
		{
			const QStringList tags{QStringLiteral("line"),
								   QStringLiteral("rect"),
								   QStringLiteral("circle"),
								   QStringLiteral("polygon"),
								   QStringLiteral("text")};
			for (const QString &category : roots()) {
				const auto map = products(category);
				for (const QStringList &files : map) {
					for (const QString &file : files) {
						if (file.endsWith(QLatin1String("_table.elmt"))) {
							continue;
						}
						const QByteArray f = file.toUtf8();
						const auto def = load(file);
						const double hx = def.attribute(
							QStringLiteral("hotspot_x")).toDouble();
						const double hy = def.attribute(
							QStringLiteral("hotspot_y")).toDouble();
						const double w = def.attribute(
							QStringLiteral("width")).toDouble();
						const double h = def.attribute(
							QStringLiteral("height")).toDouble();
						QCOMPARE(std::fmod(hx, 10.0), 0.0);
						QCOMPARE(std::fmod(hy, 10.0), 0.0);
						QCOMPARE(std::fmod(w, 10.0), 0.0);
						QCOMPARE(std::fmod(h, 10.0), 0.0);
						int drawn = 0;
						auto check = [&](double x, double y) {
							QVERIFY2(x >= -hx && x <= w - hx
									 && y >= -hy && y <= h - hy, f.constData());
							++drawn;
						};
						for (const QString &tag : tags) {
							const auto list = parts(def, tag);
							for (int i = 0; i < list.size(); ++i) {
								const auto e = list.at(i).toElement();
								auto v = [&e](const char *a) {
									return e.attribute(QString::fromLatin1(a))
										.toDouble();
								};
								if (tag == QLatin1String("line")) {
									check(v("x1"), v("y1"));
									check(v("x2"), v("y2"));
								} else if (tag == QLatin1String("rect")) {
									check(v("x"), v("y"));
									check(v("x") + v("width"),
										  v("y") + v("height"));
								} else if (tag == QLatin1String("circle")) {
									check(v("x"), v("y"));
									check(v("x") + v("diameter"),
										  v("y") + v("diameter"));
								} else if (tag == QLatin1String("polygon")) {
									for (int n = 1;
										 e.hasAttribute(QStringLiteral("x%1")
														.arg(n)); ++n) {
										check(v(qPrintable(QStringLiteral("x%1")
												  .arg(n))),
											  v(qPrintable(QStringLiteral("y%1")
												  .arg(n))));
									}
								} else {
									check(v("x"), v("y"));
								}
							}
						}
						QVERIFY2(drawn >= 10, f.constData());
						for (const Pin &p : pins(def)) {
							QCOMPARE(std::fmod(p.x, 10.0), 0.0);
							QCOMPARE(std::fmod(p.y, 10.0), 0.0);
							QVERIFY2(p.x >= -hx && p.x <= w - hx
									 && p.y >= -hy && p.y <= h - hy,
									 f.constData());
						}
					}
				}
			}
		}

		void schematicsUseTheRailColours()
		{
			for (const QString &category : roots()) {
				const auto map = products(category);
				for (auto it = map.cbegin(); it != map.cend(); ++it) {
					const auto def = load(it.key() + QLatin1Char('/')
						+ QFileInfo(it.key()).fileName()
						+ QStringLiteral(".elmt"));
					bool red = false, blue = false, dotted = false;
					const auto lines = parts(def, QStringLiteral("line"));
					for (int i = 0; i < lines.size(); ++i) {
						const QString s = lines.at(i).toElement().attribute(
							QStringLiteral("style"));
						red |= s.contains(QLatin1String("color:red"));
						blue |= s.contains(QLatin1String("color:blue"));
					}
					const auto rects = parts(def, QStringLiteral("rect"));
					for (int i = 0; i < rects.size(); ++i) {
						dotted |= rects.at(i).toElement().attribute(
							QStringLiteral("style")).contains(
							QLatin1String("line-style:dotted"));
					}
					QVERIFY2(red && blue && dotted, qPrintable(it.key()));
				}
			}
		}

		void ratingsAreInTheElementData()
		{
			const auto psus = products(QStringLiteral("17_meanwell_psu"));
			QVERIFY(psus.size() >= 18);
			for (auto it = psus.cbegin(); it != psus.cend(); ++it) {
				const QString base = it.key() + QLatin1Char('/')
					+ QFileInfo(it.key()).fileName();
				const auto def = load(base + QStringLiteral(".elmt"));
				const auto p = pins(def);
				for (const QString &n : {QStringLiteral("L"), QStringLiteral("N"),
										 QStringLiteral("+V"),
										 QStringLiteral("-V")}) {
					QVERIFY2(p.contains(n), qPrintable(base + n));
				}
				const QString d = info(def, QStringLiteral("designation"));
				QVERIFY2(d.contains(QLatin1String("24 V DC"))
						 && d.contains(QLatin1String(" A, "))
						 && d.contains(QLatin1String(" W")), qPrintable(base));
			}
			const auto micos =
				products(QStringLiteral("16_murrelektronik_mico"));
			QVERIFY(micos.size() >= 7);
			for (auto it = micos.cbegin(); it != micos.cend(); ++it) {
				const QString base = it.key() + QLatin1Char('/')
					+ QFileInfo(it.key()).fileName();
				const auto def = load(base + QStringLiteral(".elmt"));
				const auto p = pins(def);
				int channels = 0;
				for (auto t = p.cbegin(); t != p.cend(); ++t) {
					channels += t.key().startsWith(QLatin1String("OUT"));
				}
				QVERIFY2(channels >= 1, qPrintable(base));
				QVERIFY2(info(def, QStringLiteral("designation")).startsWith(
							 QStringLiteral("%1-channel").arg(channels)),
						 qPrintable(base));
				QVERIFY2(p.contains(QStringLiteral("+24V"))
						 && p.contains(QStringLiteral("0V"))
						 && p.contains(QStringLiteral("ALARM")),
						 qPrintable(base));
			}
		}

		void exampleProjectUsesLibrarySymbols()
		{
			QFile file(QStringLiteral(QET_EXAMPLES_DIR)
					   + QStringLiteral("/murr_meanwell_example.qet"));
			QVERIFY(file.open(QIODevice::ReadOnly));
			QDomDocument doc;
			QVERIFY(doc.setContent(&file));
			QCOMPARE(doc.documentElement().tagName(),
					 QStringLiteral("project"));
			QVERIFY(doc.elementsByTagName(QStringLiteral("diagram")).size()
					>= 4);
			const auto elements =
				doc.elementsByTagName(QStringLiteral("element"));
			QSet<QString> used;
			for (int i = 0; i < elements.size(); ++i) {
				const QString type = elements.at(i).toElement().attribute(
					QStringLiteral("type"));
				if (type.startsWith(QLatin1String("embed://import/"))) {
					used << type.mid(QString("embed://import/").size());
				}
			}
			QVERIFY(used.size() >= 8);
			for (const QString &path : used) {
					// generic lamp of the example, not a library element
				if (path.startsWith(QLatin1String("example_devices/"))) {
					continue;
				}
				QVERIFY2(QFileInfo::exists(QStringLiteral(QET_EXTRA_ELEMENTS_DIR)
											+ QLatin1Char('/') + path),
						 qPrintable(path));
			}
		}
};

QTEST_APPLESS_MAIN(TstMurrMeanwellElements)
#include "tst_murrmeanwellelements.moc"
