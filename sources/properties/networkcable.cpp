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
#include "networkcable.h"

#include "../conductorproperties.h"

#include <QCoreApplication>
#include <QFont>
#include <QMap>
#include <QPainter>
#include <QRegularExpression>
#include <QStringList>
#include <QtMath>

namespace NetworkCables
{
static QString tr(const char *text)
{
	return QCoreApplication::translate("NetworkCables", text);
}

/**
	@return the catalogue of cable types. Their ids are stored in the
	project files and must never change.
*/
const QList<Type> &types()
{
	static const QList<Type> list = []() {
		QList<Type> l;
		auto add = [&l](const QString &id, const QString &name,
				const QColor &color, Connector connector,
				int pins, bool y = false,
				const QColor &color_2 = QColor()) {
			Type t;
			t.id = id;
			t.name = name;
			t.color = color;
			t.color_2 = color_2;
			t.connector = connector;
			t.pins = pins;
			t.y_cable = y;
			l << t;
		};
		add(QStringLiteral("ethernet"), tr("Ethernet"),
		    QColor(30, 90, 200), Connector::RJ45, 8);
		add(QStringLiteral("ethercat"), tr("EtherCAT"),
		    QColor(0, 150, 60), Connector::RJ45, 8);
		add(QStringLiteral("ethercat_p"), tr("EtherCAT P"),
		    QColor(220, 0, 0), Connector::M8, 4, false,
		    QColor(0, 0, 0));
		add(QStringLiteral("m12_4"), tr("M12 cable, 4 pins"),
		    QColor(120, 120, 120), Connector::M12, 4);
		add(QStringLiteral("m12_5"), tr("M12 cable, 5 pins"),
		    QColor(120, 120, 120), Connector::M12, 5);
		add(QStringLiteral("m12_8"), tr("M12 cable, 8 pins"),
		    QColor(120, 120, 120), Connector::M12, 8);
		add(QStringLiteral("y_m12"), tr("M12 Y cable"),
		    QColor(120, 120, 120), Connector::M12, 5, true);
		add(QStringLiteral("motor"), tr("Motor cable"),
		    QColor(0, 0, 0), Connector::None, 0);
		return l;
	}();
	return list;
}

const Type *find(const QString &id)
{
	for (const Type &t : types()) {
		if (t.id == id) {
			return &t;
		}
	}
	return nullptr;
}

/**
	Makes a conductor the cable of type @p id: its sheath colour, its
	width and its designation. An empty or unknown id turns it back into
	an ordinary conductor, whose colour is left as it is.
*/
void apply(ConductorProperties &properties, const QString &id)
{
	const Type *type = find(id);
	if (!type) {
		properties.m_network_cable.clear();
		properties.m_network_length.clear();
		properties.m_network_branch.clear();
		return;
	}
	properties.m_network_cable = type->id;
	properties.color = type->color;
	properties.cond_size = 2;
	properties.m_bicolor = type->color_2.isValid();
	if (properties.m_bicolor) {
		properties.m_color_2 = type->color_2;
	}
	if (!type->y_cable) {
		properties.m_network_branch.clear();
	}
		//A motor cable is named by what is written on it
	if (type->id != QLatin1String("motor")) {
		properties.m_conductor_type = type->name;
	}
}

/**
	@return the length in metres of a free text such as "20", "1,5" or
	"20 m", or 0 when the text is not a length.
*/
double lengthInMetres(const QString &length)
{
	static const QRegularExpression re(
		QStringLiteral("^\\s*([0-9]+(?:[.,][0-9]+)?)\\s*m?\\s*$"));
	const QRegularExpressionMatch match = re.match(length);
	if (!match.hasMatch()) {
		return 0;
	}
	QString number = match.captured(1);
	number.replace(',', '.');
	return number.toDouble();
}

/**
	@return how many connectors this conductor stands for. Both ends of
	a cable have one; the second branch of a Y cable shares the one on
	the box side with the first.
*/
int connectorCount(const ConductorProperties &properties)
{
	const Type *type = find(properties.m_network_cable);
	if (!type || type->connector == Connector::None) {
		return 0;
	}
	return properties.m_network_branch == QLatin1String("2") ? 1 : 2;
}

/**
	@return the text written along a cable: branch mark, designation and
	length, for example "(1) M12 Y cable 5 m". Empty for an ordinary
	conductor.
*/
QString label(const ConductorProperties &properties)
{
	if (!find(properties.m_network_cable)) {
		return QString();
	}
	QStringList parts;
	if (!properties.m_network_branch.isEmpty()) {
		parts << QStringLiteral("(%1)").arg(properties.m_network_branch);
	}
	if (!properties.m_conductor_type.isEmpty()) {
		parts << properties.m_conductor_type;
	}
	if (!properties.m_network_length.trimmed().isEmpty()) {
		const QString length = properties.m_network_length.trimmed();
		parts << (lengthInMetres(length) > 0 || length == QLatin1String("0")
			  ? length + QStringLiteral(" m") : length);
	}
	return parts.join(QLatin1Char(' '));
}

/**
	@return per cable type, how many cables, how many metres and how many
	connectors the given conductors hold. Ordinary conductors are ignored.
*/
QList<Total> totals(const QList<ConductorProperties> &conductors)
{
	QMap<QString, Total> by_type;
	for (const ConductorProperties &properties : conductors) {
		if (!find(properties.m_network_cable)) {
			continue;
		}
		Total &total = by_type[properties.m_network_cable];
		total.id = properties.m_network_cable;
		++total.count;
		total.metres += lengthInMetres(properties.m_network_length);
		total.connectors += connectorCount(properties);
	}
	QList<Total> result;
	for (const Type &type : types()) {
		if (by_type.contains(type.id)) {
			result << by_type.value(type.id);
		}
	}
	return result;
}

