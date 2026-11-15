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
#ifndef NETWORKCABLE_H
#define NETWORKCABLE_H

#include <QColor>
#include <QList>
#include <QPainterPath>
#include <QString>

class ConductorProperties;
class QPainter;

/**
	@brief Network and moulded cables drawn as one conductor.

	A moulded cable (EtherCAT, EtherCAT P, M8 or M12 sensor cable, motor
	cable) joins two devices as one single link: a reference and a length,
	connectors included, with no cores to wire. It is a plain conductor
	which carries a cable type, a length and, for a Y cable, a branch
	mark. This namespace knows the catalogue of types and how a conductor
	of such a type is drawn: its sheath colour, a connector at each end and
	the folio it goes on to when it ends on a folio report.

	Nothing here is written to a project unless a conductor is given a
	type, so projects which do not use it are saved exactly as before.
*/
namespace NetworkCables
{
	enum class Connector { None, RJ45, M8, M12 };

	struct Type
	{
			/// What the project file stores
		QString id;
			/// Name shown to the user, also the default designation
		QString name;
		QColor color;
			/// Second colour of a two-coloured sheath, invalid when none
		QColor color_2;
		Connector connector = Connector::None;
			/// Number of pins of the connector, 0 when it has none
		int pins = 0;
			/// A Y cable: one connector on the box side, two branches
		bool y_cable = false;
	};

	struct Total
	{
		QString id;
		int count = 0;
		double metres = 0;
		int connectors = 0;
	};

	struct ReferenceTotal
	{
		QString reference;
		QString designation;
		int count = 0;
		double metres = 0;
	};

	const QList<Type> &types();
	const Type *find(const QString &id);

	void apply(ConductorProperties &properties, const QString &id);
	QString label(const ConductorProperties &properties);
	double lengthInMetres(const QString &length);
	int connectorCount(const ConductorProperties &properties);
	QList<Total> totals(const QList<ConductorProperties> &conductors);
	QList<ReferenceTotal> referenceTotals(
		const QList<ConductorProperties> &conductors);

	void paintEnds(QPainter *painter,
		       const QPainterPath &path,
		       const ConductorProperties &properties,
		       const QColor &ink,
		       bool plug_at_start,
		       bool plug_at_end,
		       const QString &start_reference,
		       const QString &end_reference);
}

#endif // NETWORKCABLE_H
