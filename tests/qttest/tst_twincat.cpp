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
#include "twincat/tciosync.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

/**
 * Tests of the TwinCAT I/O name synchronisation. The projects are built here,
 * small and made up: no real project is needed, and TwinCAT is never run.
 */
class TstTwincat : public QObject
{
	Q_OBJECT

	private slots:
		void scannerKeepsOffsets();
		void treeIsRead();
		void treeIsReadFromFolderWithXti();
		void renamesChangeOnlyNamesAndReferences();
		void backupAndEncodingAreKept();
		void commentInsertAndRemove();
		void refusals();
		void fileChangedSinceReadIsRefused();
		void matchingAndDifferences();
		void plansFollowTheChosenDirection();

	private:
		static QString sample();
		static QByteArray bytes(const QString &text);
		static QString read(const QString &path);
		static void write(const QString &path, const QByteArray &data);
		static int box(const TcIoTree &t, const QString &tag);
		static int channel(const TcIoTree &t, const QString &tag, const QString &name);
};

QString TstTwincat::sample()
{
	QString s = QStringLiteral(
R"XML(<?xml version="1.0"?>
<TcSmProject xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" TcSmVersion="1.0" TcVersion="3.1.4024.67">
	<Project ProjectGUID="{AAAAAAAA-0000-0000-0000-000000000001}">
		<System>
			<Settings MaxCpus="1"/>
		</System>
		<Io>
			<Device DevType="111" Id="1">
				<Name>Device 1 (EtherCAT)</Name>
				<Box Id="1">
					<Name>Coupler (EK1100)</Name>
					<ImageId>1</ImageId>
					<EtherCAT PhysAddr="1001" ProductRevision="#x00120000"/>
					<Box Id="2">
						<Name>Term 2 (EL1004)</Name>
						<ImageId>2</ImageId>
						<EtherCAT PhysAddr="1002" ProductRevision="#x00100000">
							<Pdo Name="Channel 1" Index="#x1a00" Flags="#x0008" SyncMan="3">
								<Entry Name="Input" Index="#x6000" Sub="1" BitLen="1">
									<Type>BIT</Type>
									<Comment><![CDATA[Old comment]]></Comment>
								</Entry>
							</Pdo>
							<Pdo Name="Channel 2" Index="#x1a01" Flags="#x0008" SyncMan="3">
								<Entry Name="Input" Index="#x6010" Sub="1" BitLen="1">
									<Type>BIT</Type>
								</Entry>
							</Pdo>
						</EtherCAT>
						<Xml>
							<TcDMDrive>
								<TcLink>
									<IoItemPathName>TIID^Device 1 (EtherCAT)^Coupler (EK1100)^Term 2 (EL1004)^Channel 1^Input</IoItemPathName>
								</TcLink>
							</TcDMDrive>
						</Xml>
					</Box>
					<Box Id="3">
						<Name>Term 3 (EL2004)</Name>
						<ImageId>3</ImageId>
						<EtherCAT PhysAddr="1003">
							<Pdo Name="Channel 1" Index="#x1600" Flags="#x0008" SyncMan="2">
								<Entry Name="Output" Index="#x7000" Sub="1" BitLen="1">
									<Type>BIT</Type>
								</Entry>
							</Pdo>
							<Pdo Name="Channel 2" Index="#x1601" Flags="#x0008" SyncMan="2">
								<Entry Name="Output" Index="#x7010" Sub="1" BitLen="1">
									<Type>BIT</Type>
								</Entry>
							</Pdo>
						</EtherCAT>
					</Box>
					<Box Id="4">
						<Name>Term 4 (EL3002)</Name>
						<ImageId>4</ImageId>
						<EtherCAT PhysAddr="1004">
							<Pdo Name="AI Standard Channel 1" Index="#x1a00" Flags="#x0008" SyncMan="3">
								<Entry Name="Status" Index="#x6000" Sub="1" BitLen="16">
									<Type>UINT</Type>
								</Entry>
								<Entry Name="Value" Index="#x6000" Sub="17" BitLen="16">
									<Type>INT</Type>
								</Entry>
							</Pdo>
						</EtherCAT>
					</Box>
				</Box>
			</Device>
		</Io>
		<Plc>
			<Project GUID="{BBBBBBBB-0000-0000-0000-000000000002}">
				<Name>Channel 1</Name>
			</Project>
		</Plc>
		<Mappings>
			<OwnerA Name="TIID^Device 1 (EtherCAT)^Coupler (EK1100)^Term 3 (EL2004)">
				<OwnerB Name="TIPC^Plc^Plc Instance">
					<Link VarA="Channel 1^Output" VarB="MAIN.bValve"/>
				</OwnerB>
			</OwnerA>
			<OwnerA Name="TIPC^Plc^Plc Instance">
				<OwnerB Name="TIID^Device 1 (EtherCAT)^Coupler (EK1100)^Term 2 (EL1004)">
					<Link VarA="MAIN.bStart" VarB="Channel 1^Input"/>
				</OwnerB>
			</OwnerA>
		</Mappings>
	</Project>
</TcSmProject>
)XML");
	return s.remove(QLatin1Char('\r')).replace(QLatin1Char('\n'), QStringLiteral("\r\n"));
}

