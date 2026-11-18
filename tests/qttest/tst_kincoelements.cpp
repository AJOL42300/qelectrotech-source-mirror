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
	The Kinco KS105 and HMI Green symbols shipped in extra-elements: every
	product has its four appearances (symbol, realistic, description and
	I/O table) with the same data, terminals on the 10 px grid, a bounding
	box that holds the drawing, its articles (Parts tab) and a layout
	footprint, and the example project shows all of them.
*/
#include <QDir>
#include <QDirIterator>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QtTest>
#include <cmath>

class TstKincoElements : public QObject
{
	Q_OBJECT

	private:
		static QDir categoryDir()
		{
			return QDir(QStringLiteral(QET_EXTRA_ELEMENTS_DIR)
				+ QStringLiteral(
					"/10_electric/20_manufacturers_articles/kinco"));
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

			// the exact references of the library, with their family
		static QList<QPair<QString, QString>> products()
		{
			const QList<QPair<QString, QString>> list{
				{QStringLiteral("ks105_cpu"), QStringLiteral("KS105-16DT")},
				{QStringLiteral("ks105_cpu"),
				 QStringLiteral("KS105C1-16DT")},
				{QStringLiteral("ks105_cpu"),
				 QStringLiteral("KS105C2-16DT")},
				{QStringLiteral("ks_expansion"),
				 QStringLiteral("KS121-16DX")},
				{QStringLiteral("ks_expansion"),
				 QStringLiteral("KS122-12XR")},
				{QStringLiteral("ks_expansion"),
				 QStringLiteral("KS122-14DT")},
				{QStringLiteral("ks_expansion"),
				 QStringLiteral("KS123-14DR")},
				{QStringLiteral("ks_expansion"),
				 QStringLiteral("KS133-06IV")},
				{QStringLiteral("ks_expansion"),
				 QStringLiteral("KS131-04RD")},
				{QStringLiteral("hmi_green_gl"), QStringLiteral("GL043")},
				{QStringLiteral("hmi_green_gl"), QStringLiteral("GL043E")},
				{QStringLiteral("hmi_green_gl"), QStringLiteral("GL070")},
				{QStringLiteral("hmi_green_gl"), QStringLiteral("GL070E")},
				{QStringLiteral("hmi_green_gl"), QStringLiteral("GL100")},
				{QStringLiteral("hmi_green_gl"), QStringLiteral("GL100E")},
				{QStringLiteral("hmi_green_gl"), QStringLiteral("GL104E")},
				{QStringLiteral("hmi_green_gl"), QStringLiteral("GL150E")},
				{QStringLiteral("hmi_green_gh"), QStringLiteral("GH043")},
				{QStringLiteral("hmi_green_gh"), QStringLiteral("GH043E")},
				{QStringLiteral("hmi_green_gh"), QStringLiteral("GH070")},
				{QStringLiteral("hmi_green_gh"), QStringLiteral("GH070E")},
				{QStringLiteral("hmi_green_gh"), QStringLiteral("GH104E")},
				{QStringLiteral("hmi_green_gh"), QStringLiteral("GH150E")}};
			return list;
		}

		static QString stem(const QString &reference)
		{
			return reference.toLower().replace(QLatin1Char('-'),
											   QLatin1Char('_'));
		}

			// "" (symbol), _realistic, _description or _table
		static QString cardPath(const QPair<QString, QString> &product,
								const QString &suffix)
		{
			const QString id = stem(product.second);
			return categoryDir().filePath(
				product.first + QLatin1Char('/') + id + QLatin1Char('/')
				+ id + suffix + QStringLiteral(".elmt"));
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

		static QList<QDomElement> children(const QDomElement &root,
										   const QString &tag)
		{
			QList<QDomElement> list;
			const auto nodes = root.elementsByTagName(tag);
			for (int i = 0; i < nodes.size(); ++i) {
				list << nodes.at(i).toElement();
			}
			return list;
		}

	private slots:
		void thereAreAllTheProducts()
		{
			QCOMPARE(products().size(), 23);
			for (const auto &product : products()) {
				for (const QString &suffix : {QString(),
						QStringLiteral("_realistic"),
						QStringLiteral("_description"),
						QStringLiteral("_table")}) {
					QVERIFY2(QFileInfo::exists(cardPath(product, suffix)),
							 qPrintable(cardPath(product, suffix)));
				}
			}
				// 4 views of 23 products, 9 PLC footprints, 2 per HMI
			QCOMPARE(allFiles().size(), 23 * 4 + 9 + 14 * 2);
		}

		void directoriesAreTranslated()
		{
			QStringList dirs{categoryDir().path()};
			QDirIterator it(categoryDir().path(), QDir::Dirs
							| QDir::NoDotAndDotDot,
							QDirIterator::Subdirectories);
			while (it.hasNext()) {
				dirs << it.next();
			}
			QVERIFY(dirs.size() > 20);
			for (const QString &dir : dirs) {
				const auto doc = load(dir + QStringLiteral("/qet_directory"));
				QVERIFY2(doc.documentElement().tagName()
						 == QLatin1String("qet-directory"),
						 qPrintable(dir));
				QSet<QString> langs;
				for (const auto &n : children(doc.documentElement(),
											  QStringLiteral("name"))) {
					langs << n.attribute(QStringLiteral("lang"));
					QVERIFY2(!n.text().isEmpty(), qPrintable(dir));
				}
				for (const QString &lang : {QStringLiteral("en"),
						QStringLiteral("fr"), QStringLiteral("de")}) {
					QVERIFY2(langs.contains(lang),
							 qPrintable(dir + QLatin1Char(' ') + lang));
				}
			}
		}

		void elementsAreWellFormed()
		{
			QSet<QString> uuids;
			for (const QString &file : allFiles()) {
				QVERIFY2(!QFileInfo(file).fileName()
						 .contains(QLatin1Char(' ')), qPrintable(file));
				const auto def = load(file).documentElement();
				QVERIFY2(def.tagName() == QLatin1String("definition"),
						 qPrintable(file));
				const QString uuid = def.firstChildElement(
					QStringLiteral("uuid")).attribute(QStringLiteral("uuid"));
				QVERIFY2(!uuid.isEmpty() && !uuids.contains(uuid),
						 qPrintable(file));
				uuids.insert(uuid);
				QCOMPARE(info(def, QStringLiteral("manufacturer")),
						 QStringLiteral("Kinco"));
				QVERIFY2(!info(def, QStringLiteral("description"))
						 .isEmpty(), qPrintable(file));

				QSet<QString> names, term_uuids;
				for (const auto &t : children(
						def.firstChildElement(QStringLiteral("description")),
						QStringLiteral("terminal"))) {
					const QString name = t.attribute(QStringLiteral("name"));
					const QString id = t.attribute(QStringLiteral("uuid"));
					const QString where = file + QLatin1Char(' ') + name;
					QVERIFY2(!name.isEmpty() && !names.contains(name),
							 qPrintable(where));
					QVERIFY2(!id.isEmpty() && !term_uuids.contains(id),
							 qPrintable(where));
					QVERIFY2((QStringList{QStringLiteral("n"),
										  QStringLiteral("e"),
										  QStringLiteral("s"),
										  QStringLiteral("w")})
								 .contains(t.attribute(
									 QStringLiteral("orientation"))),
							 qPrintable(where));
					names << name;
					term_uuids << id;
				}
			}
		}

		void referencesAreExactAndSharedByTheFourViews()
		{
			const QStringList shared{QStringLiteral("manufacturer"),
									 QStringLiteral("manufacturer_reference"),
									 QStringLiteral("designation")};
			for (const auto &product : products()) {
				const auto symbol = load(cardPath(product, QString()))
					.documentElement();
				QCOMPARE(info(symbol,
							  QStringLiteral("manufacturer_reference")),
						 product.second);
				QVERIFY(!info(symbol, QStringLiteral("designation"))
						.isEmpty());
				for (const QString &suffix : {QStringLiteral("_realistic"),
						QStringLiteral("_description"),
						QStringLiteral("_table")}) {
					const auto def = load(cardPath(product, suffix))
						.documentElement();
					const QString where = product.second + suffix;
					QCOMPARE(def.attribute(QStringLiteral("link_type")),
							 symbol.attribute(QStringLiteral("link_type")));
					for (const QString &name : shared) {
						QVERIFY2(info(def, name) == info(symbol, name),
								 qPrintable(where + QLatin1Char(' ')
											+ name));
					}
					QCOMPARE(ioAddresses(def), ioAddresses(symbol));
				}
			}
		}

		void channelsMatchTheTerminals()
		{
			const QStringList types{
				QStringLiteral("entree_digitale"),
				QStringLiteral("sortie_digitale"),
				QStringLiteral("entree_analogique"),
				QStringLiteral("sortie_analogique"),
				QStringLiteral("entree_universelle")};
			for (const auto &product : products()) {
				const bool hmi = product.first.startsWith(
					QLatin1String("hmi_"));
				for (const QString &suffix : {QString(),
						QStringLiteral("_realistic"),
						QStringLiteral("_description")}) {
					const auto def = load(cardPath(product, suffix))
						.documentElement();
					QSet<QString> names;
					for (const auto &t : children(def,
							QStringLiteral("terminal"))) {
						if (!t.attribute(QStringLiteral("uuid")).isEmpty()) {
							names << t.attribute(QStringLiteral("name"));
						}
					}
					const auto ios = children(def, QStringLiteral("plcIO"));
					QVERIFY2(hmi || !ios.isEmpty(), qPrintable(
						product.second + suffix));
					for (const auto &io : ios) {
						QVERIFY(types.contains(io.attribute(
							QStringLiteral("type"))));
						const auto terms = children(io,
							QStringLiteral("terminal"));
						QVERIFY(!terms.isEmpty());
						for (const auto &t : terms) {
							QVERIFY2(names.contains(t.text()), qPrintable(
								product.second + suffix + QLatin1Char(' ')
								+ t.text()));
						}
					}
				}
			}
		}

		void terminalsAreOnTheGridOfTenAndInsideTheBoundingBox()
		{
			for (const QString &file : allFiles()) {
				const auto def = load(file).documentElement();
				const bool table = file.endsWith(
					QLatin1String("_table.elmt"));
				const qreal hx = def.attribute(
					QStringLiteral("hotspot_x")).toDouble();
				const qreal hy = def.attribute(
					QStringLiteral("hotspot_y")).toDouble();
				const qreal w = def.attribute(
					QStringLiteral("width")).toDouble();
				const qreal h = def.attribute(
					QStringLiteral("height")).toDouble();
				if (!table) {
						// the sizes are multiples of 10 for the symbols
						// and the realistic views, not for a footprint
					const bool footprint = def.firstChildElement(
						QStringLiteral("kindInformations")).isNull();
					if (!footprint) {
						QVERIFY2(std::fmod(w, 10) == 0
								 && std::fmod(h, 10) == 0,
								 qPrintable(file));
					}
				}
				const auto parts = def.firstChildElement(
					QStringLiteral("description"));
				const auto inside = [&](qreal x, qreal y) {
					return x >= -hx - 0.01 && x <= w - hx + 0.01
						&& y >= -hy - 0.01 && y <= h - hy + 0.01;
				};
				const auto num = [](const QDomElement &e, const char *a) {
					return e.attribute(QString::fromLatin1(a)).toDouble();
				};
				for (const auto &t : children(parts,
						QStringLiteral("terminal"))) {
					const qreal x = num(t, "x");
					const qreal y = num(t, "y");
					const QString where = file + QLatin1Char(' ')
						+ t.attribute(QStringLiteral("name"));
					QVERIFY2(inside(x, y), qPrintable(where));
					QVERIFY2(std::fmod(x, 10) == 0 && std::fmod(y, 10) == 0,
							 qPrintable(where));
				}
				if (table) {
					continue;
				}
				for (const auto &r : children(parts,
						QStringLiteral("rect"))) {
					const qreal x = num(r, "x");
					const qreal y = num(r, "y");
					QVERIFY2(inside(x, y) && inside(x + num(r, "width"),
							 y + num(r, "height")), qPrintable(file));
				}
				for (const auto &l : children(parts,
						QStringLiteral("line"))) {
					QVERIFY2(inside(num(l, "x1"), num(l, "y1"))
							 && inside(num(l, "x2"), num(l, "y2")),
							 qPrintable(file));
				}
				for (const auto &c : children(parts,
						QStringLiteral("circle"))) {
					const qreal d = num(c, "diameter");
					QVERIFY2(inside(num(c, "x"), num(c, "y"))
							 && inside(num(c, "x") + d, num(c, "y") + d),
							 qPrintable(file));
				}
			}
		}

		void tableViewHasNoTerminalAndShowsThreeColumns()
		{
			for (const auto &product : products()) {
				const auto def = load(cardPath(product,
					QStringLiteral("_table"))).documentElement();
				QCOMPARE(children(def.firstChildElement(
							 QStringLiteral("description")),
						 QStringLiteral("terminal")).size(), 0);
				QCOMPARE(children(def, QStringLiteral("plc_table")).size(),
						 1);
				QCOMPARE(children(def,
					QStringLiteral("columnOrder")).first().text(),
					QStringLiteral("5,2,1"));
				QSet<QString> shown;
				for (const auto &c : children(def,
						QStringLiteral("column"))) {
					if (c.attribute(QStringLiteral("visible"))
							== QLatin1String("true")) {
						shown << c.attribute(QStringLiteral("index"));
					}
				}
				QCOMPARE(shown, (QSet<QString>{QStringLiteral("1"),
											   QStringLiteral("2"),
											   QStringLiteral("5")}));
			}
		}

		void wiringColoursAreRedBlueAndOrange()
		{
			for (const auto &product : products()) {
				const auto def = load(cardPath(product,
					QStringLiteral("_realistic"))).documentElement();
				const QString text = load(cardPath(product,
					QStringLiteral("_realistic"))).toString();
				QVERIFY2(text.contains(QLatin1String("color:orange")),
						 qPrintable(product.second));
				QVERIFY2(text.contains(QLatin1String("blue")),
						 qPrintable(product.second));
				if (!product.first.startsWith(QLatin1String("hmi_"))) {
					QVERIFY2(text.contains(QLatin1String("red")),
							 qPrintable(product.second));
				}
			}
		}

		void articlesAreFilledIn()
		{
			static const QRegularExpression number(
				QStringLiteral("^\\d+(\\.\\d+)?$"));
			for (const auto &product : products()) {
				const bool hmi = product.first.startsWith(
					QLatin1String("hmi_"));
				for (const QString &suffix : {QString(),
						QStringLiteral("_realistic"),
						QStringLiteral("_description"),
						QStringLiteral("_table")}) {
					const auto def = load(cardPath(product, suffix))
						.documentElement();
					const QString where = product.second + suffix;
					QCOMPARE(info(def, QStringLiteral("quantity")),
							 QStringLiteral("1"));
					QVERIFY2(!info(def, QStringLiteral("unity")).isEmpty(),
							 qPrintable(where));
					for (const QString &n : {QStringLiteral("width"),
							QStringLiteral("height"),
							QStringLiteral("depth")}) {
						QVERIFY2(number.match(info(def, n)).hasMatch(),
								 qPrintable(where + QLatin1Char(' ') + n));
					}
					int blocks = 0;
					for (int i = 1; i <= 4; ++i) {
						const QString n = QString::number(i);
						if (info(def, QStringLiteral("auxiliary") + n)
								.isEmpty()) {
							continue;
						}
						++blocks;
						for (const QString &key : {
								QStringLiteral("description_auxiliary"),
								QStringLiteral("designation_auxiliary"),
								QStringLiteral("quantity_auxiliary"),
								QStringLiteral("unity_auxiliary")}) {
							QVERIFY2(!info(def, key + n).isEmpty(),
									 qPrintable(where + QLatin1Char(' ')
												+ key + n));
						}
					}
					QVERIFY2(blocks >= (hmi ? 3 : 2), qPrintable(where));
				}
			}
				// the dimensions of the data sheets
			const auto cpu = load(cardPath(products().first(), QString()))
				.documentElement();
			QCOMPARE(info(cpu, QStringLiteral("width")),
					 QStringLiteral("25.4"));
			QCOMPARE(info(cpu, QStringLiteral("height")),
					 QStringLiteral("100"));
		}

		void layoutFootprintsAreDrawnToScale()
		{
			static const QRegularExpression size(QStringLiteral(
				"([0-9.]+) x ([0-9.]+) x [0-9.]+ mm"));
			int count = 0;
			for (const QString &file : allFiles()) {
				if (!file.contains(QLatin1String("/implantation/"))) {
					continue;
				}
				++count;
				const auto def = load(file).documentElement();
				QCOMPARE(def.attribute(QStringLiteral("link_type")),
						 QStringLiteral("simple"));
				QCOMPARE(children(def, QStringLiteral("terminal")).size(),
						 0);
				bool label = false;
				for (const auto &t : children(def,
						QStringLiteral("dynamic_text"))) {
					label |= t.firstChildElement(QStringLiteral("info_name"))
						.text() == QLatin1String("label");
				}
				QVERIFY2(label, qPrintable(file));
				if (file.endsWith(QLatin1String("_fp_cutout.elmt"))) {
					continue;
				}
					// 1 px is 2 mm: the first rectangle is the housing
				const auto m = size.match(info(def,
					QStringLiteral("description")));
				QVERIFY2(m.hasMatch(), qPrintable(file));
				const auto r = children(def, QStringLiteral("rect"))
					.first();
				QVERIFY2(std::abs(r.attribute(QStringLiteral("width"))
					.toDouble() * 2 - m.captured(1).toDouble()) < 1.5,
					qPrintable(file));
				QVERIFY2(std::abs(r.attribute(QStringLiteral("height"))
					.toDouble() * 2 - m.captured(2).toDouble()) < 1.5,
					qPrintable(file));
			}
			QCOMPARE(count, 9 + 14 * 2);
		}

		void exampleProjectShowsTheWholeRange()
		{
			const QString path = QStringLiteral(QET_EXAMPLES_DIR)
				+ QStringLiteral("/kinco_example.qet");
			const auto project = load(path).documentElement();
			QCOMPARE(project.tagName(), QStringLiteral("project"));

			QSet<QString> embedded;
			for (const auto &e : children(project.firstChildElement(
					QStringLiteral("collection")),
					QStringLiteral("element"))) {
				embedded << e.attribute(QStringLiteral("name"));
			}

			QSet<QString> catalogue;
			QStringList titles;
			QMap<QString, int> views;
			for (const auto &d : children(project,
					QStringLiteral("diagram"))) {
				const QString title = d.attribute(QStringLiteral("title"));
				titles << title;
				for (const auto &e : children(d.firstChildElement(
						QStringLiteral("elements")),
						QStringLiteral("element"))) {
					const QString file = e.attribute(QStringLiteral("type"))
						.section(QLatin1Char('/'), -1);
					QVERIFY2(embedded.contains(file), qPrintable(file));
					if (title.startsWith(QLatin1String("Catalogue"))) {
						catalogue << file;
					} else {
						++views[file];
					}
				}
			}
			QVERIFY(titles.size() >= 12);
			QVERIFY(titles.first().contains(QStringLiteral("KS105")));
			QVERIFY(titles.filter(QStringLiteral("IHM Green")).size() >= 3);
			QVERIFY(titles.filter(QStringLiteral("Catalogue")).size() >= 5);
			for (const auto &product : products()) {
				const QString file = stem(product.second)
					+ QStringLiteral(".elmt");
				QVERIFY2(catalogue.contains(file),
						 qPrintable(product.second));
			}
				// the project uses a CPU, modules and a panel, each with
				// its symbol, realistic, description and table views
			for (const QString &id : {QStringLiteral("ks105c2_16dt"),
					QStringLiteral("ks121_16dx"),
					QStringLiteral("ks131_04rd"),
					QStringLiteral("gl070e")}) {
				for (const QString &suffix : {QString(),
						QStringLiteral("_realistic"),
						QStringLiteral("_description"),
						QStringLiteral("_table")}) {
					QVERIFY2(views.value(id + suffix
										 + QStringLiteral(".elmt")) >= 1,
							 qPrintable(id + suffix));
				}
			}
			QFile raw(path);
			QVERIFY(raw.open(QIODevice::ReadOnly));
			QVERIFY(!raw.readAll().contains("\r"));
		}
};

QTEST_APPLESS_MAIN(TstKincoElements)
#include "tst_kincoelements.moc"
