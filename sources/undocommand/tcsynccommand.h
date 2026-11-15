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
#ifndef TCSYNCCOMMAND_H
#define TCSYNCCOMMAND_H

#include "../properties/elementdata.h"
#include "../twincat/tciosync.h"

#include <QList>
#include <QPair>
#include <QPointer>
#include <QUndoCommand>
#include <QVector>

class Element;

/**
 * @brief The TcSyncCommand class
 * One undo step for a TwinCAT -> QET exchange: the I/O tables of all the
 * PLC masters that changed are restored or redone together.
 */
class TcSyncCommand : public QUndoCommand
{
	public:
		using Change = QPair<Element *, ElementData>;

		TcSyncCommand(const QList<Change> &changes,
					  QUndoCommand *parent = nullptr);

		static ElementData::PlcIOType ioType(TcIoTree::Direction direction,
											 TcIoTree::Kind kind);
		static ElementData withTable(
			const ElementData &data,
			const QVector<TcSyncQetChannel> &channels);
		static QUndoCommand *fromUpdates(
			const QList<QPointer<Element>> &elements,
			const QVector<TcSyncCardUpdate> &updates);
};

#endif // TCSYNCCOMMAND_H
