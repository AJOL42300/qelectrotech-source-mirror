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

#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../conductorproperties.h"
#include "../diagram.h"
#include "../diagramcommands.h"
#include "../diagramcontent.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/independenttextitem.h"
#include "../qetproject.h"
#include "../undocommand/addgraphicsobjectcommand.h"

#include <QCoreApplication>
#include <QDomDocument>
#include <QSet>
#include <QUndoCommand>

namespace {
		//The default stripe (1) is a dotted line, not stripes
	const int TWO_COLOUR_DASH_SIZE = 10;

	QString tr(const char *text)
	{
		return QCoreApplication::translate("WireProfiles", text);
	}
}

bool WireProfile::operator==(const WireProfile &other) const
{
	return name == other.name
			&& wire_color == other.wire_color
			&& color == other.color
			&& color_2 == other.color_2
			&& section == other.section
			&& label == other.label
			&& qFuzzyCompare(1 + width, 1 + other.width);
}

/**
	@brief WireProfiles::defaultProfiles
	@return the table a project starts with. The colours are the ones of
	the reference schematic of the team; where it gives no function the
	line follows IEC 60204-1 (13.2.4) or is a plain colour that can be
	given a meaning. See docs/wire-color-profiles.md.
*/
QList<WireProfile> WireProfiles::defaultProfiles()
{
	const QColor none;
	const QColor black(0x00, 0x00, 0x00);
	const QColor red(0xff, 0x00, 0x00);
	const QColor blue(0x00, 0x00, 0xff);
	const QColor light_blue(0x00, 0x55, 0xff);
	const QColor brown(0xaa, 0x55, 0x00);
	const QColor grey(0x78, 0x78, 0x78);
	const QColor light_grey(0x98, 0x98, 0x98);
	const QColor white(0xc5, 0xc5, 0xc5);
	const QColor pure_white(0xff, 0xff, 0xff);
	const QColor green(0x00, 0xaa, 0x00);
	const QColor yellow(0xff, 0xff, 0x00);
	const QColor orange(0xff, 0x80, 0x00);
	const QColor violet(0xaa, 0x00, 0xff);
	const QColor pink(0xff, 0x55, 0xff);

	const QString mm2_075 = QStringLiteral("0.75");
	const QString mm2_1 = QStringLiteral("1");

	return {
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "+24 V DC")), QStringLiteral("RD"), red, none, mm2_075, QStringLiteral("+24V")},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "0 V DC")), QStringLiteral("BU"), blue, none, mm2_075, QStringLiteral("0V")},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "Protective earth (PE)")), QStringLiteral("GNYE"), green, yellow, QString(), QStringLiteral("PE")},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "Power circuit L1")), QStringLiteral("BK"), black, none, QString(), QStringLiteral("L1")},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "Power circuit L2")), QStringLiteral("BN"), brown, none, QString(), QStringLiteral("L2")},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "Power circuit L3")), QStringLiteral("GY"), grey, none, QString(), QStringLiteral("L3")},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "Neutral (N)")), QStringLiteral("BU"), light_blue, none, QString(), QStringLiteral("N")},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "AC control circuit")), QStringLiteral("RD"), red, none, mm2_1, QString()},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "Circuit from an external source")), QStringLiteral("OG"), orange, none, QString(), QString()},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "Analogue signal")), QStringLiteral("VT"), violet, none, QString(), QString()},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "Bus (EtherCAT)")), QStringLiteral("YE"), yellow, none, QString(), QString()},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "White wire")), QStringLiteral("WH"), white, none, QString(), QString()},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "Pink wire")), QStringLiteral("PK"), pink, none, QString(), QString()},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "Yellow wire")), QStringLiteral("YE"), yellow, none, QString(), QString()},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "Green wire")), QStringLiteral("GN"), green, none, QString(), QString()},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "Light grey wire")), QStringLiteral("GY"), light_grey, none, QString(), QString()},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "White-grey wire")), QStringLiteral("WHGY"), pure_white, grey, QString(), QString()},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "White-green wire")), QStringLiteral("WHGN"), pure_white, green, QString(), QString()},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "White-yellow wire")), QStringLiteral("WHYE"), pure_white, QColor(0xde, 0xde, 0x00), QString(), QString()},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "Brown-grey wire")), QStringLiteral("BNGY"), brown, light_grey, QString(), QString()},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "Brown-green wire")), QStringLiteral("BNGN"), brown, green, QString(), QString()},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "Yellow-brown wire")), QStringLiteral("YEBN"), yellow, brown, QString(), QString()},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "Yellow-red wire")), QStringLiteral("YERD"), yellow, red, QString(), QString()},
		{tr(QT_TRANSLATE_NOOP("WireProfiles", "Black-red wire")), QStringLiteral("BKRD"), black, red, QString(), QString()}
	};
}

