/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef TERMINALMAGNET_H
#define TERMINALMAGNET_H

#include <QList>
#include <QPointF>
#include <QtMath>

/**
	@brief The TerminalMagnet namespace holds the pure geometry used to
	"magnetize" terminals: when elements are dropped, the move is
	corrected so that a free terminal of a moved element meets a facing
	free terminal of another element (for example a pluggable terminal
	block that is plugged on its base).
	It does not depend on the diagram classes so it can be unit tested.
*/
namespace TerminalMagnet
{
	/// Maximum distance (scene units) at which terminals attract.
	constexpr qreal tolerance = 12.0;

	/// Orientation values are those of QET::Orientation (0..3).
	inline bool areFacing(int a, int b)
	{
		return ((a + 2) % 4) == b;
	}

	struct Dock
	{
		QPointF pos; ///< scene position of the conductor docking point
		int orientation = 0; ///< scene orientation, 0..3 (N, E, S, W)
	};

	/**
		@brief correction
		Search the closest pair (moved, fixed) of facing docks within
		\p max_distance.
		@param moved docks of the terminals of the moved elements, at
		their final position
		@param fixed docks of the candidate terminals
		@param max_distance attraction radius
		@param correction [out] the translation to apply to the moved
		items so the best pair coincides
		@return true if a pair was found
	*/
	inline bool correction(const QList<Dock> &moved,
			       const QList<Dock> &fixed,
			       qreal max_distance,
			       QPointF &correction)
	{
		bool found = false;
		qreal best = max_distance;
		for (const Dock &m : moved)
		{
			for (const Dock &f : fixed)
			{
				if (!areFacing(m.orientation, f.orientation))
					continue;
				const QPointF delta = f.pos - m.pos;
				const qreal dist = qSqrt(
					delta.x()*delta.x() + delta.y()*delta.y());
				if (dist <= best)
				{
					best = dist;
					correction = delta;
					found = true;
				}
			}
		}
		return found;
	}
}

#endif // TERMINALMAGNET_H
