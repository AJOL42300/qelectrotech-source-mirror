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
#ifndef TCIOTREE_H
#define TCIOTREE_H

#include "tcxmlscanner.h"

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

/**
 * @brief The TcIoTree class
 * Read-only view of the I/O tree of a TwinCAT 3 project (.tsproj, plus the
 * .xti files that are not already inlined in it): EtherCAT devices, boxes
 * (couplers and terminals) and their process data channels (PDO).
 * TwinCAT is never started: the XML files are only parsed.
 * Every box and channel remembers the XML nodes it comes from, so that
 * TcIoWriter can patch the names in place.
 */
class TcIoTree
{
	public:
		enum class Direction {Unknown, Input, Output};
		enum class Kind {Other, Digital, Analog};

		struct File {
			QString path;
			QString text;           ///< decoded content, without the BOM
			bool hasBom = false;
			QByteArray hash;        ///< SHA-256 of the bytes read
			TcXmlScanner xml;
		};

		struct Box {
			QString name;           ///< full name, e.g. "A1.02 (EL1004)"
			QString tag;            ///< part before the type, e.g. "A1.02"
			QString type;           ///< part between parentheses, e.g. "EL1004"
			QStringList path;       ///< TIID, device, boxes..., this box
			int file = -1;
			int nameNode = -1;      ///< <Name> element of the box
			int parent = -1;        ///< parent box index, -1 below the device
			QVector<int> channels;
		};

		struct Channel {
			QString name;           ///< Pdo Name attribute
			QString comment;        ///< Comment of the main entry
			Direction direction = Direction::Unknown;
			Kind kind = Kind::Other;
			int ordinal = 0;        ///< 1-based among digital/analog channels of the same direction in the box, 0 otherwise
			int box = -1;
			int file = -1;
			int pdoNode = -1;
			int mainEntryNode = -1;
			QString pdoIndex;       ///< e.g. "#x1a00"
		};

		static bool load(const QString &project_path,
						 TcIoTree &tree,
						 QString *error = nullptr);
		static bool readFile(const QString &path,
							 File &file,
							 QString *error = nullptr);
		static QString resolveProjectFile(const QString &path);
		static QStringList splitBoxName(const QString &name);
		static QStringList splitPath(const QString &path);

		QStringList channelPath(int channel) const;

		QString projectFile;            ///< .tsproj or .xti that was opened
		QVector<File> files;            ///< files holding I/O devices
		QStringList referenceFiles;     ///< other .xti that may reference I/O paths
		QVector<Box> boxes;
		QVector<Channel> channels;
		QStringList devices;            ///< device names, in file order
		QStringList warnings;

	private:
		bool parseFile(int file_index);
};

#endif // TCIOTREE_H