QByteArray TstTwincat::bytes(const QString &text)
{
	return text.toUtf8();
}

QString TstTwincat::read(const QString &path)
{
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly)) return QString();
	return QString::fromUtf8(f.readAll());
}

void TstTwincat::write(const QString &path, const QByteArray &data)
{
	QDir().mkpath(QFileInfo(path).absolutePath());
	QFile f(path);
	QVERIFY(f.open(QIODevice::WriteOnly));
	f.write(data);
}

int TstTwincat::box(const TcIoTree &t, const QString &tag)
{
	for (int i = 0; i < t.boxes.size(); ++i) {
		if (t.boxes.at(i).tag == tag) return i;
	}
	return -1;
}

int TstTwincat::channel(const TcIoTree &t, const QString &tag, const QString &name)
{
	const int b = box(t, tag);
	if (b < 0) return -1;
	for (int c : t.boxes.at(b).channels) {
		if (t.channels.at(c).name == name) return c;
	}
	return -1;
}

void TstTwincat::scannerKeepsOffsets()
{
	const QString text = sample();
	TcXmlScanner xml;
	QVERIFY2(xml.parse(text), qPrintable(xml.errorString()));
	QCOMPARE(xml.node(0).name, QStringLiteral("TcSmProject"));
	// Every attribute value is found at its offsets in the original text
	for (int i = 0; i < xml.count(); ++i) {
		for (const auto &attr : xml.node(i).attributes) {
			QCOMPARE(text.mid(attr.rawBegin, attr.rawEnd - attr.rawBegin),
					 TcXmlScanner::escapeAttribute(attr.value));
		}
	}
	TcXmlScanner broken;
	QVERIFY(!broken.parse(QStringLiteral("<a><b></a></b>")));
	QCOMPARE(TcXmlScanner::decodeEntities(QStringLiteral("a&amp;b&lt;")),
			 QStringLiteral("a&b<"));
}

