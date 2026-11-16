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
#include "moduletooltip.h"

#include <QtTest>

class TstModuleTooltip : public QObject
{
	Q_OBJECT

	private:
		static ElementData::PlcIO io(const QString &address,
					     const QString &function,
					     ElementData::PlcIOType type =
						ElementData::EntreeDigitale)
		{
			ElementData::PlcIO entry;
			entry.address = address;
			entry.functionText = function;
			entry.type = type;
			return entry;
		}

	private slots:
		void entriesFollowTheTerminalName();
		void listedTerminalsWinOverTheAddress();
		void potentialsLinkTerminals();
		void wiresAndSignalsAreListed();
		void nothingToSayGivesNoText();
		void usedAndFreePinsAreTold();
		void textIsEscaped();
};

void TstModuleTooltip::entriesFollowTheTerminalName()
{
	QVector<ElementData::PlcIO> ios;
	ios << io("X01(1)", "a") << io("X01(2)", "b") << io("X02", "c")
	    << io("X011", "d");
	QCOMPARE(ModuleTooltip::entriesOf("X01", ios).size(), 2);
	QCOMPARE(ModuleTooltip::entriesOf("X02", ios).size(), 1);
	QCOMPARE(ModuleTooltip::entriesOf("X03", ios).size(), 0);
	QCOMPARE(ModuleTooltip::entriesOf(QString(), ios).size(), 0);
}

void TstModuleTooltip::listedTerminalsWinOverTheAddress()
{
	// channel I2 sits on the Beckhoff terminal 5
	ElementData::PlcIO i2 = io("I2", "sensor");
	i2.terminals << "5";
	QVector<ElementData::PlcIO> ios;
	ios << io("I1", "other") << i2;
	QCOMPARE(ModuleTooltip::entriesOf("5", ios).size(), 1);
	QCOMPARE(ModuleTooltip::entriesOf("5", ios).first().address,
		 QString("I2"));
	// "I1" has no terminal list: the legacy address rule still applies
	QCOMPARE(ModuleTooltip::entriesOf("I1", ios).size(), 1);
	QCOMPARE(ModuleTooltip::entriesOf("2", ios).size(), 0);

	ModuleTooltip::Pin pin;
	pin.name = "5";
	const QString text = ModuleTooltip::pinText(pin, {pin}, ios);
	QVERIFY(text.contains("<b>5</b> &rarr; <b>I2</b>"));
}

void TstModuleTooltip::potentialsLinkTerminals()
{
	QList<ModuleTooltip::Pin> pins;
	pins << ModuleTooltip::Pin{"1", "24V", {}}
	     << ModuleTooltip::Pin{"3", "24V", {}}
	     << ModuleTooltip::Pin{"2", "0V", {}};
	const QString text = ModuleTooltip::moduleText(
		"EL1008", pins, QVector<ElementData::PlcIO>());
	QVERIFY(text.contains("24V"));
	QVERIFY(text.contains("1 = 3"));
	// a potential carried by one terminal only links nothing
	QVERIFY(!text.contains("<b>0V</b>:"));
	const QString line = ModuleTooltip::pinText(
		pins.first(), pins, QVector<ElementData::PlcIO>());
	QVERIFY(line.contains("linked to 3"));
}

void TstModuleTooltip::wiresAndSignalsAreListed()
{
	QVector<ElementData::PlcIO> ios;
	ios << io("X01(1)", "Presence", ElementData::EntreeDigitale)
	    << io("Q9", "Not wired", ElementData::SortieDigitale);
	QList<ModuleTooltip::Pin> pins;
	pins << ModuleTooltip::Pin{"X01", QString(), {"W12", "W13"}};
	const QString text = ModuleTooltip::moduleText("BOX 02", pins, ios);
	QVERIFY(text.contains("BOX 02"));
	QVERIFY(text.contains("X01(1)"));
	QVERIFY(text.contains("DI"));
	QVERIFY(text.contains("Presence"));
	QVERIFY(text.contains("W12, W13"));
	// an entry that no terminal carries is not lost
	QVERIFY(text.contains("Q9"));
	QVERIFY(text.contains("DO"));
}

void TstModuleTooltip::nothingToSayGivesNoText()
{
	QList<ModuleTooltip::Pin> pins;
	pins << ModuleTooltip::Pin{"1", QString(), {}};
	QVERIFY(ModuleTooltip::moduleText(
			"A", pins, QVector<ElementData::PlcIO>()).isEmpty());
	QVERIFY(ModuleTooltip::moduleText(
			"A", {}, QVector<ElementData::PlcIO>()).isEmpty());
}

void TstModuleTooltip::usedAndFreePinsAreTold()
{
	QVector<ElementData::PlcIO> ios;
	ios << io("X01(1)", "Presence") << io("X02(1)", "Valve");
	ios.last().crossRef = QStringLiteral("folio 5");
	QList<ModuleTooltip::Pin> pins;
	pins << ModuleTooltip::Pin{"X01", QString(), {"W1"}, {"folio 3"}}
	     << ModuleTooltip::Pin{"X02", QString(), {}, {}}
	     << ModuleTooltip::Pin{"X03", QString(), {}, {}};
	QVERIFY(ModuleTooltip::isUsed(pins[0], ios));
	// a channel with a slave on another folio is used
	QVERIFY(ModuleTooltip::isUsed(pins[1], ios));
	QVERIFY(!ModuleTooltip::isUsed(pins[2], ios));

	const QString text = ModuleTooltip::moduleText("BOX 02", pins, ios);
	QVERIFY(text.contains("folio 3"));
	QVERIFY(text.contains("folio 5"));
	QVERIFY(text.contains("Free: X03"));
	QVERIFY(!text.contains("Free: X02"));

	QVERIFY(ModuleTooltip::pinText(pins[0], pins, ios)
		.contains("folio 3"));
	QVERIFY(ModuleTooltip::pinText(pins[2], pins, ios).contains("free"));
	QVERIFY(ModuleTooltip::pinText(pins[1], pins, ios).contains("used"));
}

void TstModuleTooltip::textIsEscaped()
{
	QVector<ElementData::PlcIO> ios;
	ios << io("1", "<b>x</b> & y");
	QList<ModuleTooltip::Pin> pins;
	pins << ModuleTooltip::Pin{"1", QString(), {}};
	const QString text = ModuleTooltip::moduleText("<T>", pins, ios);
	QVERIFY(!text.contains("<T>"));
	QVERIFY(text.contains("&lt;T&gt;"));
	QVERIFY(text.contains("&lt;b&gt;x&lt;/b&gt; &amp; y"));
}

QTEST_MAIN(TstModuleTooltip)
#include "tst_moduletooltip.moc"
