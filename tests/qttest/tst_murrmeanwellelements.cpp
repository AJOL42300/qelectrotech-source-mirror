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
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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

		struct Route
		{
			QString a;
			int ca;
			QString b;
			int cb;
		};
		using Routes = QMap<QString, QList<Route>>;

			// the folio of the M12 links: labels, families and conductors
		struct Folio
		{
			QMap<QString, QString> family;		// label -> family folder
			QMap<QString, QDomElement> element;	// label -> element
			QList<QPair<QString, QString>> links;	// "label/terminal"
		};

		static Routes routes()
		{
			Routes map;
			QFile file(root(QStringLiteral("18_murrelektronik_m12"))
					   + QStringLiteral("/function_routes.json"));
			if (!file.open(QIODevice::ReadOnly)) {
				return map;
			}
			const auto obj = QJsonDocument::fromJson(file.readAll()).object();
			for (auto it = obj.begin(); it != obj.end(); ++it) {
				const auto list = it.value().toArray();
				for (const auto &v : list) {
					const auto a = v.toArray();
					map[it.key()] << Route{a.at(0).toString(),
										   a.at(1).toInt(),
										   a.at(2).toString(),
										   a.at(3).toInt()};
				}
			}
			return map;
		}

		static QString labelOf(const QDomElement &element)
		{
			const auto list = element.firstChildElement(
				QStringLiteral("elementInformations"))
				.elementsByTagName(QStringLiteral("elementInformation"));
			for (int i = 0; i < list.size(); ++i) {
				if (list.at(i).toElement().attribute(QStringLiteral("name"))
					== QLatin1String("label")) {
					return list.at(i).toElement().text();
				}
			}
			return QString();
		}

		static Folio m12Folio(const QDomDocument &doc)
		{
			Folio folio;
			QMap<QString, QString> labels;
			const auto diagrams =
				doc.elementsByTagName(QStringLiteral("diagram"));
			for (int i = 0; i < diagrams.size(); ++i) {
				const auto d = diagrams.at(i).toElement();
				if (d.attribute(QStringLiteral("order"))
					!= QLatin1String("7")) {
					continue;
				}
				const auto elements =
					d.elementsByTagName(QStringLiteral("element"));
				for (int j = 0; j < elements.size(); ++j) {
					const auto e = elements.at(j).toElement();
					const QString label = labelOf(e);
					labels[e.attribute(QStringLiteral("uuid"))] = label;
					folio.element[label] = e;
					const QStringList path = e.attribute(
						QStringLiteral("type")).split(QLatin1Char('/'));
					folio.family[label] = path.value(path.size() - 2);
				}
				const auto conductors =
					d.elementsByTagName(QStringLiteral("conductor"));
				for (int j = 0; j < conductors.size(); ++j) {
					const auto c = conductors.at(j).toElement();
					folio.links << qMakePair(
						labels.value(c.attribute(QStringLiteral("element1")))
						+ QLatin1Char('/')
						+ c.attribute(QStringLiteral("terminalname1")),
						labels.value(c.attribute(QStringLiteral("element2")))
						+ QLatin1Char('/')
						+ c.attribute(QStringLiteral("terminalname2")));
				}
			}
			return folio;
		}

			// where a function ends: "label/terminal/channel", sorted. The
			// passive families pass the channels on, the others end a walk
		static QStringList walk(const Folio &folio, const Routes &r,
								const QString &label,
								const QString &terminal, int channel)
		{
			QStringList ends;
			QSet<QString> seen;
			QList<std::tuple<QString, QString, int>> todo{
				{label, terminal, channel}};
			while (!todo.isEmpty()) {
				const auto [l, t, ch] = todo.takeLast();
				const QString here = l + QLatin1Char('/') + t;
				if (seen.contains(here + QLatin1Char('#')
								  + QString::number(ch))) {
					continue;
				}
				seen << here + QLatin1Char('#') + QString::number(ch);
				for (const auto &link : folio.links) {
					QString other;
					if (link.first == here) {
						other = link.second;
					} else if (link.second == here) {
						other = link.first;
					} else {
						continue;
					}
					const QString l2 = other.section(QLatin1Char('/'), 0, 0);
					const QString t2 = other.section(QLatin1Char('/'), 1, 1);
					const QString family = folio.family.value(l2);
					if (!r.contains(family)) {
						ends << other + QLatin1Char('/')
							+ QString::number(ch);
						continue;
					}
					for (const Route &x : r.value(family)) {
						if (t2 == x.a && ch == x.ca) {
							todo.append({l2, x.b, x.cb});
						} else if (t2 == x.b && ch == x.cb) {
							todo.append({l2, x.a, x.ca});
						}
					}
				}
			}
			ends.sort();
			return ends;
		}

			// the function text of an output of an EPP or MICO
		static QString functionOf(const Folio &folio, const QString &label,
								  const QString &terminal, int channel)
		{
			const auto ios = folio.element.value(label).elementsByTagName(
				QStringLiteral("plcIO"));
			const QString address = terminal + QLatin1Char('(')
				+ QString::number(channel) + QLatin1Char(')');
			for (int i = 0; i < ios.size(); ++i) {
				const auto io = ios.at(i).toElement();
				const QString a = io.attribute(QStringLiteral("address"));
				if (io.firstChildElement(QStringLiteral("terminal")).text()
					== terminal
					&& (a == address || !a.endsWith(QLatin1Char(')')))) {
					return io.attribute(QStringLiteral("functionText"));
				}
			}
			return QString();
		}

			// what a valve coil shows: the function of the output it is
			// reached from, nothing when no output reaches it
		static QString resolved(const Folio &folio, const Routes &r,
								const QString &coil)
		{
			const QStringList ends =
				walk(folio, r, coil, QStringLiteral("X1"), 1);
			if (ends.size() != 1) {
				return QString();
			}
			const QStringList end = ends.first().split(QLatin1Char('/'));
			return functionOf(folio, end.at(0), end.at(1), end.at(2).toInt());
		}

		static QString shownOn(const Folio &folio, const QString &label)
		{
			const auto list = folio.element.value(label).firstChildElement(
				QStringLiteral("elementInformations"))
				.elementsByTagName(QStringLiteral("elementInformation"));
			for (int i = 0; i < list.size(); ++i) {
				if (list.at(i).toElement().attribute(QStringLiteral("name"))
					== QLatin1String("function")) {
					return list.at(i).toElement().text();
				}
			}
			return QString();
		}

		static void setShown(const Folio &folio, const QString &label,
							 const QString &text)
		{
			const auto list = folio.element.value(label).firstChildElement(
				QStringLiteral("elementInformations"))
				.elementsByTagName(QStringLiteral("elementInformation"));
			for (int i = 0; i < list.size(); ++i) {
				auto info = list.at(i).toElement();
				if (info.attribute(QStringLiteral("name"))
					== QLatin1String("function")) {
					while (info.hasChildNodes()) {
						info.removeChild(info.firstChild());
					}
					info.appendChild(info.ownerDocument().createTextNode(text));
				}
			}
		}

		static void setFunctionOf(const Folio &folio, const QString &label,
								  const QString &terminal, int channel,
								  const QString &text)
		{
			const auto ios = folio.element.value(label).elementsByTagName(
				QStringLiteral("plcIO"));
			const QString address = terminal + QLatin1Char('(')
				+ QString::number(channel) + QLatin1Char(')');
			for (int i = 0; i < ios.size(); ++i) {
				auto io = ios.at(i).toElement();
				const QString a = io.attribute(QStringLiteral("address"));
				if (io.firstChildElement(QStringLiteral("terminal")).text()
					== terminal
					&& (a == address || !a.endsWith(QLatin1Char(')')))) {
					io.setAttribute(QStringLiteral("functionText"), text);
				}
			}
		}

			// the rule of the two directions, for the valve and the output
			// it reaches: a text on one end only is copied to the other end;
			// two different texts are a conflict and nothing is changed
		enum class Sync { Nothing, ToValve, ToOutput, Conflict };
		static Sync synchronize(const Folio &folio, const Routes &r,
								const QString &coil)
		{
			const QStringList ends =
				walk(folio, r, coil, QStringLiteral("X1"), 1);
			if (ends.size() != 1) {
				return Sync::Nothing;
			}
			const QStringList e = ends.first().split(QLatin1Char('/'));
			const int channel = e.at(2).toInt();
			const QString out = functionOf(folio, e.at(0), e.at(1), channel);
			const QString valve = shownOn(folio, coil);
			if (!out.isEmpty() && !valve.isEmpty() && out != valve) {
				return Sync::Conflict;
			}
			if (!out.isEmpty() && valve.isEmpty()) {
				setShown(folio, coil, out);
				return Sync::ToValve;
			}
			if (out.isEmpty() && !valve.isEmpty()) {
				setFunctionOf(folio, e.at(0), e.at(1), channel, valve);
				return Sync::ToOutput;
			}
			return Sync::Nothing;
		}

	private slots:
		void m12FunctionFollowsTheChain()
		{
			QFile file(QStringLiteral(QET_EXAMPLES_DIR)
					   + QStringLiteral("/murr_meanwell_example.qet"));
			QVERIFY(file.open(QIODevice::ReadOnly));
			QDomDocument doc;
			QVERIFY(doc.setContent(&file));
			const Folio folio = m12Folio(doc);
			const Routes r = routes();
			QVERIFY(r.contains(QStringLiteral("m12_cable")));
			QVERIFY(!folio.links.isEmpty());

				// T coupler: channel 1 to the first branch, 2 to the second
			QCOMPARE(walk(folio, r, QStringLiteral("A3"),
						  QStringLiteral("X02"), 1),
					 QStringList{QStringLiteral("Y4/X1/1")});
			QCOMPARE(walk(folio, r, QStringLiteral("A3"),
						  QStringLiteral("X02"), 2),
					 QStringList{QStringLiteral("Y5/X1/1")});
				// Y distributor with two valves
			QCOMPARE(walk(folio, r, QStringLiteral("A3"),
						  QStringLiteral("X01"), 1),
					 QStringList{QStringLiteral("Y1/X1/1")});
			QCOMPARE(walk(folio, r, QStringLiteral("A3"),
						  QStringLiteral("X01"), 2),
					 QStringList{QStringLiteral("Y2/X1/1")});
				// and back, to the right channel of the right socket
			QCOMPARE(walk(folio, r, QStringLiteral("Y5"),
						  QStringLiteral("X1"), 1),
					 QStringList{QStringLiteral("A3/X02/2")});
			QCOMPARE(walk(folio, r, QStringLiteral("Y2"),
						  QStringLiteral("X1"), 1),
					 QStringList{QStringLiteral("A3/X01/2")});
				// a cable only: the output of the MICO
			QCOMPARE(walk(folio, r, QStringLiteral("Y3"),
						  QStringLiteral("X1"), 1),
					 QStringList{QStringLiteral("F1/OUT1/1")});

				// every valve shows the function of its own output
			const QMap<QString, QString> expected{
				{QStringLiteral("Y1"), QStringLiteral("EV12")},
				{QStringLiteral("Y2"), QStringLiteral("EV13")},
				{QStringLiteral("Y4"), QStringLiteral("EV14")},
				{QStringLiteral("Y5"), QStringLiteral("EV15")},
				{QStringLiteral("Y3"), QStringLiteral("EV16")}};
			for (auto it = expected.cbegin(); it != expected.cend(); ++it) {
				QCOMPARE(resolved(folio, r, it.key()), it.value());
				QCOMPARE(shownOn(folio, it.key()), it.value());
			}

				// the function changes at the source: only its valve follows
			Folio changed = m12Folio(doc);
			auto ios = changed.element.value(QStringLiteral("A3"))
				.elementsByTagName(QStringLiteral("plcIO"));
			for (int i = 0; i < ios.size(); ++i) {
				auto io = ios.at(i).toElement();
				if (io.attribute(QStringLiteral("address"))
					== QLatin1String("X01(1)")) {
					io.setAttribute(QStringLiteral("functionText"),
									QStringLiteral("EV99"));
				}
			}
			QCOMPARE(resolved(changed, r, QStringLiteral("Y1")),
					 QStringLiteral("EV99"));
			QCOMPARE(resolved(changed, r, QStringLiteral("Y2")),
					 QStringLiteral("EV13"));
				// the DOM is shared: put the original text back
			for (int i = 0; i < ios.size(); ++i) {
				auto io = ios.at(i).toElement();
				if (io.attribute(QStringLiteral("address"))
					== QLatin1String("X01(1)")) {
					io.setAttribute(QStringLiteral("functionText"),
									QStringLiteral("EV12"));
				}
			}

				// a link is missing: the chain is broken on both sides
			Folio broken = m12Folio(doc);
			for (int i = broken.links.size() - 1; i >= 0; --i) {
				if (broken.links.at(i).first.startsWith(QLatin1String("W2/"))
					&& broken.links.at(i).second.startsWith(
						QLatin1String("X2/"))) {
					broken.links.removeAt(i);
				}
			}
			QVERIFY(broken.links.size() == folio.links.size() - 1);
			QVERIFY(walk(broken, r, QStringLiteral("A3"),
						 QStringLiteral("X02"), 1).isEmpty());
			QVERIFY(walk(broken, r, QStringLiteral("Y4"),
						 QStringLiteral("X1"), 1).isEmpty());
			QVERIFY(resolved(broken, r, QStringLiteral("Y5")).isEmpty());
			QCOMPARE(resolved(broken, r, QStringLiteral("Y1")),
					 QStringLiteral("EV12"));
				// a channel the T coupler does not route stops there
			QVERIFY(walk(folio, r, QStringLiteral("A3"),
						 QStringLiteral("X02"), 3).isEmpty());
		}

		void m12FunctionIsBidirectional()
		{
			QFile file(QStringLiteral(QET_EXAMPLES_DIR)
					   + QStringLiteral("/murr_meanwell_example.qet"));
			QVERIFY(file.open(QIODevice::ReadOnly));
			QDomDocument doc;
			QVERIFY(doc.setContent(&file));
			const Folio folio = m12Folio(doc);
			const Routes r = routes();
			const QString epp = QStringLiteral("A3");

				// typed on the valve Y5 only, the generated example shows it
				// on the right channel of the right socket, and not on the
				// other channels
			QCOMPARE(functionOf(folio, epp, QStringLiteral("X02"), 2),
					 QStringLiteral("EV15"));
			QCOMPARE(functionOf(folio, epp, QStringLiteral("X02"), 1),
					 QStringLiteral("EV14"));
			QCOMPARE(functionOf(folio, epp, QStringLiteral("X01"), 2),
					 QStringLiteral("EV13"));

				// valve to output, through the Y distributor: the other
				// channel is not touched
			setFunctionOf(folio, epp, QStringLiteral("X01"), 1, QString());
			setShown(folio, QStringLiteral("Y1"), QStringLiteral("EV77"));
			QVERIFY(synchronize(folio, r, QStringLiteral("Y1"))
					== Sync::ToOutput);
			QCOMPARE(functionOf(folio, epp, QStringLiteral("X01"), 1),
					 QStringLiteral("EV77"));
			QCOMPARE(functionOf(folio, epp, QStringLiteral("X01"), 2),
					 QStringLiteral("EV13"));
			QCOMPARE(shownOn(folio, QStringLiteral("Y2")),
					 QStringLiteral("EV13"));

				// valve to output, through the T coupler
			setFunctionOf(folio, epp, QStringLiteral("X02"), 2, QString());
			setShown(folio, QStringLiteral("Y5"), QStringLiteral("EV88"));
			QVERIFY(synchronize(folio, r, QStringLiteral("Y5"))
					== Sync::ToOutput);
			QCOMPARE(functionOf(folio, epp, QStringLiteral("X02"), 2),
					 QStringLiteral("EV88"));
			QCOMPARE(functionOf(folio, epp, QStringLiteral("X02"), 1),
					 QStringLiteral("EV14"));

				// output to valve
			setShown(folio, QStringLiteral("Y4"), QString());
			QVERIFY(synchronize(folio, r, QStringLiteral("Y4"))
					== Sync::ToValve);
			QCOMPARE(shownOn(folio, QStringLiteral("Y4")),
					 QStringLiteral("EV14"));

				// different texts on both ends: a conflict, nothing changes
			setShown(folio, QStringLiteral("Y2"), QStringLiteral("EV55"));
			QVERIFY(synchronize(folio, r, QStringLiteral("Y2"))
					== Sync::Conflict);
			QCOMPARE(shownOn(folio, QStringLiteral("Y2")),
					 QStringLiteral("EV55"));
			QCOMPARE(functionOf(folio, epp, QStringLiteral("X01"), 2),
					 QStringLiteral("EV13"));

				// equal texts, or nothing on both ends: no change
			setShown(folio, QStringLiteral("Y2"), QStringLiteral("EV13"));
			QVERIFY(synchronize(folio, r, QStringLiteral("Y2"))
					== Sync::Nothing);

				// a valve that no output reaches is left alone
			Folio broken = folio;
			for (int i = broken.links.size() - 1; i >= 0; --i) {
				if (broken.links.at(i).first.startsWith(QLatin1String("W2/"))
					&& broken.links.at(i).second.startsWith(
						QLatin1String("X2/"))) {
					broken.links.removeAt(i);
				}
			}
			setShown(broken, QStringLiteral("Y5"), QStringLiteral("EV66"));
			QVERIFY(synchronize(broken, r, QStringLiteral("Y5"))
					== Sync::Nothing);
			QCOMPARE(functionOf(broken, epp, QStringLiteral("X02"), 2),
					 QStringLiteral("EV88"));
		}

		void m12RoutesUseExistingTerminals()
		{
			const Routes r = routes();
			QCOMPARE(r.size(), 3);
			for (auto it = r.cbegin(); it != r.cend(); ++it) {
				const auto files = products(
					QStringLiteral("18_murrelektronik_m12/") + it.key());
				QVERIFY2(!files.isEmpty(), qPrintable(it.key()));
				for (const QStringList &list : files) {
					const auto p = pins(load(list.first()));
					for (const Route &x : it.value()) {
						QVERIFY2(p.contains(x.a) && p.contains(x.b),
								 qPrintable(list.first()));
						QVERIFY(x.ca >= 1 && x.ca <= 2 && x.cb >= 1
								&& x.cb <= 2);
					}
				}
			}
		}

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

		void m12ElementsAreConsistent()
		{
			const QString cat = QStringLiteral("18_murrelektronik_m12");
			const QMap<QString, int> terminals{
				{QStringLiteral("m12_cable"), 2},
				{QStringLiteral("m12_y_distributor"), 3},
				{QStringLiteral("m12_connector"), 1},
				{QStringLiteral("m12_t_coupler"), 3},
				{QStringLiteral("m12_valve_coil"), 1}};
			QSet<QString> references;
			int files = 0;
			QDir dir(root(cat));
			QVERIFY(QFileInfo::exists(dir.filePath(QStringLiteral("qet_directory"))));
			for (auto f = terminals.cbegin(); f != terminals.cend(); ++f) {
				QVERIFY2(QFileInfo::exists(dir.filePath(
					f.key() + QStringLiteral("/qet_directory"))),
					qPrintable(f.key()));
			}
			QDirIterator it(root(cat), {QStringLiteral("*.elmt")},
							QDir::Files, QDirIterator::Subdirectories);
			while (it.hasNext()) {
				const QFileInfo f(it.next());
				const QString family = f.dir().dirName();
				QVERIFY2(terminals.contains(family), qPrintable(f.filePath()));
				const auto def = load(f.filePath());
				QCOMPARE(def.attribute(QStringLiteral("link_type")),
						 QStringLiteral("simple"));
				++files;
					// one terminal per M12 connector, unique names
				const auto p = pins(def);
				QCOMPARE(p.size(), terminals.value(family));
				QCOMPARE(parts(def, QStringLiteral("terminal")).size(),
						 p.size());
				const double w = def.attribute(QStringLiteral("width"))
					.toDouble();
				const double h = def.attribute(QStringLiteral("height"))
					.toDouble();
				const double hx = def.attribute(QStringLiteral("hotspot_x"))
					.toDouble();
				const double hy = def.attribute(QStringLiteral("hotspot_y"))
					.toDouble();
				for (auto t = p.cbegin(); t != p.cend(); ++t) {
					QVERIFY2(std::fmod(t->x, 10) == 0
							 && std::fmod(t->y, 10) == 0,
							 qPrintable(f.filePath()));
					QVERIFY2(t->x >= -hx && t->x <= w - hx
							 && t->y >= -hy && t->y <= h - hy,
							 qPrintable(f.filePath()));
				}
				QVERIFY2(!info(def, QStringLiteral("designation")).isEmpty(),
						 qPrintable(f.filePath()));
				const QString ref = info(
					def, QStringLiteral("manufacturer_reference"));
				if (family == QLatin1String("m12_valve_coil")) {
						// generic coil: no invented order number
					QVERIFY(ref.isEmpty());
					QVERIFY(info(def, QStringLiteral("designation"))
							.contains(QLatin1String("forme BI 18 mm")));
				} else {
					QCOMPARE(info(def, QStringLiteral("manufacturer")),
							 QStringLiteral("Murrelektronik"));
					QVERIFY2(!references.contains(ref), qPrintable(ref));
					references << ref;
				}
			}
			QCOMPARE(files, 12);
			QSet<QString> expected{
				QStringLiteral("7000-41135-0000000"),
				QStringLiteral("7000-12481-0000000"),
				QStringLiteral("7000-12601-0000000"),
				QStringLiteral("7000-42421-6260060"),
				QStringLiteral("7000-42421-6260030")};
			for (const char *code : {"6340050", "6340100", "6340200",
									 "6340300", "6340500", "6341000"}) {
				expected << QStringLiteral("7000-40021-") + QLatin1String(code);
			}
			QCOMPARE(references, expected);
		}

		void m12ExampleFolioUsesDashedLinks()
		{
			QFile file(QStringLiteral(QET_EXAMPLES_DIR)
					   + QStringLiteral("/murr_meanwell_example.qet"));
			QVERIFY(file.open(QIODevice::ReadOnly));
			QDomDocument doc;
			QVERIFY(doc.setContent(&file));
			const auto diagrams =
				doc.elementsByTagName(QStringLiteral("diagram"));
			int dashed = 0;
			for (int i = 0; i < diagrams.size(); ++i) {
				const auto conductors = diagrams.at(i).toElement()
					.elementsByTagName(QStringLiteral("conductor"));
				for (int j = 0; j < conductors.size(); ++j) {
					dashed += conductors.at(j).toElement().attribute(
						QStringLiteral("style")).contains(
						QLatin1String("line-style: dashed"));
				}
			}
			QVERIFY(dashed >= 8);
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