void TstTwincat::treeIsRead()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString path = dir.filePath(QStringLiteral("p.tsproj"));
	write(path, bytes(sample()));

	TcIoTree tree;
	QString error;
	QVERIFY2(TcIoTree::load(path, tree, &error), qPrintable(error));
	QCOMPARE(tree.devices, QStringList{QStringLiteral("Device 1 (EtherCAT)")});
	QCOMPARE(tree.boxes.size(), 4);

	const int term2 = box(tree, QStringLiteral("Term 2"));
	QVERIFY(term2 >= 0);
	QCOMPARE(tree.boxes.at(term2).type, QStringLiteral("EL1004"));
	QCOMPARE(tree.boxes.at(term2).channels.size(), 2);

	const int c1 = channel(tree, QStringLiteral("Term 2"), QStringLiteral("Channel 1"));
	QVERIFY(c1 >= 0);
	QCOMPARE(tree.channels.at(c1).direction, TcIoTree::Direction::Input);
	QCOMPARE(tree.channels.at(c1).kind, TcIoTree::Kind::Digital);
	QCOMPARE(tree.channels.at(c1).ordinal, 1);
	QCOMPARE(tree.channels.at(c1).comment, QStringLiteral("Old comment"));
	const int c2 = channel(tree, QStringLiteral("Term 2"), QStringLiteral("Channel 2"));
	QCOMPARE(tree.channels.at(c2).ordinal, 2);
	QVERIFY(tree.channels.at(c2).comment.isEmpty());

	const int out = channel(tree, QStringLiteral("Term 3"), QStringLiteral("Channel 2"));
	QCOMPARE(tree.channels.at(out).direction, TcIoTree::Direction::Output);

	const int ai = channel(tree, QStringLiteral("Term 4"),
						   QStringLiteral("AI Standard Channel 1"));
	QCOMPARE(tree.channels.at(ai).kind, TcIoTree::Kind::Analog);
	QCOMPARE(tree.channels.at(ai).direction, TcIoTree::Direction::Input);

	QCOMPARE(tree.channelPath(c1),
			 (QStringList{QStringLiteral("TIID"), QStringLiteral("Device 1 (EtherCAT)"),
						  QStringLiteral("Coupler (EK1100)"),
						  QStringLiteral("Term 2 (EL1004)"),
						  QStringLiteral("Channel 1")}));

	QCOMPARE(TcIoTree::splitBoxName(QStringLiteral("A1.02 (EL1004)")),
			 (QStringList{QStringLiteral("A1.02"), QStringLiteral("EL1004")}));
	QCOMPARE(TcIoTree::splitBoxName(QStringLiteral("Plain"))[1], QString());

	// A project that is not valid XML, or without I/O, is reported
	write(dir.filePath(QStringLiteral("bad.tsproj")), "<a><b></a>");
	QVERIFY(!TcIoTree::load(dir.filePath(QStringLiteral("bad.tsproj")), tree, &error));
	QVERIFY(!error.isEmpty());
}

void TstTwincat::treeIsReadFromFolderWithXti()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	QString main = sample();
	// The first device is stored apart, in an .xti next to the project
	const int begin = main.indexOf(QStringLiteral("\t\t\t<Device"));
	const int end = main.indexOf(QStringLiteral("</Device>")) + 9;
	const QString device = main.mid(begin, end - begin);
	main.remove(begin, end - begin);
	write(dir.filePath(QStringLiteral("p.tsproj")), bytes(main));
	write(dir.filePath(QStringLiteral("sub/dev.xti")),
		  bytes(QStringLiteral("<?xml version=\"1.0\"?>\r\n<TcSmItem>\r\n")
				+ device + QStringLiteral("\r\n</TcSmItem>\r\n")));

	TcIoTree tree;
	QString error;
	QVERIFY2(TcIoTree::load(dir.path(), tree, &error), qPrintable(error));
	QCOMPARE(tree.files.size(), 2);
	QCOMPARE(tree.boxes.size(), 4);

	// Renaming through the .xti also updates the Mappings of the project
	TcIoWriter::Plan plan;
	plan.boxRenames.append({box(tree, QStringLiteral("Term 2")), QStringLiteral("A1.02")});
	const auto result = TcIoWriter::apply(tree, plan);
	QVERIFY2(result.ok, qPrintable(result.error));
	QCOMPARE(result.filesWritten.size(), 2);
	QVERIFY(read(dir.filePath(QStringLiteral("p.tsproj")))
			.contains(QStringLiteral("TIID^Device 1 (EtherCAT)^Coupler (EK1100)^A1.02 (EL1004)")));
}