/**
	@param name : a function of the table
	@param found : if not null, the line is copied there when found
	@return true if the table has a line of that name
*/
bool WireProfiles::find(const QString &name, WireProfile *found) const
{
	if (name.isEmpty())
		return false;
	for (const WireProfile &profile : m_profiles) {
		if (profile.name == name) {
			if (found)
				*found = profile;
			return true;
		}
	}
	return false;
}

/**
	@return true when the table is the default one, so there is nothing
	to write and an old QET reads the file as it always did
*/
bool WireProfiles::isDefault() const
{
	return m_profiles == defaultProfiles();
}

QDomElement WireProfiles::toXml(QDomDocument &document) const
{
	QDomElement root = document.createElement(QStringLiteral("wireprofiles"));
	for (const WireProfile &profile : m_profiles) {
		QDomElement elmt = document.createElement(QStringLiteral("wireprofile"));
		elmt.setAttribute(QStringLiteral("name"), profile.name);
		elmt.setAttribute(QStringLiteral("wirecolor"), profile.wire_color);
		elmt.setAttribute(QStringLiteral("color"), profile.color.name());
		if (profile.isTwoColour())
			elmt.setAttribute(QStringLiteral("color2"), profile.color_2.name());
		elmt.setAttribute(QStringLiteral("section"), profile.section);
		elmt.setAttribute(QStringLiteral("label"), profile.label);
		if (profile.width > 0)
			elmt.setAttribute(QStringLiteral("width"), QString::number(profile.width));
		root.appendChild(elmt);
	}
	return root;
}

/**
	@param element : the <wireprofiles> element, may be null
	@return the table it holds. A null element is a project saved before
	the table existed: it gets the default one.
*/
WireProfiles WireProfiles::fromXml(const QDomElement &element)
{
	WireProfiles result;
	if (element.isNull())
		return result;

	result.m_profiles.clear();
	for (QDomElement elmt = element.firstChildElement(QStringLiteral("wireprofile"));
		 !elmt.isNull();
		 elmt = elmt.nextSiblingElement(QStringLiteral("wireprofile")))
	{
		WireProfile profile;
		profile.name = elmt.attribute(QStringLiteral("name"));
		if (profile.name.isEmpty() || result.find(profile.name))
			continue;
		profile.wire_color = elmt.attribute(QStringLiteral("wirecolor"));
		const QColor color(elmt.attribute(QStringLiteral("color")));
		profile.color = color.isValid() ? color : QColor(Qt::black);
		const QColor color_2(elmt.attribute(QStringLiteral("color2")));
		if (color_2.isValid())
			profile.color_2 = color_2;
		profile.section = elmt.attribute(QStringLiteral("section"));
		profile.label = elmt.attribute(QStringLiteral("label"));
			//Absent in a table saved before the width: left as it is
		const double width = elmt.attribute(QStringLiteral("width")).toDouble();
		profile.width = qIsFinite(width) && width > 0 ? width : 0;
		result.m_profiles << profile;
	}
	return result;
}

/**
	@brief WireProfiles::apply
	Give @p properties the profile: the stroke colour (striped for two
	colours), the function, the label and, unless the conductor is a core
	of a cable which gives it its own, the colour code and the section.
*/
void WireProfiles::apply(const WireProfile &profile, ConductorProperties &properties)
{
	properties.color = profile.color;
	properties.m_bicolor = profile.isTwoColour();
	if (profile.isTwoColour()) {
		properties.m_color_2 = profile.color_2;
		if (properties.m_dash_size < 2)
			properties.m_dash_size = TWO_COLOUR_DASH_SIZE;
	}
	properties.m_function = profile.name;
	if (profile.width > 0)
		properties.cond_size = profile.width;
	if (!profile.label.isEmpty()) {
		properties.text = profile.label;
		properties.m_show_text = true;
	}
	if (properties.m_cable_uuid.isNull()) {
		properties.m_wire_color = profile.wire_color;
		if (!profile.section.isEmpty())
			properties.m_wire_section = profile.section;
	}
}

