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
#include "tcqetbridge.h"

#include "../diagram.h"
#include "../qetgraphicsitem/element.h"
#include "../qetproject.h"

/// The reconciliation row of the table row @p io
TcSyncQetChannel TcQetBridge::channelOf(int table_index,
										const ElementData::PlcIO &io)
{
	TcSyncQetChannel channel;
	channel.address = io.address;
	channel.name = io.functionText;
	channel.comment = io.comment;
	channel.tableIndex = table_index;
	switch (io.type) {
		case ElementData::EntreeDigitale:
			channel.direction = TcIoTree::Direction::Input;
			channel.kind = TcIoTree::Kind::Digital;
			break;
		case ElementData::SortieDigitale:
			channel.direction = TcIoTree::Direction::Output;
			channel.kind = TcIoTree::Kind::Digital;
			break;
		case ElementData::EntreeAnalogique:
			channel.direction = TcIoTree::Direction::Input;
			channel.kind = TcIoTree::Kind::Analog;
			break;
		case ElementData::SortieAnalogique:
			channel.direction = TcIoTree::Direction::Output;
			channel.kind = TcIoTree::Kind::Analog;
			break;
		case ElementData::EntreeUniverselle:
			channel.direction = TcIoTree::Direction::Input;
			channel.universal = true;
			break;
		case ElementData::SortieUniverselle:
			channel.direction = TcIoTree::Direction::Output;
			channel.universal = true;
			break;
	}
	return channel;
}

/**
	@return The PLC masters of @p project that have an I/O table, with
	their identifier (the element label) and one row per table row.
	Folios are built when needed, so this can take a while on a big project.
*/
TcQetBridge::Cards TcQetBridge::collect(QETProject *project)
{
	Cards result;
	if (!project) return result;
	for (Diagram *diagram : project->diagrams()) {
		for (Element *element : diagram->elements()) {
			const ElementData &data = element->elementData();
			if (data.m_type != ElementData::Master
				|| data.m_master_type != ElementData::PLC) continue;
			const auto ios = data.plcMasterData().ios;
			if (ios.isEmpty()) continue;
			TcSyncQetCard card;
			card.id = element->uuid().toString();
			card.tag = element->actualLabel();
			for (int i = 0; i < ios.size(); ++i) {
				card.channels.append(channelOf(i, ios.at(i)));
			}
			result.cards.append(card);
			result.elements.append(QPointer<Element>(element));
		}
	}
	return result;
}
