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
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "qetresult.h"
#include "twincat/tcqetbridge.h"
#include "undocommand/tcsynccommand.h"

#include <QDomDocument>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>

/**
	The application side of the TwinCAT I/O exchange: the path of the
	TwinCAT project is kept in the QET project, and what TwinCAT brings to
	the I/O tables of the PLC masters is one undo step.
*/
class tst_twincat_app : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	Element *card(Diagram *diagram)
	{
		auto *collection = diagram->project()->embeddedElementCollection();
		const QString kind = QStringLiteral("plc_card");
		if (!collection->exist(QStringLiteral("import/plc_card.elmt"))) {
			QDomDocument definition;
			definition.setContent(QString(
				R"(<definition type="element" link_type="master" uuid="%1" width="40" height="60" hotspot_x="20" hotspot_y="30">
<names><name lang="en">%2</name></names>
<kindInformations><kindInformation name="type">plc</kindInformation></kindInformations>
<description><line x1="-20" y1="-30" x2="20" y2="-30"/>
<terminal x="0" y="30" orientation="s" uuid="%3"/></description></definition>)")
				.arg(QUuid::createUuid().toString(), kind,
					 QUuid::createUuid().toString()));
			if (!collection->addElementDefinition(QStringLiteral("import"), kind,
												  definition.documentElement()))
				return nullptr;
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/plc_card.elmt"),
							 diagram->project()), nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		return element;
	}

	static ElementData::PlcIO row(const QString &address,
								  const QString &function,
								  const QString &comment = {})
	{
		ElementData::PlcIO io;
		io.type = ElementData::EntreeDigitale;
		io.address = address;
		io.functionText = function;
		io.comment = comment;
		io.terminalCount = 1;
		return io;
	}

private slots:
	void pathIsKeptInTheProject()
	{
		const QString path = m_dir.filePath(QStringLiteral("p.qet"));
		{
			QETProject project;
			project.addNewDiagram();
			QVERIFY(project.twincatPath().isEmpty());
			project.setTwincatPath(QStringLiteral("C:/tc/plc.tsproj"));
			QCOMPARE(project.twincatPath(), QStringLiteral("C:/tc/plc.tsproj"));
			project.setFilePath(path);
			QVERIFY(project.write().isOk());
		}
		QETProject again(path);
		QCOMPARE(again.twincatPath(), QStringLiteral("C:/tc/plc.tsproj"));
	}

	void projectWithoutPathSavesAsBefore()
	{
		QETProject project;
		project.addNewDiagram();
		const QString path = m_dir.filePath(QStringLiteral("none.qet"));
		project.setFilePath(path);
		QVERIFY(project.write().isOk());
		QFile f(path);
		QVERIFY(f.open(QIODevice::ReadOnly));
		QVERIFY(!f.readAll().contains("twincat_sync"));
		f.close();
		QETProject again(path);
		QVERIFY(again.twincatPath().isEmpty());
	}

	void ioTypeFollowsDirectionAndKind()
	{
		using D = TcIoTree::Direction;
		using K = TcIoTree::Kind;
		QCOMPARE(TcSyncCommand::ioType(D::Input, K::Digital),
				 ElementData::EntreeDigitale);
		QCOMPARE(TcSyncCommand::ioType(D::Output, K::Digital),
				 ElementData::SortieDigitale);
		QCOMPARE(TcSyncCommand::ioType(D::Input, K::Analog),
				 ElementData::EntreeAnalogique);
		QCOMPARE(TcSyncCommand::ioType(D::Output, K::Other),
				 ElementData::SortieUniverselle);
	}

	void tableUpdatedInOneUndoStep()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		Element *master = card(folio);
		QVERIFY(master);
		ElementData data = master->elementData();
		auto plc = data.plcMasterData();
		plc.ios = {row(QStringLiteral("X01(1)"), QStringLiteral("OLD"), QStringLiteral("keep me")),
				   row(QStringLiteral("X01(2)"), QStringLiteral("SAME"))};
		plc.ios[0].terminalCount = 3;
		data.setPlcMasterData(plc);
		master->setElementData(data);

		const auto cards = TcQetBridge::collect(&project);
		QCOMPARE(cards.cards.size(), 1);
		QCOMPARE(cards.cards.first().channels.size(), 2);

		TcSyncCardUpdate update;
		update.card = 0;
		update.channels = cards.cards.first().channels;
		update.channels[0].name = QStringLiteral("NEW");
		update.channels[0].comment = QStringLiteral("from TwinCAT");
		TcSyncQetChannel added;
		added.address = QStringLiteral("3");
		added.name = QStringLiteral("ADDED");
		added.direction = TcIoTree::Direction::Output;
		added.kind = TcIoTree::Kind::Digital;
		update.channels.append(added);

		QUndoCommand *command = TcSyncCommand::fromUpdates(
			cards.elements, {update});
		QVERIFY(command);
		auto *stack = project.undoStack();
		const int before = stack->count();
		stack->push(command);
		QCOMPARE(stack->count(), before + 1);

		auto ios = master->elementData().plcMasterData().ios;
		QCOMPARE(ios.size(), 3);
		QCOMPARE(ios[0].functionText, QStringLiteral("NEW"));
		QCOMPARE(ios[0].comment, QStringLiteral("from TwinCAT"));
		QCOMPARE(ios[0].terminalCount, 3);   //other values are kept
		QCOMPARE(ios[1].functionText, QStringLiteral("SAME"));
		QCOMPARE(ios[2].functionText, QStringLiteral("ADDED"));
		QCOMPARE(ios[2].type, ElementData::SortieDigitale);

		stack->undo();
		ios = master->elementData().plcMasterData().ios;
		QCOMPARE(ios.size(), 2);
		QCOMPARE(ios[0].functionText, QStringLiteral("OLD"));
		QCOMPARE(ios[0].comment, QStringLiteral("keep me"));
		stack->redo();
		QCOMPARE(master->elementData().plcMasterData().ios.size(), 3);
	}

	void nothingToChangeNoCommand()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		Element *master = card(folio);
		QVERIFY(master);
		ElementData data = master->elementData();
		auto plc = data.plcMasterData();
		plc.ios = {row(QStringLiteral("X01(1)"), QStringLiteral("A"))};
		data.setPlcMasterData(plc);
		master->setElementData(data);

		const auto cards = TcQetBridge::collect(&project);
		TcSyncCardUpdate update;
		update.card = 0;
		update.channels = cards.cards.first().channels;
		QVERIFY(!TcSyncCommand::fromUpdates(cards.elements, {update}));
	}
};

QET_APP_TEST_MAIN(tst_twincat_app)
#include "tst_twincat_app.moc"
