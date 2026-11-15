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
#include "wireprofiles.h"

#include <QDomDocument>

#include <conductorcolorcodes.h>

bool WireProfile::operator==(const WireProfile &other) const
{
	return name == other.name
			&& color == other.color
			&& section == other.section;
}

/**
	@brief WireProfiles::colouredStrands
	The strands of a multi-pair cable: one profile per single-colour IEC
	code, named "<code> <section>" ("BN 0.5").
	@param section : the section of the strands, as written in the profiles
*/
QList<WireProfile> WireProfiles::colouredStrands(const QString &section)
{
	QList<WireProfile> list;
	for (const auto &code : ConductorColorCodes::all()) {
		if (!code.isTwoColour()) {
			list << WireProfile{QStringLiteral("%1 %2").arg(code.code, section),
								code.code, section};
		}
	}
	return list;
}

/**
	@brief WireProfiles::dinStrands
	The strands of a multi-pair cable numbered after DIN 47100: one
	profile per strand, named "DIN <number> <code> <section>"
	("DIN 12 RDBU 0.5").
	@param section : the section of the strands, as written in the profiles
*/
QList<WireProfile> WireProfiles::dinStrands(const QString &section)
{
	QList<WireProfile> list;
	for (const auto &code : ConductorColorCodes::din47100()) {
		list << WireProfile{QStringLiteral("DIN %1 %2 %3").arg(
								QString::number(code.strand), code.code, section),
							code.code, section};
	}
	return list;
}

/**
	@brief WireProfiles::find
	@param name : the name a conductor carries
	@param found : if not null, the profile is copied there when found
	@return true if the project has a profile of that name
*/
bool WireProfiles::find(const QString &name, WireProfile *found) const
{
	if (name.isEmpty()) {
		return false;
	}
	for (const WireProfile &profile : m_profiles) {
		if (profile.name == name) {
			if (found) {
				*found = profile;
			}
			return true;
		}
	}
	return false;
}

bool WireProfiles::operator==(const WireProfiles &other) const
{
	return m_profiles == other.m_profiles;
}

QDomElement WireProfiles::toXml(QDomDocument &document) const
{
	QDomElement root = document.createElement(QStringLiteral("wire_profiles"));
	for (const WireProfile &profile : m_profiles) {
		QDomElement elmt = document.createElement(QStringLiteral("wire_profile"));
		elmt.setAttribute(QStringLiteral("name"), profile.name);
		elmt.setAttribute(QStringLiteral("color"), profile.color);
		elmt.setAttribute(QStringLiteral("section"), profile.section);
		root.appendChild(elmt);
	}
	return root;
}

/**
	@brief WireProfiles::fromXml
	@param element : the <wire_profiles> element, may be null
	@return the profiles it holds. A null element gives a project
	without profiles. A profile without a name, or whose name is already
	taken, is left out: a conductor names its profile.
*/
WireProfiles WireProfiles::fromXml(const QDomElement &element)
{
	WireProfiles result;
	if (element.isNull()) {
		return result;
	}
	for (QDomElement elmt = element.firstChildElement(QStringLiteral("wire_profile"));
		 !elmt.isNull();
		 elmt = elmt.nextSiblingElement(QStringLiteral("wire_profile")))
	{
		WireProfile profile;
		profile.name = elmt.attribute(QStringLiteral("name"));
		if (profile.name.isEmpty() || result.find(profile.name)) {
			continue;
		}
		profile.color = elmt.attribute(QStringLiteral("color"));
		profile.section = elmt.attribute(QStringLiteral("section"));
		result.m_profiles << profile;
	}
	return result;
}
