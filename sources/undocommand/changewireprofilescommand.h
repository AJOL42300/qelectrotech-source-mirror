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
#ifndef CHANGEWIREPROFILESCOMMAND_H
#define CHANGEWIREPROFILESCOMMAND_H

#include <QHash>
#include <QPointer>
#include <QUndoCommand>
#include <QVector>

#include <conductorproperties.h>
#include <properties/wireprofiles.h>

class Conductor;
class QETProject;

/**
	@brief The ChangeWireProfilesCommand class
	Replaces the standard wires of a project and, in the same undoable
	step, gives every conductor that names a profile the colour and the
	section of that profile as it now is. A profile renamed keeps its
	conductors (see @p renamed); a profile removed leaves its conductors
	as they are, without a profile. A conductor that is a core of a cable
	takes its colour and section from the cable, so only its profile name
	follows.
*/
class ChangeWireProfilesCommand final : public QUndoCommand
{
	public:
		ChangeWireProfilesCommand(QETProject *project,
								  const WireProfiles &new_profiles,
								  const QHash<QString, QString> &renamed,
								  QUndoCommand *parent = nullptr);

			/// The conductors this command changes
		int changedConductors() const {return m_changes.size();}
		void undo() override;
		void redo() override;

		static ConductorProperties applied(const ConductorProperties &properties,
										   const WireProfiles &profiles,
										   const QHash<QString, QString> &renamed);

	private:
		struct Change
		{
			QPointer<Conductor> wire;
			ConductorProperties old_properties;
			ConductorProperties new_properties;
		};

		QPointer<QETProject> m_project;
		WireProfiles m_old_profiles;
		WireProfiles m_new_profiles;
		QVector<Change> m_changes;
};

#endif // CHANGEWIREPROFILESCOMMAND_H
