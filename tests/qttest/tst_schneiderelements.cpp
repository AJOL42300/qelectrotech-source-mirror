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
	The Schneider Electric elements shipped in extra-elements: each one is
	well formed, has its own uuid, named terminals on the 10 px grid, its
	manufacturer and reference; the symbol and the front view of a product
	share their terminal names; the devices carry their compatible
	accessories as articles of quantity 0; the example uses the library.
*/
#include <QDir>
#include <QDirIterator>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QtTest>
#include <cmath>

class TstSchneiderElements : public QObject
{
	Q_OBJECT

	private:
		static QDir categoryDir()
		{
			return QDir(QStringLiteral(QET_EXTRA_ELEMENTS_DIR)
						+ QStringLiteral("/10_electric/20_manufacturers_articles/schneider"));
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

		static QStringList allFiles()
		{
			QStringList list;
			QDirIterator it(categoryDir().path(),
							{QStringLiteral("*.elmt")}, QDir::Files,
							QDirIterator::Subdirectories);
			while (it.hasNext()) {
				list << it.next();
			}
			list.sort();
			return list;
		}

			// terminal name -> position, from the .elmt
		static QMap<QString, QPointF> terminals(const QDomElement &definition)
		{
			QMap<QString, QPointF> map;
			const auto list = definition.elementsByTagName(
				QStringLiteral("terminal"));
			for (int i = 0; i < list.size(); ++i) {
				const auto e = list.at(i).toElement();
				map.insert(e.attribute(QStringLiteral("name")),
						   QPointF(e.attribute(QStringLiteral("x")).toDouble(),
								   e.attribute(QStringLiteral("y")).toDouble()));
			}
			return map;
		}

		static bool onGrid(double v)
		{
			return std::fabs(v / 10.0 - std::round(v / 10.0)) < 1e-6;
		}

	private slots:
		void directoriesAreNamed()
		{
			QDirIterator it(categoryDir().path(),
							{QStringLiteral("qet_directory")}, QDir::Files,
							QDirIterator::Subdirectories);
			int count = 0;
			while (it.hasNext()) {
				const QString path = it.next();
				const auto doc = load(path);
				QCOMPARE(doc.documentElement().tagName(),
						 QStringLiteral("qet-directory"));
				QSet<QString> langs;
				const auto names = doc.elementsByTagName(
					QStringLiteral("name"));
				for (int i = 0; i < names.size(); ++i) {
					const auto n = names.at(i).toElement();
					QVERIFY2(!n.text().isEmpty(), qPrintable(path));
					langs << n.attribute(QStringLiteral("lang"));
				}
				QVERIFY2(langs.contains(QStringLiteral("en")), qPrintable(path));
				++count;
			}
			QVERIFY(count >= 10);
		}

		void elementsAreWellFormed()
		{
			QSet<QString> uuids;
			const QStringList files = allFiles();
			QVERIFY(files.size() >= 60);
			for (const QString &file : files) {
				const auto doc = load(file);
				const auto def = doc.documentElement();
				QVERIFY2(def.tagName() == QLatin1String("definition"),
						 qPrintable(file));
				const QString uuid = def.firstChildElement(
					QStringLiteral("uuid")).attribute(QStringLiteral("uuid"));
				QVERIFY2(!uuid.isEmpty() && !uuids.contains(uuid),
						 qPrintable(file));
				uuids << uuid;
				QCOMPARE(info(def, QStringLiteral("manufacturer")),
						 QStringLiteral("Schneider Electric"));
				QVERIFY2(!info(def, QStringLiteral("manufacturer_reference"))
							 .isEmpty(), qPrintable(file));
				QVERIFY2(!info(def, QStringLiteral("designation")).isEmpty(),
						 qPrintable(file));
				QVERIFY2(!def.firstChildElement(QStringLiteral("names"))
							 .firstChildElement(QStringLiteral("name"))
							 .text().isEmpty(), qPrintable(file));
			}
		}

		void terminalsAreNamedAndOnTheGrid()
		{
			for (const QString &file : allFiles()) {
				const auto def = load(file).documentElement();
				const auto list = def.elementsByTagName(
					QStringLiteral("terminal"));
				QSet<QString> names;
				for (int i = 0; i < list.size(); ++i) {
					const auto t = list.at(i).toElement();
					const QString name = t.attribute(QStringLiteral("name"));
					QVERIFY2(!name.isEmpty() && !names.contains(name),
							 qPrintable(file));
					names << name;
					QVERIFY2(!t.attribute(QStringLiteral("uuid")).isEmpty(),
							 qPrintable(file));
					QVERIFY2(onGrid(t.attribute(QStringLiteral("x")).toDouble())
							 && onGrid(t.attribute(QStringLiteral("y")).toDouble()),
							 qPrintable(file));
				}
			}
		}

			// the front view of a device offers the same terminal names as
			// its symbol, so a conductor keeps its meaning from one to the other
		void symbolAndFrontShareTerminalNames()
		{
			int pairs = 0;
			for (const QString &file : allFiles()) {
				if (!file.endsWith(QLatin1String("_realistic.elmt"))) {
					continue;
				}
				QString symbol = file;
				symbol.chop(int(QStringLiteral("_realistic.elmt").size()));
				symbol += QStringLiteral(".elmt");
					// mechanical accessories (handle, shield) have no symbol
				if (!QFileInfo::exists(symbol)) {
					QVERIFY2(terminals(load(file).documentElement()).isEmpty(),
							 qPrintable(file));
					continue;
				}
				QCOMPARE(terminals(load(file).documentElement()).keys(),
						 terminals(load(symbol).documentElement()).keys());
				++pairs;
			}
			QVERIFY(pairs >= 25);
		}

		void polesMatchTheDesignation()
		{
			struct Case { const char *stem; int terminals; };
			const Case cases[] = {
				{"ic60n_1p/ic60n_1p", 2}, {"ic60n_2p/ic60n_2p", 4},
				{"ic60n_3p/ic60n_3p", 6}, {"ic60n_4p/ic60n_4p", 8},
				{"nsx250n_3p_tm250/nsx250n_3p_tm250", 6},
				{"ins250_4p/ins250_4p", 8},
				{"lc1d09p7/lc1d09p7", 12}, {"lc1d38p7/lc1d38p7", 12},
				{"lc1d09bd/lc1d09bd", 12},
				{"ladn22/ladn22", 8}, {"ladn11/ladn11", 4},
				{"ladn20/ladn20", 4}, {"ladn02/ladn02", 4}};
			for (const Case &c : cases) {
				QStringList found;
				for (const QString &file : allFiles()) {
					if (file.endsWith(QString::fromLatin1("/%1.elmt")
									  .arg(QLatin1String(c.stem)))) {
						found << file;
					}
				}
				QCOMPARE(found.size(), 1);
				QCOMPARE(terminals(load(found.first()).documentElement()).size(),
						 c.terminals);
			}
		}

			// terminals carry the standardised numbers, on the symbol and on
			// the front view alike
		void contactorAndBlockTerminalsAreNumbered()
		{
			struct Case { const char *stem; QStringList names; };
			const Case cases[] = {
				{"lc1d09p7/lc1d09p7",
				 {"1/L1", "2/T1", "3/L2", "4/T2", "5/L3", "6/T3", "A1", "A2",
				  "13", "14", "21", "22"}},
				{"ladn22/ladn22",
				 {"53", "54", "61", "62", "71", "72", "83", "84"}}};
			for (const Case &c : cases) {
				for (const char *suffix : {".elmt", "_realistic.elmt"}) {
					QString found;
					for (const QString &file : allFiles()) {
						if (file.endsWith(QString::fromLatin1("/%1%2")
										  .arg(QLatin1String(c.stem),
											   QLatin1String(suffix)))) {
							found = file;
						}
					}
					QVERIFY2(!found.isEmpty(), c.stem);
					QStringList names = terminals(
						load(found).documentElement()).keys();
					QStringList expected = c.names;
					names.sort();
					expected.sort();
					QCOMPARE(names, expected);
				}
			}
		}

			// every device lists its compatible accessories as articles of
			// quantity 0; the 220 A disconnector has the motor mechanism
		void devicesSeedTheirAccessories()
		{
			QString path;
			for (const QString &file : allFiles()) {
				if (file.endsWith(QLatin1String("/nsx250n_3p_tm250.elmt"))) {
					path = file;
				}
			}
			QVERIFY(!path.isEmpty());
			const auto def = load(path).documentElement();
			QStringList refs;
			for (int n = 1; n <= 4; ++n) {
				const QString ref = info(def, QStringLiteral(
					"manufacturer_reference_auxiliary%1").arg(n));
				QVERIFY(!ref.isEmpty());
				QCOMPARE(info(def, QStringLiteral("quantity_auxiliary%1")
							  .arg(n)), QStringLiteral("0"));
				refs << ref;
			}
			QVERIFY(refs.contains(QStringLiteral("LV431541")));
			QVERIFY(refs.contains(QStringLiteral("29450")));
		}

		void exampleUsesTheLibrary()
		{
			const auto doc = load(QStringLiteral(QET_EXAMPLES_DIR)
								  + QStringLiteral("/schneider_example.qet"));
			QCOMPARE(doc.documentElement().tagName(),
					 QStringLiteral("project"));
			QVERIFY(doc.elementsByTagName(QStringLiteral("diagram")).size() >= 6);
			QStringList refs;
			const auto list = doc.elementsByTagName(
				QStringLiteral("elementInformation"));
			for (int i = 0; i < list.size(); ++i) {
				const auto e = list.at(i).toElement();
				if (e.attribute(QStringLiteral("name")).startsWith(
						QLatin1String("manufacturer_reference"))) {
					refs << e.text();
				}
			}
			for (const char *ref : {"C25N3TM250C", "LV431541", "29450",
									"LV429410", "LV429517", "A9F74310",
									"31106", "LC1D09P7", "LADN22"}) {
				QVERIFY2(refs.contains(QLatin1String(ref)), ref);
			}
				// the accessories of -QS1 were given to it: quantity 1
			bool linked = false;
			for (int i = 0; i < list.size(); ++i) {
				const auto e = list.at(i).toElement();
				if (e.attribute(QStringLiteral("name"))
						== QLatin1String("quantity_auxiliary1")
						&& e.text() == QLatin1String("1")) {
					linked = true;
				}
			}
			QVERIFY(linked);
		}
};

QTEST_APPLESS_MAIN(TstSchneiderElements)
#include "tst_schneiderelements.moc"
