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
#include "railsnapdata.h"

#include <QRegularExpression>
#include <QtMath>

namespace {
	const QString tag_name{QStringLiteral("railSnap")};

	QString number(qreal value)
	{
		return QString::number(value, 'f', 3).remove(
			QRegularExpression(QStringLiteral("\\.?0+$")));
	}

	bool readNumber(const QDomElement &e, const QString &name,
					qreal fallback, qreal *value)
	{
		if (!e.hasAttribute(name)) {
			*value = fallback;
			return true;
		}
		bool ok{false};
		const qreal read = e.attribute(name).toDouble(&ok);
		if (!ok || !qIsFinite(read)) {
			return false;
		}
		*value = read;
		return true;
	}
}

/**
	@brief RailSnapData::fromXml
	@param definition : the <definition> element of a symbol
	@return true when the symbol declares a usable <railSnap>; otherwise
	this is left empty (a symbol which declares nothing, or something this
	version does not understand, snaps to nothing)
*/
bool RailSnapData::fromXml(const QDomElement &definition)
{
	*this = RailSnapData();
	const QDomElement e = definition.firstChildElement(tag_name);
	if (e.isNull()) {
		return false;
	}

	RailSnapData data;
	const QString role = e.attribute(QStringLiteral("role"));
	if (role == QLatin1String("rail")) {
		data.m_role = Role::Rail;
	} else if (role == QLatin1String("component")) {
		data.m_role = Role::Component;
	} else {
		return false;
	}

	const QString axis = e.attribute(QStringLiteral("axis"),
									 QStringLiteral("horizontal"));
	if (axis == QLatin1String("horizontal")) {
		data.m_axis = Qt::Horizontal;
	} else if (axis == QLatin1String("vertical")) {
		data.m_axis = Qt::Vertical;
	} else {
		return false;
	}

	const QString mount = e.attribute(QStringLiteral("mount"),
									  QStringLiteral("origin"));
	if (mount == QLatin1String("origin")) {
		data.m_mount = Mount::Origin;
	} else if (mount == QLatin1String("start")
			   || mount == QLatin1String("top")
			   || mount == QLatin1String("left")) {
		data.m_mount = Mount::Start;
	} else if (mount == QLatin1String("center")) {
		data.m_mount = Mount::Center;
	} else if (mount == QLatin1String("end")
			   || mount == QLatin1String("bottom")
			   || mount == QLatin1String("right")) {
		data.m_mount = Mount::End;
	} else {
		return false;
	}

	qreal x, y, width, height;
	if (!readNumber(e, QStringLiteral("x"), 0, &x)
		|| !readNumber(e, QStringLiteral("y"), 0, &y)
		|| !readNumber(e, QStringLiteral("width"), 0, &width)
		|| !readNumber(e, QStringLiteral("height"), 0, &height)
		|| width <= 0 || height <= 0) {
		return false;
	}
	data.m_body = QRectF(x, y, width, height);

	if (!readNumber(e, QStringLiteral("mount_offset_mm"), 0,
					&data.m_mount_offset_mm)
		|| !readNumber(e, QStringLiteral("gap_mm"), defaultGapMm,
					   &data.m_gap_mm)
		|| !readNumber(e, QStringLiteral("mm_per_px"), 1,
					   &data.m_mm_per_px)
		|| data.m_gap_mm < 0 || data.m_mm_per_px <= 0) {
		return false;
	}

	data.m_basis = e.attribute(QStringLiteral("basis"),
							   QStringLiteral("assumed"));
	data.m_source = e.attribute(QStringLiteral("source"));
	data.m_verified = e.attribute(QStringLiteral("verified"));
	*this = data;
	return true;
}

