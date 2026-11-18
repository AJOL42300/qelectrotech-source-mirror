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
#include "railsnap.h"

#include "diagram.h"
#include "qetgraphicsitem/element.h"

#include <QSettings>
#include <QTransform>
#include <cmath>
#include <limits>
#include <utility>

namespace {
	const QString enabled_key{QStringLiteral("diagrameditor/rail_snap")};
	const QString threshold_key{
		QStringLiteral("diagrameditor/rail_snap_threshold_mm")};
	constexpr qreal epsilon = 1e-6;
	std::optional<bool> cached_enabled;

	qreal first(const QRectF &r, Qt::Orientation axis)
	{ return axis == Qt::Horizontal ? r.left() : r.top(); }
	qreal last(const QRectF &r, Qt::Orientation axis)
	{ return axis == Qt::Horizontal ? r.right() : r.bottom(); }
	qreal firstAcross(const QRectF &r, Qt::Orientation axis)
	{ return axis == Qt::Horizontal ? r.top() : r.left(); }
	qreal lastAcross(const QRectF &r, Qt::Orientation axis)
	{ return axis == Qt::Horizontal ? r.bottom() : r.right(); }

	QPointF pointOf(Qt::Orientation axis, qreal along, qreal across)
	{
		return axis == Qt::Horizontal ? QPointF(along, across)
									  : QPointF(across, along);
	}

		//The body of a symbol, as it stands on the folio when the symbol
		//is moved by @a transform. Nothing when the symbol is not turned
		//by a multiple of a quarter turn: it has no rail direction.
	std::optional<RailSnap::Body> bodyOf(const RailSnapData &data,
										 const QTransform &transform)
	{
		const QPointF origin = transform.map(QPointF(0, 0));
		const QPointF direction = transform.map(
			data.axis() == Qt::Horizontal ? QPointF(1, 0) : QPointF(0, 1))
			- origin;
		RailSnap::Body body;
		if (std::abs(direction.y()) < epsilon
			&& std::abs(direction.x()) > epsilon) {
			body.m_axis = Qt::Horizontal;
		} else if (std::abs(direction.x()) < epsilon
				   && std::abs(direction.y()) > epsilon) {
			body.m_axis = Qt::Vertical;
		} else {
			return std::nullopt;
		}
		body.m_rail = data.isRail();
		body.m_rect = transform.mapRect(data.m_body);
		const QPointF fix = transform.map(data.fixPoint());
		body.m_fix = body.m_axis == Qt::Horizontal ? fix.y() : fix.x();
		body.m_gap = data.mmToPx(data.m_gap_mm);
		body.m_threshold = data.mmToPx(RailSnap::thresholdMm());
		return body;
	}
}

bool RailSnap::enabled()
{
		//Read once: it is asked for at every repaint of a component
	if (!cached_enabled) {
		cached_enabled = QSettings().value(enabled_key, false).toBool();
	}
	return *cached_enabled;
}

void RailSnap::setEnabled(bool enabled)
{
	QSettings().setValue(enabled_key, enabled);
	cached_enabled = enabled;
}

double RailSnap::thresholdMm()
{
	const double mm = QSettings().value(threshold_key, defaultThresholdMm)
			.toDouble();
	return mm > 0 ? mm : defaultThresholdMm;
}

void RailSnap::setThresholdMm(double mm)
{
	QSettings().setValue(threshold_key, mm);
}

/**
	@return true when a modifier key held by the user asks for no
	magnetism for the time of the gesture: Alt or Shift
*/
bool RailSnap::disabledBy(Qt::KeyboardModifiers modifiers)
{
	return modifiers & (Qt::AltModifier | Qt::ShiftModifier);
}

