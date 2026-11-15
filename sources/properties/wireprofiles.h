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
#ifndef WIREPROFILES_H
#define WIREPROFILES_H

#include <QDomElement>
#include <QList>
#include <QString>

/**
	@brief The WireProfile struct
	A standard wire of the project: a name ("Control 1.5 BK"), the
	colour (an IEC 60757 code such as BK, or free text) and the section
	(in mm2, free text) every wire given this profile has.
*/
struct WireProfile
{
	QString name;
	QString color;
	QString section;

	bool operator==(const WireProfile &other) const;
	bool operator!=(const WireProfile &other) const {return !(*this == other);}
};

/**
	@brief The WireProfiles class
	The list of standard wires of a project. A conductor names its
	profile in ConductorProperties::m_wire_profile; editing a profile
	here and applying it (ChangeWireProfilesCommand) changes the colour
	and the section of all the conductors that name it, in one step.
	Empty by default: a project that does not use it is saved exactly as
	before.
*/
class WireProfiles
{
	public:
		const QList<WireProfile> &profiles() const {return m_profiles;}
		void setProfiles(const QList<WireProfile> &profiles) {m_profiles = profiles;}

		bool find(const QString &name, WireProfile *found = nullptr) const;
		bool isEmpty() const {return m_profiles.isEmpty();}

		QDomElement toXml(QDomDocument &document) const;
		static WireProfiles fromXml(const QDomElement &element);

		bool operator==(const WireProfiles &other) const;
		bool operator!=(const WireProfiles &other) const {return !(*this == other);}

	private:
		QList<WireProfile> m_profiles;
};

#endif // WIREPROFILES_H
