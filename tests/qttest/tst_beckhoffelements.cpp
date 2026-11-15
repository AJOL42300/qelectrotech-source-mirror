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
#include <QSet>
#include <QtTest>

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
};

QTEST_GUILESS_MAIN(TstBeckhoffElements)
#include "tst_beckhoffelements.moc"