/**
	@brief RailSnap::compute
	@param moving : the component which is moved, where the user brings it
	@param others : everything else on the folio
	@return what to add to the position of @a moving so that it lies on the
	axis of the nearest rail within reach, and against the nearest of the
	components fixed on that rail on its side, leaving the gap it asks for.
	Nothing when no rail is within reach: a component far from every rail
	is not touched, neither along nor across.
*/
std::optional<RailSnap::Result> RailSnap::compute(const Body &moving,
												  const QList<Body> &others)
{
	const Qt::Orientation axis = moving.m_axis;
	const qreal reach = moving.m_threshold;

	const Body *rail{nullptr};
	qreal nearest = std::numeric_limits<qreal>::max();
	for (const Body &other : others) {
		if (!other.m_rail || other.m_axis != axis) {
			continue;
		}
		if (last(moving.m_rect, axis) + reach < first(other.m_rect, axis)
			|| first(moving.m_rect, axis) - reach > last(other.m_rect, axis)) {
			continue;
		}
		const qreal distance = std::abs(other.m_fix - moving.m_fix);
		if (distance <= reach && distance < nearest) {
			nearest = distance;
			rail = &other;
		}
	}
	if (!rail) {
		return std::nullopt;
	}

	const qreal across = rail->m_fix - moving.m_fix;
	QRectF placed = moving.m_rect;
	placed.translate(pointOf(axis, 0, across));

	QList<const Body *> neighbours;
	for (const Body &other : others) {
		if (!other.m_rail && other.m_axis == axis
			&& std::abs(other.m_fix - rail->m_fix) <= reach) {
			neighbours << &other;
		}
	}

	qreal shift{0};
	std::optional<bool> against_first_edge;
	qreal best = std::numeric_limits<qreal>::max();
	for (const Body *n : std::as_const(neighbours)) {
		const qreal after = last(n->m_rect, axis) + moving.m_gap;
		const qreal before = first(n->m_rect, axis) - moving.m_gap;
		const struct {qreal m_shift; bool m_first_edge;} candidates[] = {
			{after - first(placed, axis), true},
			{before - last(placed, axis), false}
		};
		for (const auto &c : candidates) {
			if (std::abs(c.m_shift) > reach
				|| std::abs(c.m_shift) >= best) {
				continue;
			}
			QRectF result = placed;
			result.translate(pointOf(axis, c.m_shift, 0));
				//Not on top of another component: that is no way to
				//stand against a neighbour
			bool free{true};
			for (const Body *o : std::as_const(neighbours)) {
				if (o != n
					&& first(result, axis) < last(o->m_rect, axis) - epsilon
					&& last(result, axis) > first(o->m_rect, axis) + epsilon) {
					free = false;
					break;
				}
			}
			if (free) {
				best = std::abs(c.m_shift);
				shift = c.m_shift;
				against_first_edge = c.m_first_edge;
			}
		}
	}

	placed.translate(pointOf(axis, shift, 0));
	Result result;
	result.m_move = pointOf(axis, shift, across);
	result.m_guide.m_axis = QLineF(
		pointOf(axis, first(placed, axis), rail->m_fix),
		pointOf(axis, last(placed, axis), rail->m_fix));
	if (against_first_edge) {
		const qreal edge = *against_first_edge ? first(placed, axis)
											   : last(placed, axis);
		result.m_guide.m_neighbour_edge = QLineF(
			pointOf(axis, edge, firstAcross(placed, axis)),
			pointOf(axis, edge, lastAcross(placed, axis)));
	}
	return result;
}

/**
	@brief RailSnap::snap
	@param element : a symbol which is put down or moved
	@param proposed : where the user brings the origin of @a element
	@param modifiers : keys held by the user
	@return the position to give to @a element instead of @a proposed, or
	nothing when the magnetism does not apply: it is off, a key suspends
	it, @a element is no component of a rail, or no rail is within reach.
	The components which are selected together with @a element move with
	it, so they are not what @a element stands against.
*/
std::optional<QPointF> RailSnap::snap(Element *element,
									  const QPointF &proposed,
									  Qt::KeyboardModifiers modifiers)
{
	if (!element) {
		return std::nullopt;
	}
	element->setRailGuide(std::nullopt);
	if (!enabled() || disabledBy(modifiers)) {
		return std::nullopt;
	}
	const RailSnapData &data = element->railSnapData();
	Diagram *diagram = element->diagram();
	if (!data.isComponent() || !diagram) {
		return std::nullopt;
	}

	QTransform transform = element->sceneTransform();
	transform *= QTransform::fromTranslate(
		proposed.x() - element->pos().x(), proposed.y() - element->pos().y());
	const std::optional<Body> moving = bodyOf(data, transform);
	if (!moving) {
		return std::nullopt;
	}

	QList<Body> others;
	const QList<Element *> elements = diagram->elements();
	for (const Element *other : elements) {
		const RailSnapData &other_data = other->railSnapData();
		if (other == element || !other_data.isValid() || !other->isVisible()
			|| (element->isSelected() && other->isSelected())) {
			continue;
		}
		if (const auto body = bodyOf(other_data, other->sceneTransform())) {
			others << *body;
		}
	}

	const std::optional<Result> result = compute(*moving, others);
	if (!result) {
		return std::nullopt;
	}
	element->setRailGuide(result->m_guide);
	return proposed + result->m_move;
}