void TstTwincat::renamesChangeOnlyNamesAndReferences()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString path = dir.filePath(QStringLiteral("p.tsproj"));
	const QString original = sample();
	write(path, bytes(original));

	TcIoTree tree;
	QVERIFY(TcIoTree::load(path, tree));

	TcIoWriter::Plan plan;
	plan.boxRenames.append({box(tree, QStringLiteral("Term 2")), QStringLiteral("A1.02")});
	TcIoWriter::ChannelEdit edit;
	edit.channel = channel(tree, QStringLiteral("Term 2"), QStringLiteral("Channel 1"));
	edit.setName = true;
	edit.name = QStringLiteral("Start pump");
	edit.setComment = true;
	edit.comment = QStringLiteral("Start & <pump>");
	plan.channelEdits.append(edit);

	const auto preview = TcIoWriter::preview(tree, plan);
	QVERIFY2(preview.ok, qPrintable(preview.error));
	QVERIFY(preview.filesWritten.isEmpty());
	QCOMPARE(read(path), original);   // a preview writes nothing

	const QDateTime now(QDate(2026, 5, 1), QTime(10, 20, 30));
	const auto result = TcIoWriter::apply(tree, plan, now);
	QVERIFY2(result.ok, qPrintable(result.error));
	QVERIFY(result.referenceUpdates >= 3);

	// What the file must be: the original with these replacements, and
	// nothing else, byte for byte (line ends included)
	QString expected = original;
	expected.replace(QStringLiteral("Term 2 (EL1004)^Channel 1^"),
					 QStringLiteral("Term 2 (EL1004)^Start pump^"));
	expected.replace(QStringLiteral("VarB=\"Channel 1^Input\""),
					 QStringLiteral("VarB=\"Start pump^Input\""));
	expected.replace(QStringLiteral("Name=\"Channel 1\" Index=\"#x1a00\""),
					 QStringLiteral("Name=\"Start pump\" Index=\"#x1a00\""));
	expected.replace(QStringLiteral("<![CDATA[Old comment]]>"),
					 QStringLiteral("<![CDATA[Start & <pump>]]>"));
	expected.replace(QStringLiteral("Term 2 (EL1004)"), QStringLiteral("A1.02 (EL1004)"));
	QCOMPARE(read(path), expected);

	// The other "Channel 1" (another box, and the PLC project name) are untouched
	QVERIFY(expected.contains(QStringLiteral("<Name>Channel 1</Name>")));
	QVERIFY(expected.contains(QStringLiteral("VarA=\"Channel 1^Output\"")));

	TcIoTree again;
	QVERIFY(TcIoTree::load(path, again));
	QCOMPARE(again.boxes.size(), tree.boxes.size());
	QCOMPARE(again.channels.size(), tree.channels.size());
	for (int i = 0; i < tree.channels.size(); ++i) {
		QCOMPARE(again.channels.at(i).ordinal, tree.channels.at(i).ordinal);
		QCOMPARE(again.channels.at(i).direction, tree.channels.at(i).direction);
		QCOMPARE(again.channels.at(i).kind, tree.channels.at(i).kind);
		QCOMPARE(again.channels.at(i).pdoIndex, tree.channels.at(i).pdoIndex);
	}
	const int renamed = channel(again, QStringLiteral("A1.02"), QStringLiteral("Start pump"));
	QVERIFY(renamed >= 0);
	QCOMPARE(again.channels.at(renamed).comment, QStringLiteral("Start & <pump>"));
}

