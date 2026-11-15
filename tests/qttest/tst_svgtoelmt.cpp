// SPDX-License-Identifier: GPL-2.0-or-later
#include <QDomDocument>
#include <QMimeData>
#include <QPainter>
#include <QTemporaryDir>
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
	void tracesAnAreaAndItsHole();
	void thresholdSelectsTheDarkPixels();
	void transparentPixelsAreLight();
	void despeckleAndMaxPathsLimitTheOutlines();
	void diagonalLinesAreSimplified();
	void rasterScaleAndGrid();
	void terminalsAreSnappedToTheGrid();
	void holesBecomeWhitePolygons();
	void findsWhatToConvertInMimeData();
	void readsAPictureFile();
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

namespace {

QImage whiteImage(int width, int height)
{
	QImage image(width, height, QImage::Format_RGB32);
	image.fill(Qt::white);
	return image;
}

void fillRect(QImage *image, const QRect &rect, const QColor &color)
{
	QPainter painter(image);
	painter.fillRect(rect, color);
}

/// The points of the polygon parts of an element.
int polygonCount(const Result &r)
{
	return partsOf(parse(r.definition), QStringLiteral("polygon")).count();
}

}

void tst_SvgToElmt::tracesAnAreaAndItsHole()
{
	QImage image = whiteImage(40, 40);
	fillRect(&image, QRect(5, 5, 30, 30), Qt::black);
	fillRect(&image, QRect(15, 15, 10, 10), Qt::white);

	const RasterVectorizer::Result r =
		RasterVectorizer::toSvg(image, RasterVectorizer::Options());
	QVERIFY2(r.ok, qPrintable(r.error));
	QCOMPARE(r.paths, 2);
	QVERIFY(r.svg.contains("fill=\"black\""));
	QVERIFY(r.svg.contains("fill=\"white\""));
	// The area comes first: the hole is drawn above it.
	QVERIFY(r.svg.indexOf("fill=\"black\"") < r.svg.indexOf("fill=\"white\""));
	// A square is simplified to its four corners.
	QCOMPARE(r.svg.count(" L"), 6);
}

void tst_SvgToElmt::thresholdSelectsTheDarkPixels()
{
	QImage image = whiteImage(20, 20);
	fillRect(&image, QRect(5, 5, 10, 10), QColor(100, 100, 100));

	RasterVectorizer::Options options;
	options.threshold = 50;
	QVERIFY(!RasterVectorizer::toSvg(image, options).ok);
	options.threshold = 150;
	QVERIFY(RasterVectorizer::toSvg(image, options).ok);

	// Inverted, the light pixels are drawn: the white around the square.
	options.invert = true;
	options.threshold = 200;
	const RasterVectorizer::Result inverted = RasterVectorizer::toSvg(image, options);
	QVERIFY(inverted.ok);
	QCOMPARE(inverted.paths, 2);
}

void tst_SvgToElmt::transparentPixelsAreLight()
{
	QImage image(20, 20, QImage::Format_ARGB32);
	image.fill(Qt::transparent);
	QVERIFY(!RasterVectorizer::toSvg(image, RasterVectorizer::Options()).ok);
	fillRect(&image, QRect(5, 5, 10, 10), Qt::black);
	QCOMPARE(RasterVectorizer::toSvg(image, RasterVectorizer::Options()).paths, 1);
}

void tst_SvgToElmt::despeckleAndMaxPathsLimitTheOutlines()
{
	QImage image = whiteImage(60, 20);
	fillRect(&image, QRect(2, 2, 16, 16), Qt::black);
	fillRect(&image, QRect(22, 2, 10, 10), Qt::black);
	fillRect(&image, QRect(40, 2, 4, 4), Qt::black);
	image.setPixelColor(55, 10, Qt::black);

	RasterVectorizer::Options options;
	options.despeckle = 2;
	options.maxPaths = 0;
	QCOMPARE(RasterVectorizer::toSvg(image, options).paths, 3);

	options.maxPaths = 2;
	const RasterVectorizer::Result r = RasterVectorizer::toSvg(image, options);
	QCOMPARE(r.paths, 2);
	QVERIFY(!r.warnings.isEmpty());
	// The two largest squares are kept.
	QVERIFY(r.svg.contains("18 "));
	QVERIFY(r.svg.contains("32 "));
	QVERIFY(!r.svg.contains("44 "));
}

void tst_SvgToElmt::diagonalLinesAreSimplified()
{
	QImage image = whiteImage(50, 50);
	QPainter painter(&image);
	painter.setPen(QPen(Qt::black, 3));
	painter.drawLine(5, 5, 45, 45);
	painter.end();

	RasterVectorizer::Options exact;
	exact.simplify = 0;
	RasterVectorizer::Options simple;
	simple.simplify = 1.5;
	const auto steps = RasterVectorizer::toSvg(image, exact).svg.count(" L");
	const auto few = RasterVectorizer::toSvg(image, simple).svg.count(" L");
	QVERIFY2(few < steps / 4, "a staircase must become a few segments");
	QVERIFY(few >= 3);
}

