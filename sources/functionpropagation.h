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
#ifndef FUNCTIONPROPAGATION_H
#define FUNCTIONPROPAGATION_H

#include "diagramcontext.h"
#include "properties/elementdata.h"

#include <QMap>
#include <QObject>
#include <QPair>
#include <QPointer>

class Element;
class QUndoCommand;

/**
	@brief The FunctionPropagation class
	Gives the function of a channel to the other end of its cables.

	A module with channels (a PLC symbol with one terminal per socket, two
	channels per terminal) is wired to a valve through cables, T couplers
	and Y distributors. When the function of a channel is edited, the
	function of the channel at the other end follows, in the same undo
	step, in both directions.

	Nothing is hard coded: a symbol that passes the channels through
	describes it in its own data, as kind information entries of its
	definition:
	- "function_routes": "X1:1>X2:1;X1:2>X3:1" (terminal:channel), the
	  channel of a terminal leaves by the channel of another terminal;
	  a channel without route stops the chain;
	- "function_ends": "X1:1" the channels of a symbol that keeps its
	  function in its "function" information (a valve).
	The channels of a PLC symbol are those of its input / output table:
	the k-th entry of a terminal is its channel k.

	The follow is off by default (setting "function-propagation"). When
	the other end already holds another function, it is kept and the
	conflict is reported by conflictFound.
*/
class FunctionPropagation : public QObject
{
	Q_OBJECT

	public:
		static QString settingsKey();
		static bool isEnabled();
		static FunctionPropagation *notifier();

		static void follow(
				QUndoCommand *parent,
				const QMap<QPointer<Element>,
				           QPair<DiagramContext, DiagramContext>> &infos);
		static void follow(
				QUndoCommand *parent,
				Element *element,
				const ElementData &new_data);

	signals:
		void conflictFound(const QString &message);

	private:
		FunctionPropagation() = default;
};

#endif // FUNCTIONPROPAGATION_H
