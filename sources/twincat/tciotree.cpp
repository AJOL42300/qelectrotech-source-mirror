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
#include "tciotree.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QStringEncoder>

namespace {
const char *rootPrefix = "TIID";

QVector<int> findDevices(const TcXmlScanner &xml)
{
	QVector<int> result;
	for (int i = 0; i < xml.count(); ++i) {
		const auto &node = xml.node(i);
		if (node.name != QLatin1String("Device") || node.parent < 0) continue;
		const QString parent = xml.node(node.parent).name;
		if (parent == QLatin1String("Io")
			|| (parent == QLatin1String("TcSmItem")
				&& node.parent == xml.root())) {
			result.append(i);
		}
	}
	return result;
}

QString nodeText(const TcXmlScanner &xml, int node, const QString &child)
{
	const int c = xml.child(node, child);
	return c < 0 ? QString() : xml.text(c).trimmed();
}

/// "#x1a00", "0x1A00" or "1A00" -> value, -1 if unreadable
int parseIndex(const QString &text)
{
	QString t = text.trimmed();
	if (t.startsWith(QLatin1String("#x"), Qt::CaseInsensitive)
		|| t.startsWith(QLatin1String("0x"), Qt::CaseInsensitive)) {
		t = t.mid(2);
	}
	bool ok = false;
	const int v = t.toInt(&ok, 16);
	return ok ? v : -1;
}

bool isBitType(const QString &type)
{
	const QString t = type.trimmed().toUpper();
	return t == QLatin1String("BIT") || t == QLatin1String("BOOL");
}

bool excludedDir(const QString &relative)
{
	static const QStringList excluded{
		QStringLiteral("_Boot"), QStringLiteral("_Repository"),
		QStringLiteral("_CompileInfo"), QStringLiteral("_Libraries"),
		QStringLiteral(".git"), QStringLiteral(".vs"),
		QStringLiteral("bin"), QStringLiteral("obj")};
	const QStringList parts = relative.split(QLatin1Char('/'));
	for (int i = 0; i < parts.size() - 1; ++i) {
		if (excluded.contains(parts.at(i), Qt::CaseInsensitive)) return true;
	}
	return false;
}
}

/**
 * @brief TcIoTree::resolveProjectFile
 * @param path A .tsproj, an .xti, a .sln or a folder.
 * @return The .tsproj / .xti to read, or an empty string.
 */
QString TcIoTree::resolveProjectFile(const QString &path)
{
	const QFileInfo info(path);
	if (info.isFile()) {
		const QString suffix = info.suffix().toLower();
		if (suffix == QLatin1String("sln")) {
			QFile file(path);
			if (!file.open(QIODevice::ReadOnly)) return QString();
			const QString text = QString::fromUtf8(file.readAll());
			const QRegularExpression re(
				QStringLiteral("\"([^\"]+\\.tsproj)\""),
				QRegularExpression::CaseInsensitiveOption);
			const auto match = re.match(text);
			if (!match.hasMatch()) return QString();
			const QString rel = QString(match.captured(1))
				.replace(QLatin1Char('\\'), QLatin1Char('/'));
			const QString full = QDir(info.absolutePath()).absoluteFilePath(rel);
			return QFileInfo::exists(full) ? QDir::cleanPath(full) : QString();
		}
		return info.absoluteFilePath();
	}
	if (info.isDir()) {
		const QDir dir(path);
		const QStringList tsprojs = dir.entryList(
			{QStringLiteral("*.tsproj")}, QDir::Files, QDir::Name);
		if (!tsprojs.isEmpty()) return dir.absoluteFilePath(tsprojs.first());
		const QStringList slns = dir.entryList(
			{QStringLiteral("*.sln")}, QDir::Files, QDir::Name);
		if (!slns.isEmpty()) return resolveProjectFile(dir.absoluteFilePath(slns.first()));
		QDirIterator it(path, {QStringLiteral("*.tsproj")}, QDir::Files,
						QDirIterator::Subdirectories);
		while (it.hasNext()) {
			const QString found = it.next();
			if (!excludedDir(dir.relativeFilePath(found))) return found;
		}
	}
	return QString();
}

