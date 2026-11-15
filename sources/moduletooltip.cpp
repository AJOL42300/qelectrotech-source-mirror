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
#include "moduletooltip.h"

#include <QCoreApplication>
#include <QMap>

namespace {

QString esc(const QString &text)
{
	return text.toHtmlEscaped();
}

QString tr(const char *text)
{
	return QCoreApplication::translate("ModuleTooltip", text);
}

/// The other terminals that share the potential of @p pin.
QStringList linkedTo(const ModuleTooltip::Pin &pin,
		     const QList<ModuleTooltip::Pin> &pins)
{
	QStringList names;
	if (pin.potential.isEmpty()) return names;
	for (const ModuleTooltip::Pin &other : pins) {
		if (other.potential == pin.potential && other.name != pin.name)
			names << other.name;
	}
	return names;
}

QString entryText(const ElementData::PlcIO &io)
{
	QStringList parts;
	parts << QStringLiteral("<b>%1</b>").arg(esc(io.address));
	parts << ModuleTooltip::shortType(io.type);
	if (!io.functionText.isEmpty()) parts << esc(io.functionText);
	if (!io.comment.isEmpty())
		parts << QStringLiteral("(%1)").arg(esc(io.comment));
	if (!io.crossRef.isEmpty())
		parts << QStringLiteral("&rarr; %1").arg(esc(io.crossRef));
	return parts.join(QLatin1Char(' '));
}

QString whereText(const ModuleTooltip::Pin &pin)
{
	QStringList parts;
	if (!pin.wires.isEmpty())
		parts << esc(pin.wires.join(QStringLiteral(", ")));
	if (!pin.folios.isEmpty())
		parts << QStringLiteral("(%1)")
			.arg(esc(pin.folios.join(QStringLiteral(", "))));
	return parts.join(QLatin1Char(' '));
}

}

bool ModuleTooltip::isUsed(const Pin &pin,
			   const QVector<ElementData::PlcIO> &ios)
{
	if (!pin.wires.isEmpty() || !pin.folios.isEmpty()) return true;
	for (const ElementData::PlcIO &io : entriesOf(pin.name, ios)) {
		if (!io.crossRef.isEmpty()) return true;
	}
	return false;
}

QList<ElementData::PlcIO> ModuleTooltip::entriesOf(
	const QString &pin_name,
	const QVector<ElementData::PlcIO> &ios)
{
	QList<ElementData::PlcIO> found;
	if (pin_name.isEmpty()) return found;
	const QString prefix = pin_name + QLatin1Char('(');
	for (const ElementData::PlcIO &io : ios) {
		if (io.address == pin_name || io.address.startsWith(prefix))
			found << io;
	}
	return found;
}

QString ModuleTooltip::shortType(ElementData::PlcIOType type)
{
	switch (type) {
	case ElementData::EntreeDigitale: return QStringLiteral("DI");
	case ElementData::SortieDigitale: return QStringLiteral("DO");
	case ElementData::EntreeAnalogique: return QStringLiteral("AI");
	case ElementData::SortieAnalogique: return QStringLiteral("AO");
	case ElementData::EntreeUniverselle: return QStringLiteral("UI");
	case ElementData::SortieUniverselle: return QStringLiteral("UO");
	}
	return QString();
}

QString ModuleTooltip::pinText(const Pin &pin,
			       const QList<Pin> &pins,
			       const QVector<ElementData::PlcIO> &ios)
{
	QStringList parts;
	parts << QStringLiteral("<b>%1</b>").arg(esc(pin.name));
	const QList<ElementData::PlcIO> entries = entriesOf(pin.name, ios);
	for (const ElementData::PlcIO &io : entries)
		parts << entryText(io);
	const QStringList links = linkedTo(pin, pins);
	if (!pin.potential.isEmpty()) {
		parts << (links.isEmpty()
			  ? tr("potential %1").arg(esc(pin.potential))
			  : tr("potential %1, linked to %2")
				.arg(esc(pin.potential), esc(links.join(QStringLiteral(", ")))));
	}
	for (const QString &wire : pin.wires)
		parts << QStringLiteral("&rarr; %1").arg(esc(wire));
	if (!pin.folios.isEmpty())
		parts << esc(pin.folios.join(QStringLiteral(", ")));
	if (pin.potential.isEmpty())
		parts << esc(isUsed(pin, ios) ? tr("used") : tr("free"));
	return parts.join(QStringLiteral("<br>"));
}

QString ModuleTooltip::moduleText(const QString &title,
				  const QList<Pin> &pins,
				  const QVector<ElementData::PlcIO> &ios)
{
	QStringList rows;
	QStringList used;
	QStringList free_pins;
	for (const Pin &pin : pins) {
		const QList<ElementData::PlcIO> entries = entriesOf(pin.name, ios);
		const bool taken = isUsed(pin, ios);
		if (pin.potential.isEmpty() && !taken && !pin.name.isEmpty())
			free_pins << pin.name;
		if (entries.isEmpty() && pin.potential.isEmpty() && !taken)
			continue;
		QStringList signal;
		for (const ElementData::PlcIO &io : entries) {
			signal << entryText(io);
			used << io.address;
		}
		rows << QStringLiteral("<tr><td><b>%1</b>&nbsp;</td><td>%2&nbsp;</td><td>%3</td></tr>")
			.arg(esc(pin.name),
			     signal.join(QStringLiteral("<br>")),
			     whereText(pin));
	}

	// Channels of the I/O table that no terminal carries
	QStringList loose;
	for (const ElementData::PlcIO &io : ios) {
		if (!used.contains(io.address)) loose << entryText(io);
	}

	// Groups of terminals linked inside the module
	QMap<QString, QStringList> groups;
	for (const Pin &pin : pins) {
		if (!pin.potential.isEmpty()) groups[pin.potential] << pin.name;
	}
	QStringList links;
	for (auto it = groups.constBegin(); it != groups.constEnd(); ++it) {
		if (it.value().size() > 1)
			links << QStringLiteral("<b>%1</b>: %2").arg(
				esc(it.key()), esc(it.value().join(QStringLiteral(" = "))));
	}

	if (rows.isEmpty() && loose.isEmpty() && links.isEmpty())
		return QString();

	QString html = QStringLiteral("<p style='white-space:pre'><b>%1</b></p>").arg(esc(title));
	if (!rows.isEmpty()) {
		html += QStringLiteral("<table>%1</table>")
				.arg(rows.join(QString()));
	}
	if (!links.isEmpty()) {
		html += QStringLiteral("<p>%1<br>%2</p>")
				.arg(esc(tr("Linked inside the module")),
				     links.join(QStringLiteral("<br>")));
	}
	if (!loose.isEmpty()) {
		html += QStringLiteral("<p>%1<br>%2</p>")
				.arg(esc(tr("Other channels")),
				     loose.join(QStringLiteral("<br>")));
	}
	if (!free_pins.isEmpty()) {
		html += QStringLiteral("<p>%1: %2</p>")
				.arg(esc(tr("Free")),
				     esc(free_pins.join(QStringLiteral(", "))));
	}
	return html;
}
