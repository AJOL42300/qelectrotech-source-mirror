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
#include "tciosync.h"

#include <QCoreApplication>
#include <QRegularExpression>
#include <QSet>

namespace {
int keyOf(TcIoTree::Direction direction, int number)
{
	return static_cast<int>(direction) * 100000 + number;
}

QString csvField(const QString &value)
{
	QString v = value;
	v.replace(QLatin1Char('"'), QLatin1String("\"\""));
	if (v.contains(QLatin1Char(';')) || v.contains(QLatin1Char('"'))
		|| v.contains(QLatin1Char('\n'))) {
		v = QLatin1Char('"') + v + QLatin1Char('"');
	}
	return v;
}
}

QString TcIoSync::normalizeTag(const QString &tag)
{
	return tag.simplified().toLower();
}

/**
 * @brief TcIoSync::channelNumber
 * @return The channel number at the end of an address: "X01(3)" -> 3,
 * "3" -> 3. 0 if there is none.
 */
int TcIoSync::channelNumber(const QString &address)
{
	static const QRegularExpression inParens(QStringLiteral("\\((\\d+)\\)\\s*$"));
	static const QRegularExpression trailing(QStringLiteral("(\\d+)\\s*$"));
	auto match = inParens.match(address);
	if (!match.hasMatch()) match = trailing.match(address);
	return match.hasMatch() ? match.captured(1).toInt() : 0;
}

/// Names that TwinCAT gives by itself, as opposed to a name chosen by a user
bool TcIoSync::isDefaultName(const QString &name)
{
	static const QRegularExpression re(
		QStringLiteral("^(AI |AO )?(Standard |Compact )?(Channel|Ch\\.?|Input|Output)\\s*\\d*$"),
		QRegularExpression::CaseInsensitiveOption);
	return name.trimmed().isEmpty() || re.match(name.trimmed()).hasMatch();
}

QString TcIoSync::typeText(TcIoTree::Direction direction, TcIoTree::Kind kind)
{
	const bool in = direction == TcIoTree::Direction::Input;
	const bool out = direction == TcIoTree::Direction::Output;
	switch (kind) {
		case TcIoTree::Kind::Digital:
			return in ? QCoreApplication::translate("TcIoSync", "digital input")
					  : out ? QCoreApplication::translate("TcIoSync", "digital output")
							: QCoreApplication::translate("TcIoSync", "digital");
		case TcIoTree::Kind::Analog:
			return in ? QCoreApplication::translate("TcIoSync", "analog input")
					  : out ? QCoreApplication::translate("TcIoSync", "analog output")
							: QCoreApplication::translate("TcIoSync", "analog");
		default:
			return QCoreApplication::translate("TcIoSync", "other");
	}
}

QString TcIoSync::kindText(TcSyncItem::Kind kind)
{
	switch (kind) {
		case TcSyncItem::Kind::BoxRename:
			return QCoreApplication::translate("TcIoSync", "Box identifier");
		case TcSyncItem::Kind::NameDiff:
			return QCoreApplication::translate("TcIoSync", "Channel name");
		case TcSyncItem::Kind::CommentDiff:
			return QCoreApplication::translate("TcIoSync", "Comment");
		case TcSyncItem::Kind::ChannelOnlyInTwincat:
			return QCoreApplication::translate("TcIoSync", "Only in TwinCAT");
		case TcSyncItem::Kind::ChannelOnlyInQet:
			return QCoreApplication::translate("TcIoSync", "Only in QElectroTech");
		case TcSyncItem::Kind::TypeMismatch:
			return QCoreApplication::translate("TcIoSync", "Incompatible type");
		case TcSyncItem::Kind::CardOnlyInQet:
			return QCoreApplication::translate("TcIoSync", "Card without TwinCAT box");
		case TcSyncItem::Kind::BoxOnlyInTwincat:
			return QCoreApplication::translate("TcIoSync", "TwinCAT box without card");
	}
	return QString();
}