/**
 * @brief TcIoTree::readFile
 * Reads a file as UTF-8 (a byte-order mark is remembered), refusing
 * anything that could not be written back identically.
 */
bool TcIoTree::readFile(const QString &path, File &file, QString *error)
{
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly)) {
		if (error) *error = QStringLiteral("Cannot open %1: %2").arg(path, f.errorString());
		return false;
	}
	const QByteArray bytes = f.readAll();
	f.close();

	QByteArray body = bytes;
	file.hasBom = body.startsWith("\xEF\xBB\xBF");
	if (file.hasBom) body.remove(0, 3);

	QStringDecoder decoder(QStringDecoder::Utf8, QStringDecoder::Flag::Stateless);
	const QString text = decoder(body);
	if (decoder.hasError()) {
		if (error) *error = QStringLiteral("%1 is not valid UTF-8").arg(path);
		return false;
	}
	file.path = path;
	file.text = text;
	file.hash = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
	if (!file.xml.parse(file.text)) {
		if (error) *error = QStringLiteral("%1: %2").arg(path, file.xml.errorString());
		return false;
	}
	return true;
}

QStringList TcIoTree::splitBoxName(const QString &name)
{
	static const QRegularExpression re(
		QStringLiteral("^(.*?)\\s*\\(([^()]*)\\)\\s*$"));
	const auto match = re.match(name);
	if (match.hasMatch()) {
		return {match.captured(1).trimmed(), match.captured(2).trimmed()};
	}
	return {name.trimmed(), QString()};
}

QStringList TcIoTree::splitPath(const QString &path)
{
	return path.split(QLatin1Char('^'));
}

QStringList TcIoTree::channelPath(int channel) const
{
	const Channel &ch = channels.at(channel);
	QStringList path = boxes.at(ch.box).path;
	path << ch.name;
	return path;
}

/**
 * @brief TcIoTree::load
 * @param project_path .tsproj, .xti, .sln or folder
 * @return false (and @p error) if nothing usable could be read.
 */
bool TcIoTree::load(const QString &project_path, TcIoTree &tree, QString *error)
{
	tree = TcIoTree();
	tree.projectFile = resolveProjectFile(project_path);
	if (tree.projectFile.isEmpty()) {
		if (error) *error = QStringLiteral("No TwinCAT project found at %1").arg(project_path);
		return false;
	}

	File first;
	QString err;
	if (!readFile(tree.projectFile, first, &err)) {
		if (error) *error = err;
		return false;
	}
	tree.files.append(std::move(first));
	tree.parseFile(0);

	const bool is_xti = tree.projectFile.endsWith(QLatin1String(".xti"),
												  Qt::CaseInsensitive);
	if (!is_xti) {
		const QString dir = QFileInfo(tree.projectFile).absolutePath();
		const QDir root(dir);
		QDirIterator it(dir, {QStringLiteral("*.xti")}, QDir::Files,
						QDirIterator::Subdirectories);
		while (it.hasNext()) {
			const QString path = it.next();
			if (excludedDir(root.relativeFilePath(path))) continue;
			File xti;
			if (!readFile(path, xti, &err)) {
				tree.warnings << err;
				continue;
			}
			bool has_new_device = false;
			for (int dev : findDevices(xti.xml)) {
				const QString name = nodeText(xti.xml, dev, QStringLiteral("Name"));
				if (!name.isEmpty() && !tree.devices.contains(name)) {
					has_new_device = true;
				}
			}
			if (has_new_device) {
				tree.files.append(std::move(xti));
				tree.parseFile(tree.files.size() - 1);
			} else {
				tree.referenceFiles << path;
			}
		}
	}

	if (tree.devices.isEmpty()) {
		if (error) *error = QStringLiteral("No I/O device found in %1").arg(tree.projectFile);
		return false;
	}
	return true;
}

/**
 * @brief TcIoTree::parseFile
 * Collects boxes and channels of the devices of one file.
 */