void TstTwincat::backupAndEncodingAreKept()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString path = dir.filePath(QStringLiteral("p.tsproj"));
	QByteArray original = QByteArray("\xEF\xBB\xBF") + bytes(sample());
	write(path, original);

	TcIoTree tree;
	QVERIFY(TcIoTree::load(path, tree));
	QVERIFY(tree.files.first().hasBom);

	TcIoWriter::Plan plan;
	plan.boxRenames.append({box(tree, QStringLiteral("Term 3")), QStringLiteral("A1.03")});
	const QDateTime now(QDate(2026, 5, 1), QTime(10, 20, 30));
	const auto result = TcIoWriter::apply(tree, plan, now);
	QVERIFY2(result.ok, qPrintable(result.error));

	QCOMPARE(result.backups.size(), 1);
	QVERIFY(result.backups.first().endsWith(
		QStringLiteral("p.tsproj.20260501-102030.bak")));
	QFile backup(result.backups.first());
	QVERIFY(backup.open(QIODevice::ReadOnly));
	QCOMPARE(backup.readAll(), original);
	backup.close();

	QFile f(path);
	QVERIFY(f.open(QIODevice::ReadOnly));
	const QByteArray written = f.readAll();
	f.close();
	QVERIFY(written.startsWith("\xEF\xBB\xBF"));
	QVERIFY(!written.startsWith("\xEF\xBB\xBF\xEF\xBB\xBF"));
	QVERIFY(written.contains("<Name>A1.03 (EL2004)</Name>"));

	// A second write in the same second does not overwrite the first backup
	TcIoTree second;
	QVERIFY(TcIoTree::load(path, second));
	TcIoWriter::Plan plan2;
	plan2.boxRenames.append({box(second, QStringLiteral("A1.03")), QStringLiteral("A1.04")});
	const auto result2 = TcIoWriter::apply(second, plan2, now);
	QVERIFY2(result2.ok, qPrintable(result2.error));
	QVERIFY(result2.backups.first() != result.backups.first());
	QVERIFY(QFileInfo::exists(result.backups.first()));
}

void TstTwincat::commentInsertAndRemove()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString path = dir.filePath(QStringLiteral("p.tsproj"));
	const QString original = sample();
	write(path, bytes(original));

	TcIoTree tree;
	QVERIFY(TcIoTree::load(path, tree));

	TcIoWriter::Plan plan;
	TcIoWriter::ChannelEdit remove;
	remove.channel = channel(tree, QStringLiteral("Term 2"), QStringLiteral("Channel 1"));
	remove.setComment = true;   // empty: the Comment element goes away
	plan.channelEdits.append(remove);
	TcIoWriter::ChannelEdit insert;
	insert.channel = channel(tree, QStringLiteral("Term 2"), QStringLiteral("Channel 2"));
	insert.setComment = true;
	insert.comment = QStringLiteral("Second input");
	plan.channelEdits.append(insert);
	TcIoWriter::ChannelEdit analog;
	analog.channel = channel(tree, QStringLiteral("Term 4"),
							 QStringLiteral("AI Standard Channel 1"));
	analog.setComment = true;
	analog.comment = QStringLiteral("Level");
	plan.channelEdits.append(analog);

	const auto result = TcIoWriter::apply(tree, plan);
	QVERIFY2(result.ok, qPrintable(result.error));

	TcIoTree again;
	QVERIFY(TcIoTree::load(path, again));
	QVERIFY(again.channels.at(channel(again, QStringLiteral("Term 2"),
									  QStringLiteral("Channel 1"))).comment.isEmpty());
	QCOMPARE(again.channels.at(channel(again, QStringLiteral("Term 2"),
									   QStringLiteral("Channel 2"))).comment,
			 QStringLiteral("Second input"));
	// The comment of an analog channel goes on the Value entry
	QCOMPARE(again.channels.at(channel(again, QStringLiteral("Term 4"),
									   QStringLiteral("AI Standard Channel 1"))).comment,
			 QStringLiteral("Level"));

	// Same elements everywhere but Comment: nothing was added or removed
	TcXmlScanner before;
	TcXmlScanner after;
	QVERIFY(before.parse(original));
	QVERIFY(after.parse(read(path)));
	QCOMPARE(after.structureSignature(QStringLiteral("Comment")),
			 before.structureSignature(QStringLiteral("Comment")));
	QVERIFY(!read(path).contains(QStringLiteral("Old comment")));
	QVERIFY(read(path).contains(QStringLiteral("\r\n")));
	QVERIFY(!read(path).contains(QRegularExpression(QStringLiteral("[^\r]\n"))));
}

