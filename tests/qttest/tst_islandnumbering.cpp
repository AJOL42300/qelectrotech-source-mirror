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
#include <QtSql>
#include <QtTest>

#include "island/islandnumbering.h"

class tst_IslandNumbering : public QObject
{
	Q_OBJECT

	private slots:
		void childLabelInheritsParent();
		void numberOfAcceptsDirectCardsOnly();
		void numbersAtTheEndGoByTen();
		void numbersBeforeFirst();
		void insertedCardTakesTheMiddle();
		void severalInsertedCardsAreSpread();
		void noRoomGivesNothing();
		void copyTakesTheNextFreeParent();
		void copyLabelsGoByTen();
		void queryListsCardsInNumberOrder();
		void queryEscapesTheLabel();
		void ioQueryListsChannelsByCard();
};

void tst_IslandNumbering::childLabelInheritsParent()
{
	QCOMPARE(IslandNumbering::childLabel("BOX 03", 10),
			 QString("BOX 03.10"));
}

void tst_IslandNumbering::numberOfAcceptsDirectCardsOnly()
{
	int n = 0;
	QVERIFY(IslandNumbering::numberOf("BOX 03.20", "BOX 03", &n));
	QCOMPARE(n, 20);
	QVERIFY(IslandNumbering::numberOf("BOX 03.15", "BOX 03", nullptr));
	QVERIFY(!IslandNumbering::numberOf("BOX 03", "BOX 03", &n));
	QVERIFY(!IslandNumbering::numberOf("BOX 03.", "BOX 03", &n));
	QVERIFY(!IslandNumbering::numberOf("BOX 03.10.5", "BOX 03", &n));
	QVERIFY(!IslandNumbering::numberOf("BOX 03.1a", "BOX 03", &n));
	QVERIFY(!IslandNumbering::numberOf("BOX 031.10", "BOX 03", &n));
	QVERIFY(!IslandNumbering::numberOf("BOX 03.10", QString(), &n));
}

void tst_IslandNumbering::numbersAtTheEndGoByTen()
{
	QCOMPARE(IslandNumbering::numbersAfter({}, 3, -1),
			 (QList<int>{10, 20, 30}));
	QCOMPARE(IslandNumbering::numbersAfter({20, 10}, 2, -1),
			 (QList<int>{30, 40}));
	QCOMPARE(IslandNumbering::numbersAfter({10, 15}, 1, -1),
			 (QList<int>{20}));
	QVERIFY(IslandNumbering::numbersAfter({10}, 0, -1).isEmpty());
}

void tst_IslandNumbering::numbersBeforeFirst()
{
	QCOMPARE(IslandNumbering::numbersAfter({10, 20}, 1, 0),
			 (QList<int>{5}));
	QCOMPARE(IslandNumbering::numbersAfter({}, 2, 0),
			 (QList<int>{10, 20}));
}

void tst_IslandNumbering::insertedCardTakesTheMiddle()
{
	QCOMPARE(IslandNumbering::numbersAfter({10, 20, 30}, 1, 10),
			 (QList<int>{15}));
	QCOMPARE(IslandNumbering::numbersAfter({10, 15, 20}, 1, 10),
			 (QList<int>{12}));
	QCOMPARE(IslandNumbering::numbersAfter({10, 20}, 1, 20),
			 (QList<int>{30}));
}

void tst_IslandNumbering::severalInsertedCardsAreSpread()
{
	const QList<int> got = IslandNumbering::numbersAfter({10, 40}, 2, 10);
	QCOMPARE(got, (QList<int>{20, 30}));
}

void tst_IslandNumbering::noRoomGivesNothing()
{
	QVERIFY(IslandNumbering::numbersAfter({10, 11}, 1, 10).isEmpty());
	QVERIFY(IslandNumbering::numbersAfter({10, 13}, 3, 10).isEmpty());
	QCOMPARE(IslandNumbering::numbersAfter({10, 13}, 2, 10),
			 (QList<int>{11, 12}));
}

void tst_IslandNumbering::copyTakesTheNextFreeParent()
{
	QCOMPARE(IslandNumbering::nextParent("BOX 03", {"BOX 03"}),
			 QString("BOX 04"));
	QCOMPARE(IslandNumbering::nextParent("BOX 03",
										 {"BOX 03", "BOX 04.10"}),
			 QString("BOX 05"));
	QCOMPARE(IslandNumbering::nextParent("BOX 009", {}),
			 QString("BOX 010"));
	QCOMPARE(IslandNumbering::nextParent("CC", {"CC"}), QString("CC 2"));
	QVERIFY(IslandNumbering::isTaken("BOX 03", {"BOX 03.10"}));
	QVERIFY(!IslandNumbering::isTaken("BOX 03", {"BOX 030", "BOX 3"}));
}

