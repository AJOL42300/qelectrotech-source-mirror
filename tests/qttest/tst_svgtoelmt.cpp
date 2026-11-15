// SPDX-License-Identifier: GPL-2.0-or-later
#include <QDomDocument>
#include <QtTest>

#include "svgtoelmt.h"

using namespace SvgToElmt;

namespace {

QDomDocument parse(const QByteArray &definition)
{
	QDomDocument doc;
	QString error;
	if (!doc.setContent(definition, &error))
		qWarning() << "not well formed:" << error;
	return doc;
}

QDomElement description(const QDomDocument &doc)
{
	return doc.documentElement().firstChildElement(QStringLiteral("description"));
}

QList<QDomElement> partsOf(const QDomDocument &doc, const QString &tag)
{
	QList<QDomElement> list;
	const QDomNodeList nodes = description(doc).elementsByTagName(tag);
	for (int i = 0; i < nodes.count(); ++i)
		list << nodes.at(i).toElement();
	return list;
}

const QByteArray device_svg(
	"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" height=\"60\" "
	"viewBox=\"0 0 100 60\">"
	"<rect x=\"0\" y=\"0\" width=\"100\" height=\"60\" fill=\"#cccccc\" "
	"stroke=\"black\"/>"
	"<circle cx=\"20\" cy=\"30\" r=\"8\" fill=\"none\" stroke=\"black\"/>"
	"<path d=\"M 40 10 L 90 10 L 90 50 Z\" fill=\"red\"/>"
	"<circle id=\"terminal-L1\" cx=\"10\" cy=\"0\" r=\"2\" "
	"data-orientation=\"n\"/>"
	"<rect id=\"terminal-PE\" x=\"88\" y=\"58\" width=\"4\" height=\"4\"/>"
	"</svg>");

}

class tst_SvgToElmt : public QObject
{
	Q_OBJECT

private slots:
	void convertsShapesAndTerminals();
	void terminalShapesAreNotDrawn();
	void explicitTerminalsAreAdded();
	void frameIsOnTheGrid();
	void scaleResizesTheDrawing();
	void nameIsWrittenInTheLanguage();
	void rejectsWhatIsNotSvg();
	void warnsOfADrawingWithoutTerminal();
	void parsesTerminalArgument_data();
	void parsesTerminalArgument();
	void parsesTerminalFile();
};

void tst_SvgToElmt::convertsShapesAndTerminals()
{
	const Result r = convert(device_svg, Options());
	QVERIFY2(r.ok, qPrintable(r.error));
	QCOMPARE(r.terminals, 2);

	const QDomDocument doc = parse(r.definition);
	const QDomElement root = doc.documentElement();
	QCOMPARE(root.tagName(), QStringLiteral("definition"));
	QCOMPARE(root.attribute(QStringLiteral("type")),
		 QStringLiteral("element"));
	QVERIFY(!root.firstChildElement(QStringLiteral("uuid"))
		.attribute(QStringLiteral("uuid")).isEmpty());

	QCOMPARE(partsOf(doc, QStringLiteral("rect")).count(), 1);
	QCOMPARE(partsOf(doc, QStringLiteral("circle")).count(), 1);
	QCOMPARE(partsOf(doc, QStringLiteral("polygon")).count(), 1);
	QCOMPARE(partsOf(doc, QStringLiteral("terminal")).count(), 2);
}

void tst_SvgToElmt::terminalShapesAreNotDrawn()
{
	const Result r = convert(device_svg, Options());
	QVERIFY(r.ok);
	const QDomDocument doc = parse(r.definition);

	QStringList names;
	for (const QDomElement &t : partsOf(doc, QStringLiteral("terminal")))
		names << t.attribute(QStringLiteral("name"));
	names.sort();
	QCOMPARE(names, QStringList({QStringLiteral("L1"),
				     QStringLiteral("PE")}));

	// The marker circle must not come out as a second circle.
	QCOMPARE(partsOf(doc, QStringLiteral("circle")).count(), 1);
}

void tst_SvgToElmt::explicitTerminalsAreAdded()
{
	Options options;
	Terminal t;
	t.name = QStringLiteral("A1");
	t.position = QPointF(50, 60);
	t.orientation = QStringLiteral("s");
	options.terminals << t;

	const Result r = convert(device_svg, options);
	QVERIFY(r.ok);
	QCOMPARE(r.terminals, 3);

	const QDomDocument doc = parse(r.definition);
	bool found = false;
	for (const QDomElement &e : partsOf(doc, QStringLiteral("terminal"))) {
		if (e.attribute(QStringLiteral("name")) == QLatin1String("A1")) {
			found = true;
			QCOMPARE(e.attribute(QStringLiteral("orientation")),
				 QStringLiteral("s"));
		}
	}
	QVERIFY(found);
}

