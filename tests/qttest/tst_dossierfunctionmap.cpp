// SPDX-License-Identifier: GPL-2.0-or-later
/*
	The function -> folios map of the HTML technical dossier: reading the
	functions out of an overview SVG, the hand-written JSON and CSV forms, and
	guessing the folios from the texts of the folios.
*/
#include <QtTest>
#include "htmlexport/dossierfunctionmap.h"

using namespace DossierFunctionMap;

class tst_dossierfunctionmap : public QObject
{
	Q_OBJECT

private slots:
	void functionsFromSvg()
	{
		const QByteArray svg =
			"<svg>"
			"<a class=\"function-link\" aria-label=\"01 - 001 TRF1 - Chain 1\""
			" href=\"#zoom-fonction-01\"><rect/></a>"
			"<a class=\"other\" aria-label=\"02 - X - Y\" href=\"#a\"/>"
			"<a class=\"function-link\" aria-label=\"01 - 001 TRF1 - Again\""
			" href=\"#zoom-fonction-01\"/>"
			"<g data-electrical-ref=\"001 trf1\" data-source-folios=\"4-5\"/>"
			"</svg>";
		const QList<Function> functions = DossierFunctionMap::functionsFromSvg(svg);
		QCOMPARE(functions.size(), 1);
		QCOMPARE(functions.at(0).id, QStringLiteral("01"));
		QCOMPARE(functions.at(0).tag, QStringLiteral("001 TRF1"));
		QCOMPARE(functions.at(0).title, QStringLiteral("Chain 1"));
		QCOMPARE(functions.at(0).anchor, QStringLiteral("zoom-fonction-01"));
		QCOMPARE(functions.at(0).hint, (QList<int>{4, 5}));
	}

	void folioLists()
	{
		QCOMPARE(parseFolioList(QStringLiteral("1,3-5 8"), 10),
				 (QList<int>{1, 3, 4, 5, 8}));
		QCOMPARE(parseFolioList(QStringLiteral("0, 9-12, x, 2-"), 10),
				 (QList<int>{9, 10}));
		QCOMPARE(canonicalId(QStringLiteral("007")), QStringLiteral("7"));
		QCOMPARE(canonicalId(QStringLiteral("A1")), QStringLiteral("A1"));
	}

	void parseForms_data()
	{
		QTest::addColumn<QByteArray>("text");
		QTest::newRow("json object")
			<< QByteArray("{\"functions\": {\"01\": {\"folios\": [2, 3]}}}");
		QTest::newRow("json flat list")
			<< QByteArray("{\"1\": [2, 3]}");
		QTest::newRow("json flat string")
			<< QByteArray("{\"01\": \"2-3\"}");
		QTest::newRow("csv")
			<< QByteArray("function;folios\n# comment\n01;2,3\n");
		QTest::newRow("csv tab")
			<< QByteArray("1\t2-3\n");
	}

	void parseForms()
	{
		QFETCH(QByteArray, text);
		Map map;
		QString error;
		QVERIFY2(DossierFunctionMap::parse(text, 5, &map, &error),
				 qPrintable(error));
		QCOMPARE(map.value(canonicalId(QStringLiteral("01"))),
				 (QList<int>{2, 3}));
	}

	void parseRejectsGarbage()
	{
		Map map;
		QString error;
		QVERIFY(!DossierFunctionMap::parse(QByteArray("{ not json"), 5, &map,
										   &error));
		QVERIFY(!error.isEmpty());
	}

	void jsonRoundTrip()
	{
		Function f;
		f.id = QStringLiteral("01");
		f.tag = QStringLiteral("001 TRF1");
		Map map;
		map[QStringLiteral("1")] = {2, 7};
		Map back;
		QString error;
		QVERIFY2(DossierFunctionMap::parse(toJson(map, {f}), 10, &back, &error),
				 qPrintable(error));
		QCOMPARE(back, map);
	}

	void deduceFromTags()
	{
		Function trf;
		trf.id = QStringLiteral("01");
		trf.tag = QStringLiteral("001 TRF1");
		Function pump;
		pump.id = QStringLiteral("02");
		pump.tag = QStringLiteral("002 PMP1");
		Function none;
		none.id = QStringLiteral("03");
		none.tag = QStringLiteral("003 ZZZ9");

		const QList<QStringList> texts{
			{QStringLiteral("cover")},
			{QStringLiteral("001 trf1"), QStringLiteral("001TRF1 supply")},
			{QStringLiteral("002 PMP1")},
			{QStringLiteral("nothing")}};
		const Map map = deduce({trf, pump, none}, texts);
		QCOMPARE(map.value(QStringLiteral("1")), (QList<int>{2}));
		QCOMPARE(map.value(QStringLiteral("2")), (QList<int>{3}));
		QVERIFY(!map.contains(QStringLiteral("3")));
	}

	void deduceIgnoresGeneralFolios()
	{
		QList<Function> functions;
		QStringList everything;
		QList<QStringList> texts{{}};
		for (int i = 1; i <= 6; ++i) {
			Function f;
			f.id = QString::number(i);
			f.tag = QStringLiteral("00%1 EQ%1").arg(i);
			functions << f;
			everything << f.tag;
			texts << QStringList{f.tag};
		}
		texts[0] = everything; // an index of all the functions
		const Map map = deduce(functions, texts);
		for (int i = 1; i <= 6; ++i)
			QCOMPARE(map.value(QString::number(i)), (QList<int>{i + 1}));
	}

	void deduceCodeInsideWords()
	{
		Function f;
		f.id = QStringLiteral("08");
		f.tag = QStringLiteral("RLX3 et \u00e9jecteur 009 EJC1");
		const Map map = deduce({f}, {{QStringLiteral("x")},
									 {QStringLiteral("009 EJC1")}});
		QCOMPARE(map.value(QStringLiteral("8")), (QList<int>{2}));
	}
};

QTEST_APPLESS_MAIN(tst_dossierfunctionmap)
#include "tst_dossierfunctionmap.moc"
