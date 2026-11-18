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
	The Lanbao sensors shipped in extra-elements: every product has its
	own folder with a schematic, a realistic, a description and a layout
	appearance, the first three share the same terminals, every drawing
	stays inside the frame of the element, and the example project only
	uses symbols of the library.
*/
#include <QDir>
#include <QDirIterator>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QtTest>
#include <cmath>

class TstLanbaoElements : public QObject
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

		static QString root()
		{
			return QStringLiteral(QET_EXTRA_ELEMENTS_DIR)
				+ QStringLiteral("/19_lanbao_sensors");
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

			// the product folders: path -> its .elmt files
		static QMap<QString, QStringList> products()
		{
			QMap<QString, QStringList> map;
			QDirIterator it(root(), {QStringLiteral("*.elmt")},
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

		static bool isLayout(const QString &file)
		{
			return file.endsWith(QLatin1String("_layout.elmt"));
		}

	private slots:
		void directoriesHaveAnEnglishAndFrenchName()
		{
			int count = 0;
			QDirIterator it(root(), {QStringLiteral("qet_directory")},
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
				// the collection, three categories, three products
			QCOMPARE(count, 7);
		}

		void everyProductHasFourAppearances()
		{
			const auto map = products();
			QCOMPARE(map.size(), 3);
			for (auto it = map.cbegin(); it != map.cend(); ++it) {
				const QString stem = QFileInfo(it.key()).fileName();
				QStringList expected;
				for (const QString &suffix :
					 {QString(), QStringLiteral("_realistic"),
					  QStringLiteral("_description"),
					  QStringLiteral("_layout")}) {
					expected << it.key() + QLatin1Char('/') + stem
						+ suffix + QStringLiteral(".elmt");
				}
				QStringList got = it.value();
				got.sort();
				expected.sort();
				QCOMPARE(got, expected);
			}
		}

		void elementsAreWellFormed()
		{
			QSet<QString> element_uuids;
			const auto map = products();
			for (const QStringList &files : map) {
				for (const QString &file : files) {
					const auto def = load(file);
					const QByteArray f = file.toUtf8();
					QVERIFY2(def.tagName() == QLatin1String("definition"),
							 f.constData());
					QCOMPARE(def.attribute(QStringLiteral("link_type")),
							 QStringLiteral("simple"));
					const QString uuid = def.firstChildElement(
						QStringLiteral("uuid")).attribute(
						QStringLiteral("uuid"));
					QVERIFY2(!uuid.isEmpty()
							 && !element_uuids.contains(uuid), f.constData());
					element_uuids.insert(uuid);
					QCOMPARE(info(def, QStringLiteral("manufacturer")),
							 QStringLiteral("Lanbao"));
					if (isLayout(file)) {
						QVERIFY2(parts(def, QStringLiteral("terminal"))
								 .isEmpty(), f.constData());
						continue;
					}
					QVERIFY2(!info(def, QStringLiteral(
								 "manufacturer_reference")).isEmpty(),
							 f.constData());
					QVERIFY2(!info(def, QStringLiteral(
								 "designation")).isEmpty(), f.constData());
					const auto names = def.firstChildElement(
						QStringLiteral("names")).elementsByTagName(
						QStringLiteral("name"));
					QVERIFY2(names.size() >= 2, f.constData());

					QSet<QString> term_names, term_uuids;
					const auto terms = parts(def, QStringLiteral("terminal"));
					QVERIFY2(terms.size() >= 3, f.constData());
					for (int i = 0; i < terms.size(); ++i) {
						const auto t = terms.at(i).toElement();
						const QString name =
							t.attribute(QStringLiteral("name"));
						const QString tu = t.attribute(QStringLiteral("uuid"));
						QVERIFY2(!name.isEmpty()
								 && !term_names.contains(name),
								 f.constData());
						QVERIFY2(!tu.isEmpty() && !term_uuids.contains(tu),
								 f.constData());
						QVERIFY2((QStringList{QStringLiteral("n"),
											  QStringLiteral("e"),
											  QStringLiteral("s"),
											  QStringLiteral("w")})
								 .contains(t.attribute(
									 QStringLiteral("orientation"))),
								 f.constData());
						term_names.insert(name);
						term_uuids.insert(tu);
					}
				}
			}
			QCOMPARE(element_uuids.size(), 12);
		}

		void appearancesShareTheSameTerminals()
		{
			const auto map = products();
			for (auto it = map.cbegin(); it != map.cend(); ++it) {
				const QString base = it.key() + QLatin1Char('/')
					+ QFileInfo(it.key()).fileName();
				const auto schematic = load(base + QStringLiteral(".elmt"));
				const auto ref = pins(schematic);
				QVERIFY2(ref.size() >= 3, qPrintable(base));
				for (const QString &suffix : {QStringLiteral("_realistic"),
											  QStringLiteral("_description")}) {
					const auto other = load(base + suffix
											+ QStringLiteral(".elmt"));
					QVERIFY2(pins(other) == ref, qPrintable(base + suffix));
					for (const QString &n : {QStringLiteral("manufacturer"),
							QStringLiteral("manufacturer_reference"),
							QStringLiteral("designation")}) {
						QCOMPARE(info(other, n), info(schematic, n));
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
			const auto map = products();
			for (const QStringList &files : map) {
				for (const QString &file : files) {
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
					QVERIFY2(drawn >= (isLayout(file) ? 4 : 10),
							 f.constData());
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

		void schematicsUseTheRailColours()
		{
			const auto map = products();
			for (auto it = map.cbegin(); it != map.cend(); ++it) {
				const auto def = load(it.key() + QLatin1Char('/')
					+ QFileInfo(it.key()).fileName()
					+ QStringLiteral(".elmt"));
				bool red = false, blue = false;
				const auto lines = parts(def, QStringLiteral("line"));
				for (int i = 0; i < lines.size(); ++i) {
					const QString s = lines.at(i).toElement().attribute(
						QStringLiteral("style"));
					red |= s.contains(QLatin1String("color:red"));
					blue |= s.contains(QLatin1String("color:blue"));
				}
				QVERIFY2(red && blue, qPrintable(it.key()));
			}
		}

		void layoutFootprintsFollowTheDatasheetDimensions()
		{
			QRegularExpression re(QStringLiteral(
				"^Lanbao [^,]+, ([0-9.]+)x([0-9.]+)x([0-9.]+) mm "
				"\\(width x height x depth\\)$"));
			int count = 0;
			QDirIterator it(root(), {QStringLiteral("*_layout.elmt")},
							QDir::Files, QDirIterator::Subdirectories);
			while (it.hasNext()) {
				const QString file = it.next();
				const QByteArray f = file.toUtf8();
				const auto def = load(file);
				const auto m = re.match(
					info(def, QStringLiteral("description")));
				QVERIFY2(m.hasMatch(), f.constData());
				const auto rects = parts(def, QStringLiteral("rect"));
				QVERIFY2(rects.size() >= 1, f.constData());
					// 1 px = 2 mm; the first rectangle is the housing
				const auto body = rects.at(0).toElement();
				QVERIFY2(qAbs(body.attribute(QStringLiteral("width"))
							  .toDouble() * 2 - m.captured(1).toDouble())
						 < 1.0, f.constData());
				QVERIFY2(qAbs(body.attribute(QStringLiteral("height"))
							  .toDouble() * 2 - m.captured(2).toDouble())
						 < 1.0, f.constData());
				QVERIFY2(m.captured(3).toDouble() > 10.0, f.constData());
				QVERIFY2(info(def, QStringLiteral("manufacturer_reference"))
						 .isEmpty(), f.constData());
				++count;
			}
			QCOMPARE(count, 3);
		}

		void sensorsStateTheirDetectionTypeAndRange()
		{
			const QMap<QString, QStringList> expected{
				{QStringLiteral("psm_bc100dpb_e2"),
				 {QStringLiteral("diffuse"), QStringLiteral("100 cm")}},
				{QStringLiteral("lr18xcf12dpoy_e2"),
				 {QStringLiteral("inductive"), QStringLiteral("12 mm")}},
				{QStringLiteral("pdb_cc50tgi"),
				 {QStringLiteral("laser"), QStringLiteral("80-500 mm")}}};
			const auto map = products();
			int count = 0;
			for (const QStringList &files : map) {
				for (const QString &file : files) {
					if (isLayout(file)) {
						continue;
					}
					const QByteArray f = file.toUtf8();
					const auto def = load(file);
					const QString name = QFileInfo(file).dir().dirName();
					QVERIFY2(expected.contains(name), f.constData());
					for (const QString &word : expected.value(name)) {
						QVERIFY2(info(def, QStringLiteral("comment"))
								 .contains(word, Qt::CaseInsensitive),
								 f.constData());
						QVERIFY2(info(def, QStringLiteral("description"))
								 .contains(word, Qt::CaseInsensitive),
								 f.constData());
					}
					++count;
				}
			}
			QCOMPARE(count, 9);
		}

		void exampleProjectUsesLibrarySymbols()
		{
			QFile file(QStringLiteral(QET_EXAMPLES_DIR)
					   + QStringLiteral("/lanbao_example.qet"));
			QVERIFY(file.open(QIODevice::ReadOnly));
			QDomDocument doc;
			QVERIFY(doc.setContent(&file));
			QCOMPARE(doc.documentElement().tagName(),
					 QStringLiteral("project"));
			QCOMPARE(doc.elementsByTagName(QStringLiteral("diagram")).size(),
					 2);
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
			int lanbao = 0;
			for (const QString &path : used) {
					// the Beckhoff cards are embedded under the former
					// folder, which now lives in 10_electric
				if (path.startsWith(QLatin1String("15_beckhoff_ethercat/"))) {
					continue;
				}
				QVERIFY2(QFileInfo::exists(
							 QStringLiteral(QET_EXTRA_ELEMENTS_DIR)
							 + QLatin1Char('/') + path),
						 qPrintable(path));
				lanbao += path.startsWith(QLatin1String("19_lanbao_sensors/"));
			}
				// the three sensors in four appearances
			QCOMPARE(lanbao, 12);
		}
};

QTEST_APPLESS_MAIN(TstLanbaoElements)
#include "tst_lanbaoelements.moc"
