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
#ifndef WIRECOLOURTABLE_H
#define WIRECOLOURTABLE_H

#include <QColor>
#include <QDomElement>
#include <QList>
#include <QPointF>
#include <QString>

class ConductorProperties;
class Diagram;

/**
	@brief The WireColourEntry struct
	One line of the wire colour meaning table: a function (+24 V, PE...)
	and how a wire that carries it is drawn and labelled.
*/
struct WireColourEntry
{
		/// The function, shown in the lists and the legend
	QString name;
		/// The colour code written on the wire (IEC 60757: RD, GNYE...)
	QString wire_color;
		/// The colour of the stroke
	QColor color = Qt::black;
		/// The second colour of a two-colour wire, invalid when single
	QColor color_2;
		/// The section as text, for example "0.75", empty when not set
	QString section;
		/// The text written on the conductor, empty to leave it as it is
	QString label;

	bool isTwoColour() const {return color_2.isValid();}
	bool operator==(const WireColourEntry &other) const;
	bool operator!=(const WireColourEntry &other) const {return !(*this == other);}
};

/**
	@brief The WireColourTable class
	The wire colour meaning table of a project. A project saved without
	one has the default table.
*/
class WireColourTable
{
	public:
		const QList<WireColourEntry> &profiles() const {return m_profiles;}
		void setProfiles(const QList<WireColourEntry> &profiles) {m_profiles = profiles;}
		bool find(const QString &name, WireColourEntry *found = nullptr) const;

		static QList<WireColourEntry> defaultProfiles();

		QDomElement toXml(QDomDocument &document) const;
		static WireColourTable fromXml(const QDomElement &element);
		QString legendHtml() const;
		static void addLegend(Diagram *diagram, const QPointF &position);

		bool isDefault() const;
		bool operator==(const WireColourTable &other) const {return m_profiles == other.m_profiles;}
		bool operator!=(const WireColourTable &other) const {return !(*this == other);}

		static void apply(const WireColourEntry &profile, ConductorProperties &properties);
		static bool applyToSelection(Diagram *diagram, const WireColourEntry &profile);

	private:
		QList<WireColourEntry> m_profiles = defaultProfiles();
};

#endif // WIRECOLOURTABLE_H
