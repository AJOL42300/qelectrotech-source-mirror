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
#ifndef TCQETBRIDGE_H
#define TCQETBRIDGE_H

#include "../properties/elementdata.h"
#include "tciosync.h"

#include <QList>
#include <QPointer>
#include <QVector>

class Element;
class QETProject;

/**
 * @brief The TcQetBridge class
 * Reads the I/O tables of the PLC masters of a QET project as cards for
 * TcIoSync. Nothing is modified here: TwinCAT -> QET goes through
 * TcSyncCommand.
 */
class TcQetBridge
{
	public:
		struct Cards {
			QVector<TcSyncQetCard> cards;
			QList<QPointer<Element>> elements;  ///< same order as cards
		};

		static Cards collect(QETProject *project);
		static TcSyncQetChannel channelOf(int table_index,
										  const ElementData::PlcIO &io);
};

#endif // TCQETBRIDGE_H
