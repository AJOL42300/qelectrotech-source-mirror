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
#include "tcsynccommand.h"

#include "../qetgraphicsitem/element.h"
#include "changeelementdatacommand.h"

TcSyncCommand::TcSyncCommand(const QList<Change> &changes,
							 QUndoCommand *parent) :
	QUndoCommand(parent)
{
	setText(QObject::tr("Exchange I/O names with TwinCAT"));
	for (const Change &change : changes) {
		if (change.first) {
			new ChangeElementDataCommand(change.first, change.second, this);
		}
	}
}

/// QET table type for a TwinCAT channel, used for the rows TwinCAT adds
ElementData::PlcIOType TcSyncCommand::ioType(TcIoTree::Direction direction,
											 TcIoTree::Kind kind)
{
	const bool input = direction == TcIoTree::Direction::Input;
	switch (kind) {
		case TcIoTree::Kind::Digital:
			return input ? ElementData::EntreeDigitale
						 : ElementData::SortieDigitale;
		case TcIoTree::Kind::Analog:
			return input ? ElementData::EntreeAnalogique
						 : ElementData::SortieAnalogique;
		default:
			return input ? ElementData::EntreeUniverselle
						 : ElementData::SortieUniverselle;
	}
}

/**
	@return @p data with the name and comment of the rows of @p channels.
	Rows with a table index keep every other value (terminals, cross
	reference...); the others are appended.
*/
ElementData TcSyncCommand::withTable(
	const ElementData &data,
	const QVector<TcSyncQetChannel> &channels)
{
	ElementData result = data;
	ElementData::PlcMasterData plc = data.plcMasterData();
	for (const TcSyncQetChannel &channel : channels) {
		if (channel.tableIndex >= 0 && channel.tableIndex < plc.ios.size()) {
			ElementData::PlcIO &row = plc.ios[channel.tableIndex];
			row.functionText = channel.name;
			row.comment = channel.comment;
		} else if (channel.tableIndex < 0
				   && channel.direction != TcIoTree::Direction::Unknown) {
			ElementData::PlcIO row;
			row.type = ioType(channel.direction, channel.kind);
			row.address = channel.address;
			row.functionText = channel.name;
			row.comment = channel.comment;
			plc.ios.append(row);
		}
	}
	result.setPlcMasterData(plc);
	return result;
}

/**
	@return One undo step applying @p updates, whose card indexes refer to
	@p elements; nullptr if nothing changes.
*/
QUndoCommand *TcSyncCommand::fromUpdates(
	const QList<QPointer<Element>> &elements,
	const QVector<TcSyncCardUpdate> &updates)
{
	QList<Change> changes;
	for (const TcSyncCardUpdate &update : updates) {
		if (update.card < 0 || update.card >= elements.size()) continue;
		Element *element = elements.at(update.card).data();
		if (!element) continue;
		const ElementData data = withTable(element->elementData(),
										   update.channels);
		if (data.plcMasterData() != element->elementData().plcMasterData()) {
			changes.append(Change(element, data));
		}
	}
	if (changes.isEmpty()) return nullptr;
	return new TcSyncCommand(changes);
}