void TstTwincat::refusals()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString path = dir.filePath(QStringLiteral("p.tsproj"));
	const QString original = sample();
	write(path, bytes(original));

	TcIoTree tree;
	QVERIFY(TcIoTree::load(path, tree));

	QVERIFY(!TcIoWriter::validateName(QString()).isEmpty());
	QVERIFY(!TcIoWriter::validateName(QStringLiteral("a^b")).isEmpty());
	QVERIFY(!TcIoWriter::validateName(QStringLiteral("a\tb")).isEmpty());
	QVERIFY(TcIoWriter::validateName(QStringLiteral("Start pump")).isEmpty());

	auto tryName = [&](const QString &tag, const QString &old, const QString &name) {
		TcIoWriter::Plan plan;
		TcIoWriter::ChannelEdit edit;
		edit.channel = channel(tree, tag, old);
		edit.setName = true;
		edit.name = name;
		plan.channelEdits.append(edit);
		return TcIoWriter::apply(tree, plan);
	};
	QVERIFY(!tryName(QStringLiteral("Term 2"), QStringLiteral("Channel 1"),
					 QStringLiteral("bad^name")).ok);
	// Two channels of a box cannot share a name
	QVERIFY(!tryName(QStringLiteral("Term 2"), QStringLiteral("Channel 1"),
					 QStringLiteral("channel 2")).ok);
	QCOMPARE(read(path), original);
	QVERIFY(QDir(dir.path()).entryList({QStringLiteral("*.bak")}).isEmpty());

	// Swapping the names of two channels is a rename of each: it works
	// and the references follow, since every path is computed from the
	// names read, not from the names being written.
	TcIoWriter::Plan swap;
	TcIoWriter::ChannelEdit a;
	a.channel = channel(tree, QStringLiteral("Term 2"), QStringLiteral("Channel 1"));
	a.setName = true;
	a.name = QStringLiteral("Channel 2");
	TcIoWriter::ChannelEdit b;
	b.channel = channel(tree, QStringLiteral("Term 2"), QStringLiteral("Channel 2"));
	b.setName = true;
	b.name = QStringLiteral("Channel 1");
	swap.channelEdits << a << b;
	const auto preview = TcIoWriter::preview(tree, swap);
	QVERIFY2(preview.ok, qPrintable(preview.error));
}

void TstTwincat::fileChangedSinceReadIsRefused()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString path = dir.filePath(QStringLiteral("p.tsproj"));
	write(path, bytes(sample()));

	TcIoTree tree;
	QVERIFY(TcIoTree::load(path, tree));

	// Someone (XAE, for instance) saved the project after it was read
	const QString changed = sample().replace(QStringLiteral("PhysAddr=\"1002\""),
											 QStringLiteral("PhysAddr=\"1012\""));
	write(path, bytes(changed));

	TcIoWriter::Plan plan;
	plan.boxRenames.append({box(tree, QStringLiteral("Term 2")), QStringLiteral("A1.02")});
	const auto result = TcIoWriter::apply(tree, plan);
	QVERIFY(!result.ok);
	QVERIFY(!result.error.isEmpty());
	QCOMPARE(read(path), changed);
	QVERIFY(QDir(dir.path()).entryList({QStringLiteral("*.bak")}).isEmpty());
}

static TcSyncQetChannel qetRow(const QString &address, const QString &name,
							   const QString &comment,
							   TcIoTree::Direction dir = TcIoTree::Direction::Input,
							   TcIoTree::Kind kind = TcIoTree::Kind::Digital)
{
	TcSyncQetChannel c;
	c.address = address;
	c.name = name;
	c.comment = comment;
	c.direction = dir;
	c.kind = kind;
	return c;
}

