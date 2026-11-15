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
#ifndef TCIOWRITER_H
#define TCIOWRITER_H

#include "tciotree.h"

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QVector>

/**
 * @brief The TcIoWriter class
 * Writes names and comments back into the TwinCAT files, and nothing else.
 *
 * The I/O tree is never restructured: no node is added, removed or moved,
 * and addresses, types, indexes and PDO layout are untouched. Only
 * - the name of a box (a coupler or terminal),
 * - the name of a channel (Pdo Name attribute),
 * - the comment of a channel (Comment of its main entry),
 * - and the path references that follow a renamed box or channel
 *   (Mappings, variable links, IoItemPathName)
 * can change. Each edit replaces only those characters in the original
 * text, so the rest of every file stays identical byte for byte.
 *
 * Before writing, the files are checked against the hash taken when they
 * were read (refused if they changed since) and a timestamped .bak copy is
 * made. TwinCAT XAE is never started; if it has the project open it has to
 * reload it afterwards.
 */
class TcIoWriter
{
	public:
		struct BoxRename {
			int box = -1;
			QString newTag;     ///< new identifier; the "(type)" suffix is kept
		};

		struct ChannelEdit {
			int channel = -1;
			bool setName = false;
			QString name;
			bool setComment = false;
			QString comment;
		};

		struct Plan {
			QVector<BoxRename> boxRenames;
			QVector<ChannelEdit> channelEdits;
			bool isEmpty() const {
				return boxRenames.isEmpty() && channelEdits.isEmpty();
			}
		};

		struct Change {
			enum class Kind {BoxName, ChannelName, ChannelComment, Reference};
			Kind kind = Kind::BoxName;
			QString file;
			QString from;
			QString to;
		};

		struct Result {
			bool ok = false;
			QString error;
			QStringList warnings;
			QVector<Change> changes;
			QStringList filesWritten;
			QStringList backups;
			int referenceUpdates = 0;
		};

		static QString fullBoxName(const TcIoTree::Box &box, const QString &new_tag);
		static QString validateName(const QString &name);
		static QString backupPath(const QString &path, const QDateTime &now);

		static Result preview(const TcIoTree &tree, const Plan &plan);
		static Result apply(const TcIoTree &tree,
							const Plan &plan,
							const QDateTime &now = QDateTime::currentDateTime());
};

#endif // TCIOWRITER_H
