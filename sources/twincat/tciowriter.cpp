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
#include "tciowriter.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QSaveFile>
#include <algorithm>

namespace {
struct Edit {int begin; int end; QString text;};
using Kind = TcIoWriter::Change::Kind;

struct Prepared {
	QVector<TcIoTree::File> refs;
	QVector<const TcIoTree::File *> files;   ///< tree files, then refs
	QVector<QVector<Edit>> edits;
	QVector<QString> newTexts;
};

bool isWhite(QChar c)
{
	return c == QLatin1Char(' ') || c == QLatin1Char('\t')
		|| c == QLatin1Char('\r') || c == QLatin1Char('\n');
}

QStringList segments(const QString &path)
{
	return path.isEmpty() ? QStringList() : path.split(QLatin1Char('^'));
}

/// Applies the renames to the segments of owner^var. Prefixes are looked up
/// with the original names, so that nested renames do not disturb each other.
bool rewrite(QStringList &owner,
			 QStringList &var,
			 const QHash<QString, QString> &renames)
{
	QStringList all = owner + var;
	QStringList result = all;
	bool changed = false;
	QString prefix;
	for (int k = 0; k < all.size(); ++k) {
		prefix = k == 0 ? all.at(0)
						: prefix + QLatin1Char('^') + all.at(k);
		const auto it = renames.constFind(prefix);
		if (it != renames.constEnd()) {
			result[k] = it.value();
			changed = true;
		}
	}
	if (changed) {
		owner = result.mid(0, owner.size());
		var = result.mid(owner.size());
	}
	return changed;
}

QString renderComment(const QString &comment, bool cdata)
{
	if (cdata && !comment.contains(QLatin1String("]]>"))) {
		return QStringLiteral("<![CDATA[") + comment + QStringLiteral("]]>");
	}
	return TcXmlScanner::escapeText(comment);
}

bool fileUsesCData(const TcIoTree::File &file)
{
	const TcXmlScanner &xml = file.xml;
	for (int i = 0; i < xml.count(); ++i) {
		if (xml.node(i).name == QLatin1String("Comment") && xml.hasCData(i)) {
			return true;
		}
	}
	return false;
}

void addCommentEdit(const TcIoTree::File &file,
					int entry,
					const QString &comment,
					bool new_cdata,
					QVector<Edit> &edits)
{
	const TcXmlScanner &xml = file.xml;
	const QString &text = file.text;
	const int existing = xml.child(entry, QStringLiteral("Comment"));

	if (existing >= 0) {
		const auto &nd = xml.node(existing);
		if (comment.isEmpty()) {
			int begin = nd.startBegin;
			while (begin > 0 && (text.at(begin - 1) == QLatin1Char(' ')
								 || text.at(begin - 1) == QLatin1Char('\t'))) {
				--begin;
			}
			if (begin > 0 && text.at(begin - 1) == QLatin1Char('\n')) {
				--begin;
				if (begin > 0 && text.at(begin - 1) == QLatin1Char('\r')) --begin;
			}
			edits.append({begin, nd.endEnd, QString()});
		} else if (nd.selfClosing) {
			edits.append({nd.startBegin, nd.endEnd,
						  QStringLiteral("<Comment>") + renderComment(comment, new_cdata)
						  + QStringLiteral("</Comment>")});
		} else {
			edits.append({nd.contentBegin, nd.contentEnd,
						  renderComment(comment, xml.hasCData(existing))});
		}
		return;
	}
	if (comment.isEmpty()) return;

	const QString element = QStringLiteral("<Comment>")
		+ renderComment(comment, new_cdata) + QStringLiteral("</Comment>");
	const auto &entry_node = xml.node(entry);
	if (entry_node.selfClosing) {
		edits.append({entry_node.startEnd - 2, entry_node.startEnd,
					  QStringLiteral(">") + element + QStringLiteral("</")
					  + entry_node.name + QStringLiteral(">")});
		return;
	}
	if (entry_node.children.isEmpty()) {
		edits.append({entry_node.contentBegin, entry_node.contentBegin, element});
		return;
	}
	int prior = xml.child(entry, QStringLiteral("Type"));
	if (prior < 0) prior = entry_node.children.last();
	const auto &pn = xml.node(prior);
	int j = pn.startBegin;
	while (j > 0 && isWhite(text.at(j - 1))) --j;
	QString sep = text.mid(j, pn.startBegin - j);
	if (!sep.contains(QLatin1Char('\n'))) sep.clear();
	edits.append({pn.endEnd, pn.endEnd, sep + element});
}

bool applyEdits(const QString &text, QVector<Edit> edits, QString &out, QString &error)
{
	std::sort(edits.begin(), edits.end(),
			  [](const Edit &a, const Edit &b) {
				  return a.begin != b.begin ? a.begin < b.begin : a.end < b.end;
			  });
	for (int i = 1; i < edits.size(); ++i) {
		if (edits.at(i).begin < edits.at(i - 1).end) {
			error = QStringLiteral("Overlapping edits at offset %1").arg(edits.at(i).begin);
			return false;
		}
	}
	out.clear();
	out.reserve(text.size() + 256);
	int pos = 0;
	for (const Edit &e : edits) {
		out += QStringView(text).mid(pos, e.begin - pos);
		out += e.text;
		pos = e.end;
	}
	out += QStringView(text).mid(pos);
	return true;
}

bool prepare(const TcIoTree &tree,
			 const TcIoWriter::Plan &plan,
			 TcIoWriter::Result &res,
			 Prepared &prep)
{
	auto fail = [&res](const QString &msg) {
		res.ok = false;
		res.error = msg;
		return false;
	};

	QHash<QString, QString> renames;
	QHash<int, QString> box_names;
	QHash<int, QString> channel_names;
	QHash<int, QString> channel_comments;

	for (const auto &br : plan.boxRenames) {
		if (br.box < 0 || br.box >= tree.boxes.size()) return fail(QStringLiteral("Invalid box"));
		const auto &box = tree.boxes.at(br.box);
		const QString err = TcIoWriter::validateName(br.newTag);
		if (!err.isEmpty()) return fail(QStringLiteral("%1: %2").arg(box.name, err));
		const QString name = TcIoWriter::fullBoxName(box, br.newTag);
		if (name == box.name) continue;
		box_names.insert(br.box, name);
	}
	for (const auto &ce : plan.channelEdits) {
		if (ce.channel < 0 || ce.channel >= tree.channels.size()) return fail(QStringLiteral("Invalid channel"));
		const auto &ch = tree.channels.at(ce.channel);
		if (ce.setName) {
			const QString err = TcIoWriter::validateName(ce.name);
			if (!err.isEmpty()) return fail(QStringLiteral("%1: %2").arg(ch.name, err));
			if (ce.name != ch.name) channel_names.insert(ce.channel, ce.name);
		}
		if (ce.setComment && ce.comment.trimmed() != ch.comment) {
			if (ch.mainEntryNode < 0) {
				res.warnings << QStringLiteral("%1 has no entry to hold a comment").arg(ch.name);
			} else {
				channel_comments.insert(ce.channel, ce.comment.trimmed());
			}
		}
	}

	// A renamed node must stay unique among its siblings
	for (auto it = box_names.cbegin(); it != box_names.cend(); ++it) {
		const auto &box = tree.boxes.at(it.key());
		const QStringList parent = box.path.mid(0, box.path.size() - 1);
		for (int i = 0; i < tree.boxes.size(); ++i) {
			if (i == it.key()) continue;
			const auto &other = tree.boxes.at(i);
			if (other.path.mid(0, other.path.size() - 1) != parent) continue;
			const QString other_name = box_names.value(i, other.name);
			if (other_name.compare(it.value(), Qt::CaseInsensitive) == 0) {
				return fail(QStringLiteral("\"%1\" would be used twice under the same parent").arg(it.value()));
			}
		}
		renames.insert(box.path.join(QLatin1Char('^')), it.value());
	}
	for (auto it = channel_names.cbegin(); it != channel_names.cend(); ++it) {
		const auto &ch = tree.channels.at(it.key());
		for (int other_idx : tree.boxes.at(ch.box).channels) {
			if (other_idx == it.key()) continue;
			const QString other_name = channel_names.value(
				other_idx, tree.channels.at(other_idx).name);
			if (other_name.compare(it.value(), Qt::CaseInsensitive) == 0) {
				return fail(QStringLiteral("\"%1\" would be used twice in %2")
							.arg(it.value(), tree.boxes.at(ch.box).name));
			}
		}
		renames.insert(tree.channelPath(it.key()).join(QLatin1Char('^')), it.value());
	}

	// Files to look at: the I/O files, then the other .xti
	prep.refs.reserve(tree.referenceFiles.size());
	for (int i = 0; i < tree.files.size(); ++i) prep.files.append(&tree.files.at(i));
	if (!renames.isEmpty()) {
		for (const QString &path : tree.referenceFiles) {
			TcIoTree::File file;
			QString err;
			if (!TcIoTree::readFile(path, file, &err)) {
				res.warnings << err;
				continue;
			}
			prep.refs.append(std::move(file));
			prep.files.append(&prep.refs.last());
		}
	}
	prep.edits.resize(prep.files.size());

	// Names and comments
	for (auto it = box_names.cbegin(); it != box_names.cend(); ++it) {
		const auto &box = tree.boxes.at(it.key());
		const auto &xml = tree.files.at(box.file).xml;
		if (box.nameNode < 0 || xml.node(box.nameNode).selfClosing) {
			return fail(QStringLiteral("%1 has no <Name> to rename").arg(box.name));
		}
		const auto &nd = xml.node(box.nameNode);
		prep.edits[box.file].append({nd.contentBegin, nd.contentEnd,
									 TcXmlScanner::escapeText(it.value())});
		res.changes.append({Kind::BoxName, tree.files.at(box.file).path,
							box.name, it.value()});
	}
	for (auto it = channel_names.cbegin(); it != channel_names.cend(); ++it) {
		const auto &ch = tree.channels.at(it.key());
		const auto &xml = tree.files.at(ch.file).xml;
		const int ai = xml.attributeIndex(ch.pdoNode, QStringLiteral("Name"));
		if (ai < 0) return fail(QStringLiteral("%1 has no Name attribute").arg(ch.name));
		const auto &attr = xml.node(ch.pdoNode).attributes.at(ai);
		prep.edits[ch.file].append({attr.rawBegin, attr.rawEnd,
									TcXmlScanner::escapeAttribute(it.value())});
		res.changes.append({Kind::ChannelName, tree.files.at(ch.file).path,
							tree.boxes.at(ch.box).tag + QLatin1Char('/') + ch.name,
							it.value()});
	}
	for (auto it = channel_comments.cbegin(); it != channel_comments.cend(); ++it) {
		const auto &ch = tree.channels.at(it.key());
		const auto &file = tree.files.at(ch.file);
		addCommentEdit(file, ch.mainEntryNode, it.value(),
					   fileUsesCData(file), prep.edits[ch.file]);
		res.changes.append({Kind::ChannelComment, file.path,
							tree.boxes.at(ch.box).tag + QLatin1Char('/') + ch.name
							+ QStringLiteral(": ") + ch.comment,
							it.value()});
	}

	// Path references that follow a renamed box or channel
	if (!renames.isEmpty()) {
		for (int f = 0; f < prep.files.size(); ++f) {
			const auto &file = *prep.files.at(f);
			const auto &xml = file.xml;
			auto rewriteAttr = [&](int node, const QString &attr_name) {
				const int ai = xml.attributeIndex(node, attr_name);
				if (ai < 0) return;
				const auto &attr = xml.node(node).attributes.at(ai);
				if (!attr.value.contains(QLatin1Char('^'))) return;
				QStringList owner = segments(attr.value);
				QStringList none;
				if (!rewrite(owner, none, renames)) return;
				const QString to = owner.join(QLatin1Char('^'));
				prep.edits[f].append({attr.rawBegin, attr.rawEnd,
									  TcXmlScanner::escapeAttribute(to)});
				res.changes.append({Kind::Reference, file.path, attr.value, to});
			};
			for (int i = 0; i < xml.count(); ++i) {
				const auto &nd = xml.node(i);
				if (nd.name == QLatin1String("OwnerA")) {
					rewriteAttr(i, QStringLiteral("Name"));
					rewriteAttr(i, QStringLiteral("Prefix"));
				} else if (nd.name == QLatin1String("OwnerB")) {
					rewriteAttr(i, QStringLiteral("Name"));
				} else if (nd.name == QLatin1String("IoItemPathName")
						   && !nd.selfClosing && nd.children.isEmpty()) {
					const QString value = xml.text(i).trimmed();
					QStringList owner = segments(value);
					QStringList none;
					if (value.contains(QLatin1Char('^')) && rewrite(owner, none, renames)
						&& value == xml.text(i)) {
						const QString to = owner.join(QLatin1Char('^'));
						prep.edits[f].append({nd.contentBegin, nd.contentEnd,
											  TcXmlScanner::escapeText(to)});
						res.changes.append({Kind::Reference, file.path, value, to});
					}
				} else if (nd.name == QLatin1String("Link") && nd.parent >= 0) {
					const auto &owner_b = xml.node(nd.parent);
					if (owner_b.name != QLatin1String("OwnerB") || owner_b.parent < 0) continue;
					const int owner_a = owner_b.parent;
					QString a_name = xml.attribute(owner_a, QStringLiteral("Name"));
					if (a_name.isEmpty()) a_name = xml.attribute(owner_a, QStringLiteral("Prefix"));
					const QString b_name = xml.attribute(nd.parent, QStringLiteral("Name"));
					const QPair<QString, QString> sides[2] = {
						{QStringLiteral("VarA"), a_name},
						{QStringLiteral("VarB"), b_name}};
					for (const auto &side : sides) {
						const int ai = xml.attributeIndex(i, side.first);
						if (ai < 0) continue;
						const auto &attr = nd.attributes.at(ai);
						QStringList owner = segments(side.second);
						QStringList var = segments(attr.value);
						if (!rewrite(owner, var, renames)) continue;
						const QString to = var.join(QLatin1Char('^'));
						if (to == attr.value) continue;
						prep.edits[f].append({attr.rawBegin, attr.rawEnd,
											  TcXmlScanner::escapeAttribute(to)});
						res.changes.append({Kind::Reference, file.path, attr.value, to});
					}
				}
			}
		}
	}

	// Build the new texts and make sure only values changed
	prep.newTexts.resize(prep.files.size());
	for (int f = 0; f < prep.files.size(); ++f) {
		if (prep.edits.at(f).isEmpty()) continue;
		const auto &file = *prep.files.at(f);
		QString err;
		if (!applyEdits(file.text, prep.edits.at(f), prep.newTexts[f], err)) {
			return fail(QStringLiteral("%1: %2").arg(file.path, err));
		}
		TcXmlScanner check;
		if (!check.parse(prep.newTexts.at(f))) {
			return fail(QStringLiteral("%1: patched text is not valid XML (%2)")
						.arg(file.path, check.errorString()));
		}
		if (check.structureSignature(QStringLiteral("Comment"))
			!= file.xml.structureSignature(QStringLiteral("Comment"))) {
			return fail(QStringLiteral("%1: the structure would change, refused")
						.arg(file.path));
		}
	}

	// References not rewritten, for instance in a format not known here
	for (auto it = renames.cbegin(); it != renames.cend(); ++it) {
		const QString old_segment = it.key().section(QLatin1Char('^'), -1);
		const QString token = QLatin1Char('^') + old_segment;
		for (int f = 0; f < prep.files.size(); ++f) {
			const QString &text = prep.newTexts.at(f).isNull()
				? prep.files.at(f)->text : prep.newTexts.at(f);
			for (const QString &end : {QStringLiteral("^"), QStringLiteral("\""),
									   QStringLiteral("<")}) {
				if (text.contains(token + end)) {
					res.warnings << QStringLiteral(
						"\"%1\" still appears in a path in %2: check the links after reloading")
						.arg(old_segment, QFileInfo(prep.files.at(f)->path).fileName());
					f = prep.files.size();
					break;
				}
			}
		}
	}

	for (const auto &change : std::as_const(res.changes)) {
		if (change.kind == Kind::Reference) ++res.referenceUpdates;
	}
	res.ok = true;
	return true;
}
}