void TstTwincat::matchingAndDifferences()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString path = dir.filePath(QStringLiteral("p.tsproj"));
	write(path, bytes(sample()));
	TcIoTree tree;
	QVERIFY(TcIoTree::load(path, tree));

	QCOMPARE(TcIoSync::channelNumber(QStringLiteral("X01(3)")), 3);
	QCOMPARE(TcIoSync::channelNumber(QStringLiteral("12")), 12);
	QCOMPARE(TcIoSync::channelNumber(QStringLiteral("X")), 0);
	QVERIFY(TcIoSync::isDefaultName(QStringLiteral("Channel 1")));
	QVERIFY(TcIoSync::isDefaultName(QStringLiteral("AI Standard Channel 2")));
	QVERIFY(!TcIoSync::isDefaultName(QStringLiteral("Start pump")));

	TcSyncQetCard card;
	card.id = QStringLiteral("c1");
	card.tag = QStringLiteral("Term 2");
	card.channels << qetRow(QStringLiteral("X01(1)"), QStringLiteral("Start pump"),
							QStringLiteral("Old comment"))
				  << qetRow(QStringLiteral("X01(2)"), QStringLiteral("Stop"), QString())
				  << qetRow(QStringLiteral("X01(3)"), QStringLiteral("Spare"), QString())
				  // digital in QET, analog in TwinCAT: no such input on EL1004
				  ;
	TcSyncQetCard other;
	other.id = QStringLiteral("c2");
	other.tag = QStringLiteral("A9.99");

	auto kinds = [](const QVector<TcSyncItem> &items) {
		QList<TcSyncItem::Kind> k;
		for (const auto &i : items) k << i.kind;
		return k;
	};
	auto count = [&](const QVector<TcSyncItem> &items, TcSyncItem::Kind kind) {
		return kinds(items).count(kind);
	};

	auto items = TcIoSync::compare({card, other}, tree);
	QCOMPARE(count(items, TcSyncItem::Kind::NameDiff), 2);   // channels 1 and 2
	QCOMPARE(count(items, TcSyncItem::Kind::CommentDiff), 0 + 0);
	QCOMPARE(count(items, TcSyncItem::Kind::ChannelOnlyInQet), 1);   // X01(3)
	QCOMPARE(count(items, TcSyncItem::Kind::CardOnlyInQet), 1);       // A9.99
	QCOMPARE(count(items, TcSyncItem::Kind::BoxOnlyInTwincat), 2);    // Term 3, Term 4
	QCOMPARE(count(items, TcSyncItem::Kind::BoxRename), 0);

	for (const auto &item : items) {
		if (item.kind == TcSyncItem::Kind::NameDiff && item.qetValue == QStringLiteral("Start pump")) {
			// TwinCAT still has its default name: it cannot overwrite QET
			QVERIFY(item.canQetToTwincat);
			QVERIFY(!item.canTwincatToQet);
		}
	}

	// Card "A1.02" of QET paired by hand with the box "Term 2" of TwinCAT
	card.tag = QStringLiteral("A1.02");
	items = TcIoSync::compare({card, other}, tree,
							  {{QStringLiteral("c1"), box(tree, QStringLiteral("Term 2"))}});
	QCOMPARE(count(items, TcSyncItem::Kind::BoxRename), 1);
	QCOMPARE(count(items, TcSyncItem::Kind::CardOnlyInQet), 1);
	QCOMPARE(count(items, TcSyncItem::Kind::BoxOnlyInTwincat), 2);

	// A digital row against an analog channel, and a channel only in TwinCAT
	TcSyncQetCard analog;
	analog.id = QStringLiteral("c3");
	analog.tag = QStringLiteral("term 4");
	analog.channels << qetRow(QStringLiteral("1"), QStringLiteral("Level"), QString(),
							  TcIoTree::Direction::Input, TcIoTree::Kind::Digital);
	items = TcIoSync::compare({analog}, tree);
	QCOMPARE(count(items, TcSyncItem::Kind::TypeMismatch), 1);

	TcSyncQetCard missing;
	missing.id = QStringLiteral("c4");
	missing.tag = QStringLiteral("Term 3");
	items = TcIoSync::compare({missing}, tree);
	QCOMPARE(count(items, TcSyncItem::Kind::ChannelOnlyInTwincat), 2);

	QVERIFY(TcIoSync::reportCsv(items).startsWith(QStringLiteral("kind;label;qet;twincat")));
}