	//Draws the plug of one end. The path runs away from it along +x.
static void paintPlug(QPainter *painter, Connector connector, int pins,
		      const QColor &ink)
{
	painter->setPen(QPen(ink, 0.8));
	painter->setBrush(QColor(235, 235, 235));
	if (connector == Connector::RJ45) {
		painter->drawRect(QRectF(0, -4, 10, 8));
		painter->setBrush(ink);
		painter->drawRect(QRectF(7, -2, 3, 4));
		return;
	}
	const qreal radius = connector == Connector::M12 ? 5 : 3.5;
	painter->drawEllipse(QPointF(radius, 0), radius, radius);
	painter->setBrush(ink);
	const int shown = qMin(pins, 8);
	for (int i = 0; i < shown; ++i) {
		const qreal a = (6.2831853 * i) / shown;
		painter->drawEllipse(
			QPointF(radius + 0.55 * radius * qCos(a),
				0.55 * radius * qSin(a)),
			0.5, 0.5);
	}
}

/**
	Draws what makes a conductor a cable: a connector at each end and its
	label. An end on a folio report has no connector: the cable goes on
	on the other folio, which is written there instead.
	@param plug_at_start draw the connector at the start of the path
	@param plug_at_end draw the connector at the end of the path
	@param start_reference text written instead of the connector at the
	start, for example "Folio 37"
	@param end_reference same at the end
	The painter keeps its state.
*/
void paintEnds(QPainter *painter,
	       const QPainterPath &path,
	       const ConductorProperties &properties,
	       const QColor &ink,
	       bool plug_at_start,
	       bool plug_at_end,
	       const QString &start_reference,
	       const QString &end_reference)
{
	const Type *type = find(properties.m_network_cable);
	if (!type || path.isEmpty()) {
		return;
	}
	painter->save();
	painter->setRenderHint(QPainter::Antialiasing, true);
	QFont font = painter->font();
	font.setPointSizeF(6);
	painter->setFont(font);
	const QFontMetricsF metrics(font);

	auto end = [&](qreal percent, bool reversed, bool plug,
		       const QString &reference) {
		const QPointF point = path.pointAtPercent(percent);
		const qreal angle = -path.angleAtPercent(percent)
				    + (reversed ? 180 : 0);
		if (plug && type->connector != Connector::None) {
			painter->save();
			painter->translate(point);
			painter->rotate(angle);
			paintPlug(painter, type->connector, type->pins, ink);
			painter->restore();
		}
		if (!reference.isEmpty()) {
			painter->setPen(ink);
			painter->drawText(
				QPointF(point.x() - metrics.horizontalAdvance(
						reference) / 2,
					point.y() + 12),
				reference);
		}
	};
	end(0, false, plug_at_start, start_reference);
	end(1, true, plug_at_end, end_reference);

	const QString text = label(properties);
	if (!text.isEmpty()) {
		const QPointF middle = path.pointAtPercent(0.5);
		painter->setPen(ink);
		painter->drawText(
			QPointF(middle.x() - metrics.horizontalAdvance(text) / 2,
				middle.y() - 5),
			text);
	}
	painter->restore();
}
}
