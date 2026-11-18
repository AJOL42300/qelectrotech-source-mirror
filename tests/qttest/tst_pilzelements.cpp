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
	The Pilz safety switches and cables shipped in extra-elements: one
	element per order number, with the terminal designations of the
	manufacturer and, for the cables, one terminal per pin and one per
	wire named after its IEC 60757 colour code.
*/
#include <QDirIterator>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QSet>
#include <QtTest>
#include <cmath>

class TstPilzElements : public QObject
{
	Q_OBJECT

	private:
		static QString root()
		{
			return QStringLiteral(QET_EXTRA_ELEMENTS_DIR)
				+ QStringLiteral("/19_pilz");
		}

		static QStringList elements()
		{
			QStringList list;
			QDirIterator it(root(), {QStringLiteral("*.elmt")},
							QDir::Files, QDirIterator::Subdirectories);
			while (it.hasNext()) {
				list << it.next();
			}
			list.sort();
			return list;
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

			// terminal names, in the order of the file
		static QStringList terminals(const QDomElement &definition,
									 const QString &side = QString())
		{
			QStringList names;
			const auto list = parts(definition, QStringLiteral("terminal"));
			for (int i = 0; i < list.size(); ++i) {
				const auto t = list.at(i).toElement();
				if (side.isEmpty()
					|| t.attribute(QStringLiteral("orientation")) == side) {
					names << t.attribute(QStringLiteral("name"));
				}
			}
			return names;
		}

			// the element of an order number
		static QString fileOf(const QString &reference)
		{
			for (const auto &path : elements()) {
				if (info(load(path), QStringLiteral("manufacturer_reference"))
					== reference) {
					return path;
				}
			}
			return QString();
		}

	private slots:
		void everyReferenceHasAnElement()
		{
			const QStringList references{
				QStringLiteral("570480"), QStringLiteral("540003"),
				QStringLiteral("630311"), QStringLiteral("C1000074"),
				QStringLiteral("C1000073"), QStringLiteral("6M000008"),
				QStringLiteral("6M000009"), QStringLiteral("6M000007")};
			QCOMPARE(elements().size(), references.size());
			for (const auto &reference : references) {
				const QString path = fileOf(reference);
				QVERIFY2(!path.isEmpty(), qPrintable(reference));
				QCOMPARE(info(load(path), QStringLiteral("manufacturer")),
						 QStringLiteral("Pilz"));
			}
		}

		void directoriesHaveAnEnglishName()
		{
			QSet<QString> dirs{root()};
			for (const auto &path : elements()) {
				dirs << QFileInfo(path).path();
			}
			for (const auto &dir : dirs) {
				const auto names = load(dir + QStringLiteral("/qet_directory"))
					.firstChildElement(QStringLiteral("names"))
					.elementsByTagName(QStringLiteral("name"));
				bool english = false;
				for (int i = 0; i < names.size(); ++i) {
					english |= names.at(i).toElement().attribute(
						QStringLiteral("lang")) == QLatin1String("en");
				}
				QVERIFY2(english, qPrintable(dir));
			}
		}

		void elementsAreWellFormed()
		{
			for (const auto &path : elements()) {
				const auto definition = load(path);
				QVERIFY2(definition.tagName() == QLatin1String("definition"),
						 qPrintable(path));
				QCOMPARE(definition.attribute(QStringLiteral("link_type")),
						 QStringLiteral("simple"));
				QVERIFY2(!parts(definition, QStringLiteral("image")).size(),
						 qPrintable(path));
				const auto names = terminals(definition);
				QVERIFY2(!names.isEmpty(), qPrintable(path));
				QCOMPARE(QSet<QString>(names.begin(), names.end()).size(),
						 names.size());
				QVERIFY2(!info(load(path), QStringLiteral("description"))
						 .isEmpty(), qPrintable(path));
			}
		}

		void terminalsAreOnTheGrid()
		{
			for (const auto &path : elements()) {
				const auto list = parts(load(path),
										QStringLiteral("terminal"));
				for (int i = 0; i < list.size(); ++i) {
					const auto t = list.at(i).toElement();
					for (const char *axis : {"x", "y"}) {
						const double v = t.attribute(
							QString::fromLatin1(axis)).toDouble();
						QVERIFY2(std::fmod(v, 10.0) == 0.0, qPrintable(path));
					}
				}
			}
		}

		void textsHaveAPosition()
		{
			for (const auto &path : elements()) {
				const auto list = parts(load(path), QStringLiteral("text"));
				for (int i = 0; i < list.size(); ++i) {
					const auto t = list.at(i).toElement();
					if (t.parentNode().nodeName()
						!= QLatin1String("description")) {
						continue;
					}
					QVERIFY2(t.attribute(QStringLiteral("text")).size() > 0,
							 qPrintable(path));
					QVERIFY2(t.hasAttribute(QStringLiteral("x"))
							 && t.hasAttribute(QStringLiteral("y")),
							 qPrintable(path));
				}
			}
		}

		void mgateTerminalsUseThePilzDesignations()
		{
			const QStringList expected{
				QStringLiteral("A1"), QStringLiteral("A2"),
				QStringLiteral("Y1"), QStringLiteral("S31"),
				QStringLiteral("S41"), QStringLiteral("B1"),
				QStringLiteral("B2"), QStringLiteral("LED1"),
				QStringLiteral("LED2"), QStringLiteral("12"),
				QStringLiteral("22"), QStringLiteral("Y32"),
				QStringLiteral("ES_SO"), QStringLiteral("ES1.1"),
				QStringLiteral("ES1.2"), QStringLiteral("ES2.1"),
				QStringLiteral("ES2.2")};
			for (const auto &reference : {QStringLiteral("6M000007"),
										  QStringLiteral("6M000008"),
										  QStringLiteral("6M000009")}) {
				auto names = terminals(load(fileOf(reference)));
				QCOMPARE(QSet<QString>(names.begin(), names.end()),
						 QSet<QString>(expected.begin(), expected.end()));
			}
		}

		void safetySwitchUsesThePilzDesignations()
		{
			auto names = terminals(load(fileOf(QStringLiteral("540003"))));
			names.sort();
			QCOMPARE(names, (QStringList{QStringLiteral("12"),
										 QStringLiteral("22"),
										 QStringLiteral("A1"),
										 QStringLiteral("A2")}));
		}

		void m12CableWiresFollowTheDatasheetColours()
		{
			const auto definition = load(fileOf(QStringLiteral("630311")));
			QCOMPARE(terminals(definition, QStringLiteral("w")),
					 (QStringList{QStringLiteral("1"), QStringLiteral("2"),
								  QStringLiteral("3"), QStringLiteral("4"),
								  QStringLiteral("5")}));
			QCOMPARE(terminals(definition, QStringLiteral("e")),
					 (QStringList{QStringLiteral("BN"), QStringLiteral("WH"),
								  QStringLiteral("BU"), QStringLiteral("BK"),
								  QStringLiteral("GY")}));
		}

		void m23CablesHaveNineteenColouredWires()
		{
			const QStringList colours{
				QStringLiteral("RD"), QStringLiteral("GY"),
				QStringLiteral("VT"), QStringLiteral("YE"),
				QStringLiteral("GN"), QStringLiteral("BU"),
				QStringLiteral("WH"), QStringLiteral("RDBU"),
				QStringLiteral("GYBN"), QStringLiteral("BNGN"),
				QStringLiteral("YEBN"), QStringLiteral("GNYE"),
				QStringLiteral("BK"), QStringLiteral("WHYE"),
				QStringLiteral("WHGN"), QStringLiteral("WHGY"),
				QStringLiteral("GYPK"), QStringLiteral("PK"),
				QStringLiteral("BN")};
			for (const auto &reference : {QStringLiteral("C1000073"),
										  QStringLiteral("C1000074")}) {
				const auto definition = load(fileOf(reference));
				const auto pins = terminals(definition, QStringLiteral("w"));
				QCOMPARE(pins.size(), 19);
				QCOMPARE(pins.first(), QStringLiteral("1"));
				QCOMPARE(pins.last(), QStringLiteral("19"));
				QCOMPARE(terminals(definition, QStringLiteral("e")), colours);
			}
		}

		void cableTerminalsFaceTheirSide()
		{
			for (const auto &reference : {QStringLiteral("630311"),
										  QStringLiteral("C1000073"),
										  QStringLiteral("C1000074")}) {
				const auto list = parts(load(fileOf(reference)),
										QStringLiteral("terminal"));
				for (int i = 0; i < list.size(); ++i) {
					const auto t = list.at(i).toElement();
					const bool west = t.attribute(QStringLiteral("x"))
						.toDouble() == 0.0;
					QCOMPARE(t.attribute(QStringLiteral("orientation")),
							 west ? QStringLiteral("w")
								  : QStringLiteral("e"));
				}
			}
		}
};

QTEST_APPLESS_MAIN(TstPilzElements)
#include "tst_pilzelements.moc"
