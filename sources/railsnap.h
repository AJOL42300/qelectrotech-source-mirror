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
#ifndef RAILSNAP_H
#define RAILSNAP_H

#include "properties/railsnapdata.h"

#include <QLineF>
#include <QList>
#include <QPointF>
#include <QRectF>
#include <optional>

class Element;

/**
	@brief The RailSnap class
	Magnetism of a layout folio: a symbol which declares itself a component
	of a rail (see RailSnapData) is put on the axis of the nearest rail and
	against its nearest neighbour, with the free space its data asks for
	(2 mm by default), when it is put down or moved close enough.

	The calculation (RailSnap::compute) works on plain rectangles in the
	coordinates of the folio, so that it does not depend on any symbol;
	RailSnap::snap reads them from the symbols of a folio.
*/
class RailSnap
{
	public:
		static constexpr double defaultThresholdMm = 10.0;

			/// A symbol taking part in the calculation, in the coordinates
			/// of the folio
		struct Body
		{
			bool m_rail{false};
			Qt::Orientation m_axis{Qt::Horizontal};
			QRectF m_rect;
				/// Position, across the rail, of the fixing line
			qreal m_fix{0};
				/// Free space wanted along the rail, and distance within
				/// which the magnetism acts
			qreal m_gap{0};
			qreal m_threshold{0};
		};

			/// What to show while the magnetism acts, in the coordinates of
			/// the folio: the axis of the rail over the moved body, and
			/// the edge of the moved body which is against its neighbour
		struct Guide
		{
			QLineF m_axis;
			std::optional<QLineF> m_neighbour_edge;
		};

		struct Result
		{
			QPointF m_move;
			Guide m_guide;
		};

		static bool enabled();
		static void setEnabled(bool enabled);
		static double thresholdMm();
		static void setThresholdMm(double mm);
		static bool disabledBy(Qt::KeyboardModifiers modifiers);

		static std::optional<Result> compute(const Body &moving,
											 const QList<Body> &others);
		static std::optional<QPointF> snap(Element *element,
										   const QPointF &proposed,
										   Qt::KeyboardModifiers modifiers);
};

#endif // RAILSNAP_H