/**
	@brief WireProfiles::legendHtml
	@return the table of the colour meaning as rich text: colour, section
	and function of every line of the table
*/
QString WireProfiles::legendHtml() const
{
	auto cell = [](const QColor &color) {
		return QStringLiteral("<span style=\"background-color:%1\">&nbsp;&nbsp;&nbsp;&nbsp;</span>")
				.arg(color.name());
	};
	QString html = QStringLiteral("<p><b>%1</b></p>"
								  "<table border=\"1\" cellspacing=\"0\" cellpadding=\"3\">"
								  "<tr><th>%2</th><th>%3</th><th>%4</th></tr>")
			.arg(QCoreApplication::translate("WireProfiles", "Wire colour meaning").toHtmlEscaped(),
				 QCoreApplication::translate("WireProfiles", "Colour").toHtmlEscaped(),
				 QCoreApplication::translate("WireProfiles", "Section").toHtmlEscaped(),
				 QCoreApplication::translate("WireProfiles", "Function").toHtmlEscaped());
	for (const WireProfile &profile : m_profiles) {
		QString swatch = cell(profile.color);
		if (profile.isTwoColour())
			swatch += cell(profile.color_2);
		const QString section = profile.section.isEmpty()
				? QString()
				: QCoreApplication::translate("WireProfiles", "%1 mm²").arg(profile.section);
		html += QStringLiteral("<tr><td>%1 %2</td><td>%3</td><td>%4</td></tr>")
				.arg(swatch, profile.wire_color.toHtmlEscaped(), section.toHtmlEscaped(),
					 profile.name.toHtmlEscaped());
	}
	return html + QStringLiteral("</table>");
}

/**
	@brief WireProfiles::addLegend
	Put the legend of the table of the project on @p diagram, as an
	independent text, as one undo step. It is a copy of the table at that
	moment: it is not updated when the table changes.
	@param position : where to put its top left corner
*/
void WireProfiles::addLegend(Diagram *diagram, const QPointF &position)
{
	if (!diagram || diagram->isReadOnly() || !diagram->project())
		return;
	IndependentTextItem *text = new IndependentTextItem();
	text->setHtml(diagram->project()->wireProfiles().legendHtml());
	diagram->undoStack().push(new AddGraphicsObjectCommand(text, diagram, position));
}

/**
	@brief WireProfiles::applyToSelection
	Apply @p profile to the selected conductors of @p diagram, and to the
	conductors on the same potential, as one undo step.
	@return true if at least one conductor changed
*/
bool WireProfiles::applyToSelection(Diagram *diagram, const WireProfile &profile)
{
	if (!diagram || diagram->isReadOnly())
		return false;

	DiagramContent dc(diagram);
	QSet<Conductor *> conductors;
	for (Conductor *conductor : dc.conductors(DiagramContent::AnyConductor)) {
		conductors << conductor;
		conductors += conductor->relatedPotentialConductors();
	}
	if (conductors.isEmpty())
		return false;

	QUndoCommand *undo = new QUndoCommand(
				QCoreApplication::translate("WireProfiles",
											"Apply the wire profile \"%1\" to %n conductors",
											"undo caption", conductors.count()).arg(profile.name));
	int changed = 0;
	for (Conductor *conductor : conductors) {
		ConductorProperties before = conductor->properties();
		ConductorProperties after = before;
		apply(profile, after);
		if (after == before)
			continue;
		QVariant old_value, new_value;
		old_value.setValue(before);
		new_value.setValue(after);
		new QPropertyUndoCommand(conductor, "properties", old_value, new_value, undo);
		++changed;
	}

	if (!changed) {
		delete undo;
		return false;
	}
	diagram->undoStack().push(undo);
	return true;
}