QString TcIoWriter::fullBoxName(const TcIoTree::Box &box, const QString &new_tag)
{
	return box.type.isEmpty()
		? new_tag.trimmed()
		: QStringLiteral("%1 (%2)").arg(new_tag.trimmed(), box.type);
}

/**
 * @brief TcIoWriter::validateName
 * @return An error message, or an empty string if the name can be used.
 */
QString TcIoWriter::validateName(const QString &name)
{
	if (name.trimmed().isEmpty()) return QStringLiteral("empty name");
	if (name.contains(QLatin1Char('^'))) {
		return QStringLiteral("'^' separates TwinCAT path elements and cannot be used in a name");
	}
	for (QChar c : name) {
		if (c.unicode() < 0x20) return QStringLiteral("control character in name");
	}
	return QString();
}

QString TcIoWriter::backupPath(const QString &path, const QDateTime &now)
{
	const QString base = QStringLiteral("%1.%2")
		.arg(path, now.toString(QStringLiteral("yyyyMMdd-HHmmss")));
	QString candidate = base + QStringLiteral(".bak");
	for (int i = 2; QFileInfo::exists(candidate); ++i) {
		candidate = QStringLiteral("%1-%2.bak").arg(base).arg(i);
	}
	return candidate;
}

/**
 * @brief TcIoWriter::preview
 * Computes what apply() would change, without touching any file.
 */
