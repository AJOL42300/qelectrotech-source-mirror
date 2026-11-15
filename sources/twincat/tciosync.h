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
#ifndef TCIOSYNC_H
#define TCIOSYNC_H

#include "tciotree.h"
#include "tciowriter.h"

#include <QHash>
#include <QString>
#include <QVector>

/**
 * @brief The TcSyncQetChannel struct
 * One row of the I/O table of a PLC master, as seen by the reconciliation.
 */
struct TcSyncQetChannel
{
	QString address;                ///< "X01(1)": the number is the channel
	QString name;                   ///< function text
	QString comment;
	TcIoTree::Direction direction = TcIoTree::Direction::Unknown;
	TcIoTree::Kind kind = TcIoTree::Kind::Other;
	bool universal = false;         ///< compatible with digital and analog
	int tableIndex = -1;            ///< row in the QET table, -1 for new rows
};

/**
 * @brief The TcSyncQetCard struct
 * A PLC master of the QET project: an I/O card or box.
 */
struct TcSyncQetCard
{
	QString id;                     ///< opaque, e.g. element uuid
	QString tag;                    ///< identifier of the card, e.g. "A1.02"
	QVector<TcSyncQetChannel> channels;
};

/**
 * @brief The TcSyncItem struct
 * One difference between QET and TwinCAT.
 */
struct TcSyncItem
{
	enum class Kind {
		BoxRename,              ///< same card, identifiers differ
		NameDiff,               ///< channel names differ
		CommentDiff,            ///< channel comments differ
		ChannelOnlyInTwincat,   ///< no such row in the QET table
		ChannelOnlyInQet,       ///< no such channel in TwinCAT (report only)
		TypeMismatch,           ///< digital against analog (report only)
		CardOnlyInQet,          ///< no box in TwinCAT (report only)
		BoxOnlyInTwincat        ///< no card in QET (report only)
	};
	enum class Direction {None, QetToTwincat, TwincatToQet};

	Kind kind = Kind::NameDiff;
	int card = -1;                  ///< index in the QET cards
	int qetChannel = -1;            ///< index in the card channels
	int box = -1;                   ///< index in TcIoTree::boxes
	int channel = -1;               ///< index in TcIoTree::channels
	QString label;                  ///< "A1.02 / 3"
	QString qetValue;
	QString twincatValue;
	bool canQetToTwincat = false;
	bool canTwincatToQet = false;
	Direction direction = Direction::None;   ///< choice of the user
};

/**
 * @brief The TcSyncCardUpdate struct
 * New content of the table of a QET card, after TwinCAT -> QET.
 */
struct TcSyncCardUpdate
{
	int card = -1;
	QVector<TcSyncQetChannel> channels;
};

/**
 * @brief The TcIoSync class
 * Compares the I/O of a QET project with a TwinCAT I/O tree and turns the
 * choices made on the differences into a plan for each side.
 *
 * Matching rules:
 * - a QET card matches the TwinCAT box whose identifier (name without its
 *   "(type)") is the same, or the box picked by the user in @p pairing;
 * - a QET row matches the channel of the box with the same direction and
 *   the same number: the number at the end of the address ("X01(3)" -> 3)
 *   against the position of the channel among the digital/analog channels
 *   of that direction. The position is stable when channels are renamed.
 */
class TcIoSync
{
	public:
		static QString normalizeTag(const QString &tag);
		static int channelNumber(const QString &address);
		static bool isDefaultName(const QString &name);
		static QString typeText(TcIoTree::Direction direction, TcIoTree::Kind kind);

		static QVector<TcSyncItem> compare(
			const QVector<TcSyncQetCard> &cards,
			const TcIoTree &tree,
			const QHash<QString, int> &pairing = QHash<QString, int>());

		static TcIoWriter::Plan twincatPlan(const QVector<TcSyncItem> &items,
											const QVector<TcSyncQetCard> &cards);
		static QVector<TcSyncCardUpdate> qetUpdates(
			const QVector<TcSyncItem> &items,
			const QVector<TcSyncQetCard> &cards,
			const TcIoTree &tree);

		static QString kindText(TcSyncItem::Kind kind);
		static QString reportCsv(const QVector<TcSyncItem> &items);
};

#endif // TCIOSYNC_H