QVector<TcSyncItem> TcIoSync::compare(const QVector<TcSyncQetCard> &cards,
									  const TcIoTree &tree,
									  const QHash<QString, int> &pairing)
{
	QVector<TcSyncItem> items;
	QSet<int> matched_boxes;

	QHash<QString, QVector<int>> by_tag;
	for (int b = 0; b < tree.boxes.size(); ++b) {
		by_tag[normalizeTag(tree.boxes.at(b).tag)].append(b);
	}
	// Pairings chosen by the user come first, so that they are not stolen
	QSet<int> paired;
	for (auto it = pairing.cbegin(); it != pairing.cend(); ++it) {
		if (it.value() >= 0 && it.value() < tree.boxes.size()) paired.insert(it.value());
	}

	for (int ci = 0; ci < cards.size(); ++ci) {
		const TcSyncQetCard &card = cards.at(ci);
		int box = -1;
		const auto pair = pairing.constFind(card.id);
		if (pair != pairing.constEnd() && pair.value() >= 0
			&& pair.value() < tree.boxes.size()) {
			box = pair.value();
		} else if (!normalizeTag(card.tag).isEmpty()) {
			for (int candidate : by_tag.value(normalizeTag(card.tag))) {
				if (matched_boxes.contains(candidate) || paired.contains(candidate)) continue;
				if (box < 0 || (tree.boxes.at(box).channels.isEmpty()
								&& !tree.boxes.at(candidate).channels.isEmpty())) {
					box = candidate;
				}
			}
		}
		if (box < 0 || matched_boxes.contains(box)) {
			TcSyncItem item;
			item.kind = TcSyncItem::Kind::CardOnlyInQet;
			item.card = ci;
			item.label = card.tag;
			item.qetValue = card.tag;
			items.append(item);
			continue;
		}
		matched_boxes.insert(box);
		const TcIoTree::Box &tb = tree.boxes.at(box);

		if (card.tag.trimmed() != tb.tag) {
			TcSyncItem item;
			item.kind = TcSyncItem::Kind::BoxRename;
			item.card = ci;
			item.box = box;
			item.label = tb.name;
			item.qetValue = card.tag;
			item.twincatValue = tb.tag;
			item.canQetToTwincat = TcIoWriter::validateName(card.tag).isEmpty();
			items.append(item);
		}

		QHash<int, int> tc_by_key;
		for (int chi : tb.channels) {
			const auto &ch = tree.channels.at(chi);
			if (ch.ordinal > 0) tc_by_key.insert(keyOf(ch.direction, ch.ordinal), chi);
		}
		QSet<int> used;
		QHash<int, int> position;

		for (int qi = 0; qi < card.channels.size(); ++qi) {
			const TcSyncQetChannel &q = card.channels.at(qi);
			const int pos = ++position[static_cast<int>(q.direction)];
			int number = channelNumber(q.address);
			if (number <= 0) number = pos;
			const int key = keyOf(q.direction, number);
			TcSyncItem base;
			base.card = ci;
			base.qetChannel = qi;
			base.box = box;
			base.label = QStringLiteral("%1 / %2").arg(card.tag, q.address.isEmpty()
													   ? QString::number(number) : q.address);

			if (q.direction == TcIoTree::Direction::Unknown
				|| !tc_by_key.contains(key) || used.contains(key)) {
				TcSyncItem item = base;
				item.kind = TcSyncItem::Kind::ChannelOnlyInQet;
				item.qetValue = q.name;
				items.append(item);
				continue;
			}
			used.insert(key);
			const int chi = tc_by_key.value(key);
			const TcIoTree::Channel &tc = tree.channels.at(chi);
			base.channel = chi;

			if (!q.universal && q.kind != tc.kind) {
				TcSyncItem item = base;
				item.kind = TcSyncItem::Kind::TypeMismatch;
				item.qetValue = typeText(q.direction, q.kind);
				item.twincatValue = typeText(tc.direction, tc.kind);
				items.append(item);
				continue;
			}

			const QString qname = q.name.trimmed();
			const bool tc_default = isDefaultName(tc.name);
			if (qname != tc.name && !(qname.isEmpty() && tc_default)) {
				TcSyncItem item = base;
				item.kind = TcSyncItem::Kind::NameDiff;
				item.qetValue = qname;
				item.twincatValue = tc.name;
				item.canQetToTwincat = !qname.isEmpty()
					&& TcIoWriter::validateName(qname).isEmpty();
				item.canTwincatToQet = !tc_default;
				items.append(item);
			}
			const QString qcomment = q.comment.trimmed();
			if (qcomment != tc.comment && tc.mainEntryNode >= 0) {
				TcSyncItem item = base;
				item.kind = TcSyncItem::Kind::CommentDiff;
				item.qetValue = qcomment;
				item.twincatValue = tc.comment;
				item.canQetToTwincat = !qcomment.isEmpty();
				item.canTwincatToQet = !tc.comment.isEmpty();
				items.append(item);
			}
		}

		for (int chi : tb.channels) {
			const auto &ch = tree.channels.at(chi);
			if (ch.ordinal <= 0 || used.contains(keyOf(ch.direction, ch.ordinal))) continue;
			TcSyncItem item;
			item.kind = TcSyncItem::Kind::ChannelOnlyInTwincat;
			item.card = ci;
			item.box = box;
			item.channel = chi;
			item.label = QStringLiteral("%1 / %2").arg(card.tag).arg(ch.ordinal);
			item.twincatValue = ch.name;
			item.canTwincatToQet = true;
			items.append(item);
		}
	}

	for (int b = 0; b < tree.boxes.size(); ++b) {
		if (matched_boxes.contains(b)) continue;
		bool has_io = false;
		for (int chi : tree.boxes.at(b).channels) {
			if (tree.channels.at(chi).ordinal > 0) has_io = true;
		}
		if (!has_io) continue;
		TcSyncItem item;
		item.kind = TcSyncItem::Kind::BoxOnlyInTwincat;
		item.box = b;
		item.label = tree.boxes.at(b).name;
		item.twincatValue = tree.boxes.at(b).name;
		items.append(item);
	}
	return items;
}