/**
	@brief RailSnapData::toXml
	@param document : owner of the element
	@return the <railSnap> element, null when the symbol declares nothing
*/
QDomElement RailSnapData::toXml(QDomDocument &document) const
{
	if (!isValid()) {
		return QDomElement();
	}
	static const char *mounts[] = {"origin", "start", "center", "end"};
	QDomElement e = document.createElement(tag_name);
	e.setAttribute(QStringLiteral("role"), isRail() ? QStringLiteral("rail")
												   : QStringLiteral("component"));
	e.setAttribute(QStringLiteral("axis"),
				   m_axis == Qt::Horizontal ? QStringLiteral("horizontal")
											: QStringLiteral("vertical"));
	e.setAttribute(QStringLiteral("x"), number(m_body.x()));
	e.setAttribute(QStringLiteral("y"), number(m_body.y()));
	e.setAttribute(QStringLiteral("width"), number(m_body.width()));
	e.setAttribute(QStringLiteral("height"), number(m_body.height()));
	e.setAttribute(QStringLiteral("mount"),
				   QLatin1String(mounts[static_cast<int>(m_mount)]));
	if (m_mount_offset_mm != 0) {
		e.setAttribute(QStringLiteral("mount_offset_mm"),
					   number(m_mount_offset_mm));
	}
	if (isComponent()) {
		e.setAttribute(QStringLiteral("gap_mm"), number(m_gap_mm));
	}
	e.setAttribute(QStringLiteral("mm_per_px"), number(m_mm_per_px));
	e.setAttribute(QStringLiteral("basis"), m_basis);
	if (!m_source.isEmpty()) {
		e.setAttribute(QStringLiteral("source"), m_source);
	}
	if (!m_verified.isEmpty()) {
		e.setAttribute(QStringLiteral("verified"), m_verified);
	}
	return e;
}

bool RailSnapData::operator==(const RailSnapData &other) const
{
	return m_role == other.m_role && m_axis == other.m_axis
		&& m_body == other.m_body && m_mount == other.m_mount
		&& m_mount_offset_mm == other.m_mount_offset_mm
		&& m_gap_mm == other.m_gap_mm
		&& m_mm_per_px == other.m_mm_per_px && m_basis == other.m_basis
		&& m_source == other.m_source && m_verified == other.m_verified;
}

/**
	@return true when the position of the fixing line is a guess: nothing
	tells where it comes from
*/
bool RailSnapData::isAssumed() const
{
	return m_basis == QLatin1String("assumed");
}

/**
	@param mm : a length in millimetres
	@return the same length in pixels of the symbol
*/
qreal RailSnapData::mmToPx(qreal mm) const
{
	return mm / m_mm_per_px;
}

/**
	@return the coordinate, across the rail and in the coordinates of the
	symbol, of the fixing line (y for a horizontal rail, x for a vertical one)
*/
qreal RailSnapData::fixCoordinate() const
{
	const bool horizontal = m_axis == Qt::Horizontal;
	const qreal first = horizontal ? m_body.top() : m_body.left();
	const qreal last = horizontal ? m_body.bottom() : m_body.right();
	qreal fix{0};
	switch (m_mount) {
		case Mount::Origin: fix = 0; break;
		case Mount::Start: fix = first; break;
		case Mount::Center: fix = (first + last) / 2; break;
		case Mount::End: fix = last; break;
	}
	return fix + mmToPx(m_mount_offset_mm);
}

/**
	@return the point of the fixing line which is the middle of the body,
	along the rail: where the marker of the symbol is drawn
*/
QPointF RailSnapData::fixPoint() const
{
	const QPointF middle = m_body.center();
	return m_axis == Qt::Horizontal ? QPointF(middle.x(), fixCoordinate())
									: QPointF(fixCoordinate(), middle.y());
}

/**
	@return the fixing line, over the extent of the body
*/
QLineF RailSnapData::fixLine() const
{
	const qreal fix = fixCoordinate();
	return m_axis == Qt::Horizontal
		? QLineF(m_body.left(), fix, m_body.right(), fix)
		: QLineF(fix, m_body.top(), fix, m_body.bottom());
}