void tst_SvgToElmt::rasterScaleAndGrid()
{
	QImage image = whiteImage(100, 60);
	fillRect(&image, QRect(0, 0, 100, 60), Qt::black);

	// 96 dpi, 2 element pixels per millimetre: 100 px = 26.46 mm = 52.9
	Options mm;
	mm.pixelsPerMm = 2;
	mm.warnNoTerminal = false;
	Result r = convertImage(image, mm, RasterVectorizer::Options());
	QVERIFY2(r.ok, qPrintable(r.error));
	QVERIFY(r.warnings.isEmpty());
	QDomElement root = parse(r.definition).documentElement();
	QCOMPARE(root.attribute(QStringLiteral("width")).toInt(), 60);
	QCOMPARE(root.attribute(QStringLiteral("height")).toInt(), 40);

	// 200 dpi: the same picture is half as large
	r = convertImage(image, mm, RasterVectorizer::Options(), 192);
	root = parse(r.definition).documentElement();
	QCOMPARE(root.attribute(QStringLiteral("width")).toInt(), 40);

	// A width in grid cells
	Options cells;
	cells.widthInCells = 5;
	r = convertImage(image, cells, RasterVectorizer::Options());
	root = parse(r.definition).documentElement();
	// 5 cells = 50 px of drawing, in the symmetric frame of 60
	QCOMPARE(root.attribute(QStringLiteral("width")).toInt(), 60);
	QCOMPARE(root.attribute(QStringLiteral("height")).toInt() % 10, 0);
}

void tst_SvgToElmt::terminalsAreSnappedToTheGrid()
{
	QImage image = whiteImage(100, 60);
	fillRect(&image, QRect(0, 0, 100, 60), Qt::black);

	Options options;
	options.widthInCells = 10; // scale 1
	Terminal t;
	t.name = QStringLiteral("A");
	t.position = QPointF(13, 7);
	options.terminals << t;

	Result r = convertImage(image, options, RasterVectorizer::Options());
	QVERIFY(r.ok);
	QDomElement e = partsOf(parse(r.definition), QStringLiteral("terminal")).first();
	QCOMPARE(e.attribute(QStringLiteral("x")).toDouble(), -40.0);
	QCOMPARE(e.attribute(QStringLiteral("y")).toDouble(), -20.0);

	options.snapTerminals = false;
	r = convertImage(image, options, RasterVectorizer::Options());
	e = partsOf(parse(r.definition), QStringLiteral("terminal")).first();
	QCOMPARE(e.attribute(QStringLiteral("x")).toDouble(), -37.0);
	QCOMPARE(e.attribute(QStringLiteral("y")).toDouble(), -23.0);
}

void tst_SvgToElmt::holesBecomeWhitePolygons()
{
	QImage image = whiteImage(40, 40);
	fillRect(&image, QRect(5, 5, 30, 30), Qt::black);
	fillRect(&image, QRect(15, 15, 10, 10), Qt::white);
	Options options;
	options.warnNoTerminal = false;
	const Result r = convertImage(image, options, RasterVectorizer::Options());
	QVERIFY2(r.ok, qPrintable(r.error));
	QCOMPARE(polygonCount(r), 2);
	QCOMPARE(r.parts, 2);
}

void tst_SvgToElmt::findsWhatToConvertInMimeData()
{
	QByteArray svg;
	QImage image;

	QMimeData empty;
	QVERIFY(!sourceFromMimeData(&empty, &svg, &image));

	QMimeData text;
	text.setText(QStringLiteral("hello"));
	QVERIFY(!sourceFromMimeData(&text, &svg, &image));

	QMimeData picture;
	picture.setImageData(whiteImage(10, 10));
	QVERIFY(sourceFromMimeData(&picture, &svg, &image));
	QVERIFY(!image.isNull());
	QVERIFY(svg.isEmpty());

	QMimeData drawing;
	drawing.setData(QStringLiteral("image/svg+xml"), device_svg);
	QVERIFY(sourceFromMimeData(&drawing, &svg, &image));
	QCOMPARE(svg, device_svg);

	QMimeData source;
	source.setText(QString::fromUtf8(device_svg));
	QVERIFY(sourceFromMimeData(&source, &svg, &image));
	QVERIFY(svg.contains("<svg"));
}

void tst_SvgToElmt::readsAPictureFile()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	QImage image = whiteImage(30, 30);
	fillRect(&image, QRect(5, 5, 20, 20), Qt::black);
	const QString path = dir.filePath(QStringLiteral("logo.png"));
	QVERIFY(image.save(path));

	Options options;
	options.warnNoTerminal = false;
	const Result r = convertFile(path, options, RasterVectorizer::Options());
	QVERIFY2(r.ok, qPrintable(r.error));
	QCOMPARE(r.parts, 1);
	QVERIFY(!convertFile(dir.filePath(QStringLiteral("none.png")), options,
			     RasterVectorizer::Options()).ok);
}

QTEST_MAIN(tst_SvgToElmt)
#include "tst_svgtoelmt.moc"