TcIoWriter::Plan TcIoSync::twincatPlan(const QVector<TcSyncItem> &items,
									   const QVector<TcSyncQetCard> &cards)
{
	TcIoWriter::Plan plan;
	QHash<int, int> edit_of_channel;
	for (const TcSyncItem &item : items) {
		if (item.direction != TcSyncItem::Direction::QetToTwincat
			|| !item.canQetToTwincat) continue;
		if (item.kind == TcSyncItem::Kind::BoxRename) {
			plan.boxRenames.append({item.box, cards.at(item.card).tag});
		} else if (item.kind == TcSyncItem::Kind::NameDiff
				   || item.kind == TcSyncItem::Kind::CommentDiff) {
			int idx = edit_of_channel.value(item.channel, -1);
			if (idx < 0) {
				idx = plan.channelEdits.size();
				edit_of_channel.insert(item.channel, idx);
				TcIoWriter::ChannelEdit edit;
				edit.channel = item.channel;
				plan.channelEdits.append(edit);
			}
			auto &edit = plan.channelEdits[idx];
			if (item.kind == TcSyncItem::Kind::NameDiff) {
				edit.setName = true;
				edit.name = item.qetValue;
			} else {
				edit.setComment = true;
				edit.comment = item.qetValue;
			}
		}
	}
	return plan;
}

QVector<TcSyncCardUpdate> TcIoSync::qetUpdates(const QVector<TcSyncItem> &items,
											   const QVector<TcSyncQetCard> &cards,
											   const TcIoTree &tree)
{
	QHash<int, TcSyncCardUpdate> updates;
	QVector<int> order;
	for (const TcSyncItem &item : items) {
		if (item.direction != TcSyncItem::Direction::TwincatToQet
			|| !item.canTwincatToQet || item.card < 0) continue;
		if (!updates.contains(item.card)) {
			TcSyncCardUpdate update;
			update.card = item.card;
			update.channels = cards.at(item.card).channels;
			updates.insert(item.card, update);
			order << item.card;
		}
		TcSyncCardUpdate &update = updates[item.card];
		if (item.kind == TcSyncItem::Kind::NameDiff && item.qetChannel >= 0) {
			update.channels[item.qetChannel].name = item.twincatValue;
		} else if (item.kind == TcSyncItem::Kind::CommentDiff && item.qetChannel >= 0) {
			update.channels[item.qetChannel].comment = item.twincatValue;
		} else if (item.kind == TcSyncItem::Kind::ChannelOnlyInTwincat && item.channel >= 0) {
			const auto &tc = tree.channels.at(item.channel);
			TcSyncQetChannel row;
			row.address = QString::number(tc.ordinal);
			row.name = isDefaultName(tc.name) ? QString() : tc.name;
			row.comment = tc.comment;
			row.direction = tc.direction;
			row.kind = tc.kind;
			update.channels.append(row);
		}
	}
	QVector<TcSyncCardUpdate> result;
	for (int card : order) result.append(updates.value(card));
	return result;
}

/// Semicolon separated, one line per difference
QString TcIoSync::reportCsv(const QVector<TcSyncItem> &items)
{
	QString out = QStringLiteral("kind;label;qet;twincat\n");
	for (const TcSyncItem &item : items) {
		out += csvField(kindText(item.kind)) + QLatin1Char(';')
			+ csvField(item.label) + QLatin1Char(';')
			+ csvField(item.qetValue) + QLatin1Char(';')
			+ csvField(item.twincatValue) + QLatin1Char('\n');
	}
	return out;
}