bool TcIoTree::parseFile(int file_index)
{
	const TcXmlScanner &xml = files.at(file_index).xml;
	bool found = false;

	struct Frame {int node; int boxIndex; QStringList path;};

	for (int device : findDevices(xml)) {
		const QString dev_name = nodeText(xml, device, QStringLiteral("Name"));
		if (dev_name.isEmpty() || devices.contains(dev_name)) continue;
		devices << dev_name;
		found = true;

		QVector<Frame> todo;
		todo.append({device, -1, {QString::fromLatin1(rootPrefix), dev_name}});
		while (!todo.isEmpty()) {
			const Frame frame = todo.takeLast();
			QVector<Frame> kids;
			for (int b : xml.childrenNamed(frame.node, QStringLiteral("Box"))) {
				Box box;
				box.file = file_index;
				box.parent = frame.boxIndex;
				box.nameNode = xml.child(b, QStringLiteral("Name"));
				box.name = box.nameNode < 0 ? QString() : xml.text(box.nameNode);
				const QStringList parts = splitBoxName(box.name);
				box.tag = parts.at(0);
				box.type = parts.at(1);
				box.path = frame.path;
				box.path << box.name;
				const int box_index = boxes.size();
				boxes.append(box);

				int ord_in = 0;
				int ord_out = 0;
				const int ecat = xml.child(b, QStringLiteral("EtherCAT"));
				const QVector<int> pdos = ecat < 0
					? QVector<int>()
					: xml.childrenNamed(ecat, QStringLiteral("Pdo"));
				for (int pdo : pdos) {
					Channel ch;
					ch.box = box_index;
					ch.file = file_index;
					ch.pdoNode = pdo;
					ch.name = xml.attribute(pdo, QStringLiteral("Name"));
					ch.pdoIndex = xml.attribute(pdo, QStringLiteral("Index"));

					const int index = parseIndex(ch.pdoIndex);
					if (index >= 0x1A00 && index < 0x1C00) {
						ch.direction = Direction::Input;
					} else if (index >= 0x1600 && index < 0x1800) {
						ch.direction = Direction::Output;
					} else if (ch.name.contains(QLatin1String("Output"), Qt::CaseInsensitive)
							   || ch.name.contains(QLatin1String("RxPDO"), Qt::CaseInsensitive)) {
						ch.direction = Direction::Output;
					} else if (ch.name.contains(QLatin1String("Input"), Qt::CaseInsensitive)
							   || ch.name.contains(QLatin1String("TxPDO"), Qt::CaseInsensitive)) {
						ch.direction = Direction::Input;
					}

					const QVector<int> entries = xml.childrenNamed(pdo, QStringLiteral("Entry"));
					int bit_entries = 0;
					int wide_entries = 0;
					for (int e : entries) {
						const QString type = nodeText(xml, e, QStringLiteral("Type"));
						const int bitlen = xml.attribute(e, QStringLiteral("BitLen")).toInt();
						if (isBitType(type) || (type.isEmpty() && bitlen == 1)) ++bit_entries;
						else ++wide_entries;
					}
					if (wide_entries > 0) ch.kind = Kind::Analog;
					else if (bit_entries == 1) ch.kind = Kind::Digital;

					if (!entries.isEmpty()) {
						ch.mainEntryNode = entries.last();
						if (entries.size() == 1) {
							ch.mainEntryNode = entries.first();
						} else {
							for (int e : entries) {
								const QString n = xml.attribute(e, QStringLiteral("Name"));
								if (n == QLatin1String("Value") || n == QLatin1String("Input")
									|| n == QLatin1String("Output")) {
									ch.mainEntryNode = e;
									break;
								}
							}
						}
						const int comment = xml.child(ch.mainEntryNode, QStringLiteral("Comment"));
						if (comment >= 0) ch.comment = xml.text(comment).trimmed();
					}

					if (ch.kind != Kind::Other) {
						if (ch.direction == Direction::Input) ch.ordinal = ++ord_in;
						else if (ch.direction == Direction::Output) ch.ordinal = ++ord_out;
					}
					boxes[box_index].channels.append(channels.size());
					channels.append(ch);
				}

				Frame sub;
				sub.node = b;
				sub.boxIndex = box_index;
				sub.path = boxes.at(box_index).path;
				kids.append(sub);
			}
			// Reverse so that boxes are visited in document order
			for (int i = kids.size() - 1; i >= 0; --i) todo.append(kids.at(i));
		}
	}
	return found;
}