void tst_IslandNumbering::copyLabelsGoByTen()
{
	QCOMPARE(IslandNumbering::copyLabels("BOX 04", 3),
			 (QStringList{"BOX 04.10", "BOX 04.20", "BOX 04.30"}));
	QVERIFY(IslandNumbering::copyLabels("BOX 04", 0).isEmpty());
}

static QStringList runCards(const QString &parent,
							const QStringList &labels)
{
	QStringList result;
	const QString name = QStringLiteral("islandtest");
	{
		QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", name);
		db.setDatabaseName(":memory:");
		if (!db.open()) {
			return {"<open failed>"};
		}
		QSqlQuery q(db);
		q.exec("CREATE TABLE element_nomenclature_view (label TEXT, "
			   "designation TEXT, manufacturer TEXT, "
			   "manufacturer_reference TEXT)");
		for (const QString &l : labels) {
			q.prepare("INSERT INTO element_nomenclature_view "
					  "VALUES (?, '', '', '')");
			q.addBindValue(l);
			q.exec();
		}
		if (!q.exec(IslandNumbering::cardsQuery(parent))) {
			return {"<query failed: " + q.lastError().text() + ">"};
		}
		while (q.next()) {
			result << q.value(0).toString();
		}
	}
	QSqlDatabase::removeDatabase(name);
	return result;
}

void tst_IslandNumbering::queryListsCardsInNumberOrder()
{
	const QStringList got = runCards(
		"BOX 03",
		{"BOX 03", "BOX 03.100", "BOX 03.20", "BOX 03.15", "BOX 03.10",
		 "BOX 03.10.5", "BOX 031.10", "BOX 03.x", "BOX 03.", "BOX 04.10"});
	QCOMPARE(got, (QStringList{"BOX 03.10", "BOX 03.15", "BOX 03.20",
							   "BOX 03.100"}));
}

void tst_IslandNumbering::queryEscapesTheLabel()
{
	QCOMPARE(runCards("A_B", {"A_B.10", "AXB.10"}),
			 (QStringList{"A_B.10"}));
	QCOMPARE(runCards("A'B", {"A'B.10", "A'B.20"}),
			 (QStringList{"A'B.10", "A'B.20"}));
	QCOMPARE(runCards("100%", {"100%.10", "1000.10"}),
			 (QStringList{"100%.10"}));
}

void tst_IslandNumbering::ioQueryListsChannelsByCard()
{
	const QString name = QStringLiteral("islandio");
	QStringList got;
	{
		QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", name);
		db.setDatabaseName(":memory:");
		QVERIFY(db.open());
		QSqlQuery q(db);
		QVERIFY(q.exec("CREATE TABLE qet_plc_io (plc TEXT, entry INTEGER, "
					   "address TEXT, type TEXT, function TEXT, "
					   "comment TEXT, plc_folio_number INTEGER)"));
		const QList<QPair<QString, int>> rows{
			{"BOX 03.20", 2}, {"BOX 03.20", 1}, {"BOX 03.100", 1},
			{"BOX 03.10", 1}, {"BOX 03", 1}, {"BOX 04.10", 1},
			{"BOX 03.10.5", 1}, {"BOX 031.10", 1}};
		for (const auto &r : rows) {
			q.prepare("INSERT INTO qet_plc_io VALUES (?, ?, NULL, 'DI', "
					  "NULL, NULL, 7)");
			q.addBindValue(r.first);
			q.addBindValue(r.second);
			QVERIFY(q.exec());
		}
		QVERIFY2(q.exec(IslandNumbering::ioQuery("BOX 03")),
				 qPrintable(q.lastError().text()));
		QCOMPARE(q.record().fieldName(0), QString("card"));
		while (q.next()) {
			got << q.value(0).toString() + "/" + q.value(1).toString()
				   + "/" + q.value(6).toString();
		}
	}
	QSqlDatabase::removeDatabase(name);
	QCOMPARE(got, (QStringList{"BOX 03/1/7", "BOX 03.10/1/7",
							   "BOX 03.20/1/7", "BOX 03.20/2/7",
							   "BOX 03.100/1/7"}));
}

QTEST_GUILESS_MAIN(tst_IslandNumbering)
#include "tst_islandnumbering.moc"