void TstTwincat::plansFollowTheChosenDirection()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString path = dir.filePath(QStringLiteral("p.tsproj"));
	write(path, bytes(sample()));
	TcIoTree tree;
	QVERIFY(TcIoTree::load(path, tree));

	TcSyncQetCard card;
	card.id = QStringLiteral("c1");
	card.tag = QStringLiteral("A1.02");
	card.channels << qetRow(QStringLiteral("X01(1)"), QStringLiteral("Start pump"),
							QStringLiteral("Pushbutton"))
				  << qetRow(QStringLiteral("X01(2)"), QStringLiteral("Stop"),
							QStringLiteral("Old comment"));
	const QVector<TcSyncQetCard> cards{card};
	auto items = TcIoSync::compare(cards, tree,
								   {{QStringLiteral("c1"), box(tree, QStringLiteral("Term 2"))}});

	// Nothing is chosen: nothing is planned
	QVERIFY(TcIoSync::twincatPlan(items, cards).isEmpty());
	QVERIFY(TcIoSync::qetUpdates(items, cards, tree).isEmpty());

	for (auto &item : items) {
		if (item.canQetToTwincat) item.direction = TcSyncItem::Direction::QetToTwincat;
	}
	const auto plan = TcIoSync::twincatPlan(items, cards);
	QCOMPARE(plan.boxRenames.size(), 1);
	QCOMPARE(plan.boxRenames.first().newTag, QStringLiteral("A1.02"));
	QCOMPARE(plan.channelEdits.size(), 2);
	QVERIFY(TcIoSync::qetUpdates(items, cards, tree).isEmpty());

	const auto result = TcIoWriter::apply(tree, plan);
	QVERIFY2(result.ok, qPrintable(result.error));
	TcIoTree again;
	QVERIFY(TcIoTree::load(path, again));
	QVERIFY(channel(again, QStringLiteral("A1.02"), QStringLiteral("Start pump")) >= 0);
	QVERIFY(channel(again, QStringLiteral("A1.02"), QStringLiteral("Stop")) >= 0);

	// The other way: TwinCAT (now named) to a QET card still empty
	TcSyncQetCard empty;
	empty.id = QStringLiteral("c1");
	empty.tag = QStringLiteral("A1.02");
	empty.channels << qetRow(QStringLiteral("X01(1)"), QString(), QString());
	const QVector<TcSyncQetCard> cards2{empty};
	auto items2 = TcIoSync::compare(cards2, again);
	for (auto &item : items2) {
		if (item.canTwincatToQet) item.direction = TcSyncItem::Direction::TwincatToQet;
	}
	const auto updates = TcIoSync::qetUpdates(items2, cards2, again);
	QCOMPARE(updates.size(), 1);
	// The row 1 is filled, the row 2 is created
	QCOMPARE(updates.first().channels.size(), 2);
	QCOMPARE(updates.first().channels.at(0).name, QStringLiteral("Start pump"));
	QCOMPARE(updates.first().channels.at(0).comment, QStringLiteral("Pushbutton"));
	QCOMPARE(updates.first().channels.at(1).name, QStringLiteral("Stop"));
	QCOMPARE(updates.first().channels.at(1).address, QStringLiteral("2"));
	QVERIFY(TcIoSync::twincatPlan(items2, cards2).isEmpty());
}

QTEST_APPLESS_MAIN(TstTwincat)
#include "tst_twincat.moc"
