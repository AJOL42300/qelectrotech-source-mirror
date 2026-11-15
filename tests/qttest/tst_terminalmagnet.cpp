// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include "terminalmagnet.h"

class tst_TerminalMagnet : public QObject
{
	Q_OBJECT

	private slots:
		void facingTerminalsSnap();
		void sameOrientationIgnored();
		void farTerminalsIgnored();
		void nearestPairWins();
};

using TerminalMagnet::Dock;

void tst_TerminalMagnet::facingTerminalsSnap()
{
	// East terminal of the plug, West terminal of the base
	QList<Dock> moved{{QPointF(100, 50), 1}};
	QList<Dock> fixed{{QPointF(104, 52), 3}};
	QPointF c;
	QVERIFY(TerminalMagnet::correction(
		moved, fixed, TerminalMagnet::tolerance, c));
	QCOMPARE(c, QPointF(4, 2));
}

void tst_TerminalMagnet::sameOrientationIgnored()
{
	QList<Dock> moved{{QPointF(100, 50), 1}};
	QList<Dock> fixed{{QPointF(104, 50), 1}};
	QPointF c;
	QVERIFY(!TerminalMagnet::correction(
		moved, fixed, TerminalMagnet::tolerance, c));
}

void tst_TerminalMagnet::farTerminalsIgnored()
{
	QList<Dock> moved{{QPointF(0, 0), 0}};
	QList<Dock> fixed{{QPointF(0, 40), 2}};
	QPointF c;
	QVERIFY(!TerminalMagnet::correction(
		moved, fixed, TerminalMagnet::tolerance, c));
}

void tst_TerminalMagnet::nearestPairWins()
{
	QList<Dock> moved{{QPointF(0, 0), 1}, {QPointF(0, 20), 1}};
	QList<Dock> fixed{{QPointF(8, 0), 3}, {QPointF(3, 20), 3}};
	QPointF c;
	QVERIFY(TerminalMagnet::correction(
		moved, fixed, TerminalMagnet::tolerance, c));
	QCOMPARE(c, QPointF(3, 0));
}

QTEST_APPLESS_MAIN(tst_TerminalMagnet)
#include "tst_terminalmagnet.moc"
