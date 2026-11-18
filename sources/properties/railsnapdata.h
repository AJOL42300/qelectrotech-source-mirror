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
#ifndef RAILSNAPDATA_H
#define RAILSNAPDATA_H

#include <QDomDocument>
#include <QDomElement>
#include <QLineF>
#include <QPointF>
#include <QRectF>
#include <QString>

/**
	@brief The RailSnapData class
	What a symbol says about itself so that, in a layout folio, it can be
	snapped to a mounting rail: either it is a rail, or it is a component
	which is fixed on a rail. It is read from the optional
	<railSnap> child of the <definition> of a symbol, which older versions
	ignore, and it is kept (with the rest of the definition) when the
	symbol is saved by the symbol editor.

	Everything is drawn for a rail along the local x axis (axis
	"horizontal") or the local y axis (axis "vertical"); a symbol turned
	by a quarter turn simply fits the other direction. The lengths of the
	body are in pixels of the symbol, relative to its origin (hotspot);
	only the gap and the offset are in millimetres, which mm_per_px turns
	into pixels, so that the same 2 mm means the same on every scale.

	The fixing line of a component is the line which has to lie on the
	axis of the rail, perpendicular to the rail. It goes through the origin
	of the symbol (mount "origin", the default), or through the first edge,
	the middle or the last edge of the body, plus an offset.
*/
class RailSnapData
{
	public:
		enum class Role {None, Rail, Component};
		enum class Mount {Origin, Start, Center, End};

		static constexpr double defaultGapMm = 2.0;

		bool fromXml(const QDomElement &definition);
		QDomElement toXml(QDomDocument &document) const;
		bool operator==(const RailSnapData &other) const;
		bool operator!=(const RailSnapData &other) const
		{ return !(*this == other); }

		bool isValid() const { return m_role != Role::None; }
		bool isRail() const { return m_role == Role::Rail; }
		bool isComponent() const { return m_role == Role::Component; }
		bool isAssumed() const;

		Qt::Orientation axis() const { return m_axis; }
		qreal fixCoordinate() const;
		QPointF fixPoint() const;
		QLineF fixLine() const;
		qreal mmToPx(qreal mm) const;

			/// None when the symbol takes no part in rail snapping
		Role m_role{Role::None};
			/// Direction of the rail, in the coordinates of the symbol
		Qt::Orientation m_axis{Qt::Horizontal};
			/// What the symbol occupies on the rail, in pixels, relative
			/// to the origin of the symbol
		QRectF m_body;
		Mount m_mount{Mount::Origin};
			/// Shift of the fixing line, across the rail; positive goes
			/// towards the bottom (axis horizontal) or the right
		qreal m_mount_offset_mm{0};
			/// Free space wanted between two components, along the rail
		qreal m_gap_mm{defaultGapMm};
			/// Scale of the symbol: how many millimetres a pixel stands for
		qreal m_mm_per_px{1};
			/// Where the position of the fixing line comes from:
			/// "datasheet", "image", "standard" or "assumed" (the default
			/// when nothing is said)
		QString m_basis{QStringLiteral("assumed")};
		QString m_source;
			/// Date the position was checked against its source, empty
			/// when it was not
		QString m_verified;
};

#endif // RAILSNAPDATA_H
