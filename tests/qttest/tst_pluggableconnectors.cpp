// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QSet>

/**
	The generic pluggable terminal blocks of
	extra-elements/16_pluggable_connectors: each pin has a wire side
	(west) and a plug side (east) terminal sharing a potential, so the
	id of a wire carries across the plug, and every terminal lies on the
	grid so that terminals can meet.
*/
class tst_PluggableConnectors : public QObject
{
	Q_OBJECT

	private slots:
		void elementsAreWellFormed();
};

void tst_PluggableConnectors::elementsAreWellFormed()
{
	QDir dir(QStringLiteral(QET_PLUGGABLE_DIR));
	const QStringList files = dir.entryList({QStringLiteral("*.elmt")});
	QVERIFY2(files.count() >= 2, "single and double blocks expected");

	for (const QString &file : files)
	{
		QFile f(dir.filePath(file));
		QVERIFY2(f.open(QIODevice::ReadOnly), qPrintable(file));
		QDomDocument doc;
		QVERIFY2(doc.setContent(&f), qPrintable(file));

		QSet<QString> west, east, names;
		const QDomNodeList terminals = doc.elementsByTagName(
			QStringLiteral("terminal"));
		for (int i = 0; i < terminals.count(); ++i)
		{
			const QDomElement t = terminals.at(i).toElement();
			const QString potential = t.attribute(QStringLiteral("potential"));
			QVERIFY2(!potential.isEmpty(), qPrintable(file));
			QVERIFY2(!names.contains(t.attribute(QStringLiteral("name"))),
				 qPrintable(file));
			names.insert(t.attribute(QStringLiteral("name")));
			QCOMPARE(t.attribute(QStringLiteral("x")).toInt() % 10, 0);
			QCOMPARE(t.attribute(QStringLiteral("y")).toInt() % 10, 0);
			const QString orientation = t.attribute(QStringLiteral("orientation"));
			if (orientation == QLatin1String("w") || orientation == QLatin1String("n"))
				west.insert(potential);
			else
				east.insert(potential);
		}
		QVERIFY2(!west.isEmpty(), qPrintable(file));
		QCOMPARE(west, east);
		QCOMPARE(terminals.count(), west.count() * 2);
	}
}

QTEST_APPLESS_MAIN(tst_PluggableConnectors)
#include "tst_pluggableconnectors.moc"