TcIoWriter::Result TcIoWriter::preview(const TcIoTree &tree, const Plan &plan)
{
	Result res;
	Prepared prep;
	prepare(tree, plan, res, prep);
	return res;
}

/**
 * @brief TcIoWriter::apply
 * Writes the plan: verifies that the files did not change since they were
 * read, copies each one to a timestamped .bak, then patches it.
 */
TcIoWriter::Result TcIoWriter::apply(const TcIoTree &tree,
									 const Plan &plan,
									 const QDateTime &now)
{
	Result res;
	Prepared prep;
	if (!prepare(tree, plan, res, prep)) return res;
	res.ok = false;

	QVector<int> modified;
	for (int f = 0; f < prep.files.size(); ++f) {
		if (!prep.newTexts.at(f).isNull() || !prep.edits.at(f).isEmpty()) modified << f;
	}
	if (modified.isEmpty()) {
		res.ok = true;
		return res;
	}

	for (int f : modified) {
		if (f >= tree.files.size()) continue;
		QFile file(tree.files.at(f).path);
		if (!file.open(QIODevice::ReadOnly)) {
			res.error = QStringLiteral("Cannot read %1").arg(file.fileName());
			return res;
		}
		const QByteArray hash = QCryptographicHash::hash(
			file.readAll(), QCryptographicHash::Sha256);
		if (hash != tree.files.at(f).hash) {
			res.error = QStringLiteral(
				"%1 was modified since it was read (is it open in TwinCAT?). "
				"Reload the project data and try again.").arg(file.fileName());
			return res;
		}
	}

	QStringList backups;
	for (int f : modified) {
		const QString path = prep.files.at(f)->path;
		const QString bak = backupPath(path, now);
		if (!QFile::copy(path, bak)) {
			res.error = QStringLiteral("Cannot create the backup %1, nothing was written").arg(bak);
			for (const QString &b : std::as_const(backups)) QFile::remove(b);
			return res;
		}
		backups << bak;
	}
	res.backups = backups;

	QStringList written;
	for (int i = 0; i < modified.size(); ++i) {
		const auto &file = *prep.files.at(modified.at(i));
		QSaveFile out(file.path);
		QByteArray bytes;
		if (file.hasBom) bytes.append("\xEF\xBB\xBF");
		bytes.append(prep.newTexts.at(modified.at(i)).toUtf8());
		if (!out.open(QIODevice::WriteOnly) || out.write(bytes) != bytes.size()
			|| !out.commit()) {
			res.error = QStringLiteral("Cannot write %1: %2").arg(file.path, out.errorString());
			for (int w = 0; w < written.size(); ++w) {
				QFile::remove(written.at(w));
				QFile::copy(backups.at(w), written.at(w));
			}
			return res;
		}
		written << file.path;
	}
	res.filesWritten = written;
	res.ok = true;
	return res;
}
