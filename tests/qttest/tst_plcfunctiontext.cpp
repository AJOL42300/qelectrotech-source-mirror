// SPDX-License-Identifier: GPL-2.0-or-later
#include "autoNum/assignvariables.h"
#include "diagram.h"
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetinformation.h"
#include "qetproject.h"
#include "undocommand/linkelementcommand.h"

#include <QDomDocument>
#include <QUuid>

/**
	A PLC element shows the "Function" of any row of its own table with
	%{plc_function_N} (N from 1, no upper limit): the column the cross
	reference table of the PLC shows. Display only, and it follows the
	table. The slave side %{plc_function} is not changed by it.
*/
class tst_plcfunctiontext : public QObject
{
	Q_OBJECT

	Element *symbol(Diagram *diagram, bool master)
	{
		const QString kind = master ? QStringLiteral("plc_card") : QStringLiteral("plc_point");
		auto *collection = diagram->project()->embeddedElementCollection();
		if (!collection->exist(QStringLiteral("import/") + kind + QStringLiteral(".elmt"))) {
			QDomDocument definition;
			definition.setContent(QString(
				R"(<definition type="element" link_type="%1" uuid="%2" width="40" height="60" hotspot_x="20" hotspot_y="30">
<names><name lang="en">%3</name></names>
<kindInformations><kindInformation name="type">plc</kindInformation></kindInformations>
<description><line x1="-20" y1="-30" x2="20" y2="-30"/>
<terminal x="0" y="30" orientation="s" uuid="%4"/></description></definition>)")
				.arg(master ? QStringLiteral("master") : QStringLiteral("slave"),
					 QUuid::createUuid().toString(), kind, QUuid::createUuid().toString()));
			if (!collection->addElementDefinition(QStringLiteral("import"), kind,
												  definition.documentElement()))
				return nullptr;
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/") + kind + QStringLiteral(".elmt"),
							 diagram->project()), nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		return element;
	}

	static ElementData::PlcIO entry(const QString &function)
	{
		ElementData::PlcIO io;
		io.type = ElementData::EntreeDigitale;
		io.functionText = function;
		io.terminalCount = 1;
		return io;
	}

	static void setFunctions(Element *card, const QStringList &functions)
	{
		ElementData data = card->elementData();
		auto plc = data.plcMasterData();
		plc.ios.clear();
		for (const QString &function : functions) {
			plc.ios << entry(function);
		}
		data.setPlcMasterData(plc);
		card->setElementData(data);
	}

	static DynamicElementTextItem *addComposite(Element *e, const QString &text)
	{
		auto *deti = new DynamicElementTextItem(e);
		e->addDynamicTextItem(deti);
		deti->setCompositeText(text);
		deti->setTextFrom(DynamicElementTextItem::CompositeText);
		return deti;
	}

private slots:
	void keys()
	{
		QCOMPARE(QETInformation::plcFunctionInfoKey(12), QStringLiteral("plc_function_12"));
		int channel = 0;
		QVERIFY(QETInformation::isPlcFunctionInfo(QStringLiteral("plc_function_13"), &channel));
		QCOMPARE(channel, 13);
		QVERIFY(!QETInformation::isPlcFunctionInfo(QStringLiteral("plc_function")));
		QVERIFY(!QETInformation::isPlcFunctionInfo(QStringLiteral("plc_function_0")));
		QVERIFY(!QETInformation::isPlcFunctionInfo(QStringLiteral("plc_function_1a")));
		QVERIFY(QETInformation::usesPlcFunctionInfo(QStringLiteral("a%{plc_function_2}")));
		QVERIFY(!QETInformation::usesPlcFunctionInfo(QStringLiteral("%{plc_function}")));
	}

	void oneVariablePerRowWithoutLimit()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		Element *card = symbol(folio, true);
		QVERIFY(card);

		QStringList functions;
		for (int i = 1 ; i <= 12 ; ++i) {
			functions << QString::number(10000 + i);
		}
		setFunctions(card, functions);

		auto *first = addComposite(card, QStringLiteral("%{plc_function_1}"));
		auto *ninth = addComposite(card, QStringLiteral("[%{plc_function_9}]"));
		auto *last = addComposite(card, QStringLiteral("%{plc_function_12}"));
		auto *both = addComposite(card, QStringLiteral("%{plc_function_2}/%{plc_function_10}"));
		auto *beyond = addComposite(card, QStringLiteral("(%{plc_function_13})"));
		QCOMPARE(first->toPlainText(), QStringLiteral("10001"));
		QCOMPARE(ninth->toPlainText(), QStringLiteral("[10009]"));
		QCOMPARE(last->toPlainText(), QStringLiteral("10012"));
		QCOMPARE(both->toPlainText(), QStringLiteral("10002/10010"));
		QCOMPARE(beyond->toPlainText(), QStringLiteral("()"));

			//Display only: nothing is stored in the element
		for (const QString &key : card->elementInformations().keys()) {
			QVERIFY(!QETInformation::isPlcFunctionInfo(key));
		}
	}

	void textFollowsTheTable()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		Element *card = symbol(folio, true);
		QVERIFY(card);
		setFunctions(card, {QStringLiteral("20001"), QStringLiteral("20002")});
		auto *text = addComposite(card, QStringLiteral("%{plc_function_2}"));
		QCOMPARE(text->toPlainText(), QStringLiteral("20002"));

		setFunctions(card, {QStringLiteral("20001"), QStringLiteral("30002")});
		QCOMPARE(text->toPlainText(), QStringLiteral("30002"));

			//A row removed: the text of that row is empty
		setFunctions(card, {QStringLiteral("20001")});
		QCOMPARE(text->toPlainText(), QString());
	}

	void slaveFunctionIsUnchanged()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		Element *card = symbol(folio, true);
		Element *point = symbol(folio, false);
		QVERIFY(card && point);
		setFunctions(card, {QStringLiteral("40001"), QStringLiteral("40002")});

		auto *link = new LinkElementCommand(point);
		link->setLink(card);
		link->setGroupIndex(1);
		project.undoStack()->push(link);
		QCOMPARE(card->groupIndexForElement(point), 1);
			//The master pushes a changed table to its slaves
		setFunctions(card, {QStringLiteral("41001"), QStringLiteral("41002")});

		auto *slave_text = addComposite(point, QStringLiteral("%{plc_function}|%{plc_function_2}"));
		QCOMPARE(slave_text->toPlainText(), QStringLiteral("41002|"));
		auto *master_text = addComposite(card, QStringLiteral("%{plc_function}|%{plc_function_2}"));
		QCOMPARE(master_text->toPlainText(), QStringLiteral("|41002"));
	}

	void replaceVariableUsesTheContext()
	{
		DiagramContext dc;
		dc.addValue(QStringLiteral("plc_function_11"), QStringLiteral("50011"));
		QCOMPARE(autonum::AssignVariables::replaceVariable(
					 QStringLiteral("%{plc_function_11}%{plc_function_111}"), dc),
				 QStringLiteral("50011"));
	}
};

QET_APP_TEST_MAIN(tst_plcfunctiontext)
#include "tst_plcfunctiontext.moc"
