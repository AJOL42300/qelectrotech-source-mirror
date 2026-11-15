// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "changewireprofilescommand.h"

#include <conductorcolorcodes.h>
#include <diagram.h>
#include <qetgraphicsitem/conductor.h>
#include <qetproject.h>

/**
	@brief ChangeWireProfilesCommand::ChangeWireProfilesCommand
	@param project : the project, its conductors are all looked at
	@param new_profiles : the standard wires to set
	@param renamed : old name -> new name, for the profiles renamed
	@param parent
*/
ChangeWireProfilesCommand::ChangeWireProfilesCommand(
		QETProject *project, const WireProfiles &new_profiles,
		const QHash<QString, QString> &renamed, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_project(project),
	m_new_profiles(new_profiles)
{
	setText(QObject::tr("Change the standard wires"));
	if (!project) {
		return;
	}
	m_old_profiles = project->wireProfiles();

		//diagrams() builds the folios not built yet
	const QList<Diagram *> folios = project->diagrams();
	for (Diagram *folio : folios)
	{
		for (Conductor *wire : folio->conductors())
		{
			const ConductorProperties before = wire->properties();
			if (before.m_wire_profile.isEmpty()) {
				continue;
			}
			const ConductorProperties after = applied(before, new_profiles, renamed);
			if (after == before) {
				continue;
			}
			m_changes.append({wire, before, after});
		}
	}
}

/**
	@brief ChangeWireProfilesCommand::applied
	@param properties : the properties of a conductor
	@param profiles : the standard wires to apply
	@param renamed : old name -> new name
	@return @p properties as they become with @p profiles: nothing
	changes without a profile. With one, the colour and the section of the
	profile are copied (not for a core of a cable). A profile that no
	longer exists is dropped, colour and section stay.
*/
ConductorProperties ChangeWireProfilesCommand::applied(
		const ConductorProperties &properties, const WireProfiles &profiles,
		const QHash<QString, QString> &renamed)
{
	ConductorProperties result = properties;
	if (properties.m_wire_profile.isEmpty()) {
		return result;
	}

	const QString name = renamed.value(properties.m_wire_profile,
									   properties.m_wire_profile);
	WireProfile profile;
	if (!profiles.find(name, &profile)) {
		result.m_wire_profile.clear();
		return result;
	}

	result.m_wire_profile = name;
	if (!properties.m_cable_uuid.isNull()) {
		return result;
	}
	if (const ConductorColorCodes::Code *code = ConductorColorCodes::find(profile.color)) {
		ConductorColorCodes::apply(*code, result);
	} else {
		result.m_wire_color = profile.color;
	}
	result.m_wire_section = profile.section;
	return result;
}

void ChangeWireProfilesCommand::undo()
{
	if (!m_project) {
		return;
	}
	m_project->setWireProfiles(m_old_profiles);
	for (const Change &change : std::as_const(m_changes)) {
		if (change.wire) {
			change.wire->setProperties(change.old_properties);
		}
	}
}

void ChangeWireProfilesCommand::redo()
{
	if (!m_project) {
		return;
	}
	m_project->setWireProfiles(m_new_profiles);
	for (const Change &change : std::as_const(m_changes)) {
		if (change.wire) {
			change.wire->setProperties(change.new_properties);
		}
	}
}