void tst_SvgToElmt::frameIsOnTheGrid()
{
	const Result r = convert(device_svg, Options());
	QVERIFY(r.ok);
	const QDomElement root = parse(r.definition).documentElement();
	const int width = root.attribute(QStringLiteral("width")).toInt();
	const int height = root.attribute(QStringLiteral("height")).toInt();
	QVERIFY(width >= 100 && width % 10 == 0);
	QVERIFY(height >= 60 && height % 10 == 0);
}

void tst_SvgToElmt::scaleResizesTheDrawing()
{
	Options half;
	half.scale = 0.5;
	const Result r1 = convert(device_svg, Options());
	const Result r2 = convert(device_svg, half);
	QVERIFY(r1.ok && r2.ok);
	const int w1 = parse(r1.definition).documentElement()
		.attribute(QStringLiteral("width")).toInt();
	const int w2 = parse(r2.definition).documentElement()
		.attribute(QStringLiteral("width")).toInt();
	QVERIFY2(w2 < w1, "a half scale must give a narrower element");

	Options wide;
	wide.width = 200;
	const Result r3 = convert(device_svg, wide);
	QVERIFY(r3.ok);
	QVERIFY(parse(r3.definition).documentElement()
		.attribute(QStringLiteral("width")).toInt() >= 200);
}

void tst_SvgToElmt::nameIsWrittenInTheLanguage()
{
	Options options;
	options.name = QStringLiteral("AF1215");
	options.language = QStringLiteral("fr");
	const Result r = convert(device_svg, options);
	QVERIFY(r.ok);

	const QDomElement name = parse(r.definition).documentElement()
		.firstChildElement(QStringLiteral("names"))
		.firstChildElement(QStringLiteral("name"));
	QCOMPARE(name.attribute(QStringLiteral("lang")), QStringLiteral("fr"));
	QCOMPARE(name.text(), QStringLiteral("AF1215"));
}

void tst_SvgToElmt::rejectsWhatIsNotSvg()
{
	const Result r = convert("this is not a drawing", Options());
	QVERIFY(!r.ok);
	QVERIFY(!r.error.isEmpty());
}

void tst_SvgToElmt::warnsOfADrawingWithoutTerminal()
{
	const QByteArray svg(
		"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"40\" "
		"height=\"40\"><rect width=\"40\" height=\"40\"/></svg>");
	const Result r = convert(svg, Options());
	QVERIFY(r.ok);
	QCOMPARE(r.terminals, 0);
	QVERIFY(!r.warnings.isEmpty());
}

void tst_SvgToElmt::parsesTerminalArgument_data()
{
	QTest::addColumn<QString>("text");
	QTest::addColumn<bool>("valid");
	QTest::addColumn<QString>("name");
	QTest::addColumn<QString>("orientation");
	QTest::addColumn<qreal>("x");

	QTest::newRow("position only")
		<< "10,20" << true << QString() << QString() << 10.0;
	QTest::newRow("orientation")
		<< "10,20,s" << true << QString() << "s" << 10.0;
	QTest::newRow("name")
		<< "5.5,20,w,L1" << true << "L1" << "w" << 5.5;
	QTest::newRow("not a number")
		<< "a,b" << false << QString() << QString() << 0.0;
	QTest::newRow("bad orientation")
		<< "1,2,up" << false << QString() << QString() << 0.0;
	QTest::newRow("empty") << "" << false << QString() << QString() << 0.0;
}

void tst_SvgToElmt::parsesTerminalArgument()
{
	QFETCH(QString, text);
	QFETCH(bool, valid);
	QFETCH(QString, name);
	QFETCH(QString, orientation);
	QFETCH(qreal, x);

	Terminal t;
	QCOMPARE(parseTerminal(text, &t), valid);
	if (!valid)
		return;
	QCOMPARE(t.name, name);
	QCOMPARE(t.orientation, orientation);
	QCOMPARE(t.position.x(), x);
}

void tst_SvgToElmt::parsesTerminalFile()
{
	QStringList errors;
	const QList<Terminal> list = parseTerminals(
		QStringLiteral("name;x;y;orientation\n"
			       "# comment\n"
			       "\n"
			       "L1;10;0;n\n"
			       "U\t90\t60\n"
			       "broken line\n"),
		&errors);
	QCOMPARE(list.count(), 2);
	QCOMPARE(list.at(0).name, QStringLiteral("L1"));
	QCOMPARE(list.at(0).orientation, QStringLiteral("n"));
	QCOMPARE(list.at(1).name, QStringLiteral("U"));
	QCOMPARE(list.at(1).position, QPointF(90, 60));
	QCOMPARE(errors.count(), 1);
}

QTEST_MAIN(tst_SvgToElmt)
#include "tst_svgtoelmt.moc"
