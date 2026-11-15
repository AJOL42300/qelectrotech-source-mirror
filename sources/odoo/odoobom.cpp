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
#include "odoobom.h"

#include <QDomDocument>
#include <QDomElement>
#include <QDomNodeList>
#include <QRegularExpression>
#include <QSqlQuery>
#include <algorithm>

/**
	@param value
	@return value as Odoo reads a number: a dot, no useless zeros
*/
QString OdooPart::number(double value)
{
	QString text = QString::number(value, 'f', 4);
	if (text.contains(QLatin1Char('.')))
	{
		while (text.endsWith(QLatin1Char('0'))) {
			text.chop(1);
		}
		if (text.endsWith(QLatin1Char('.'))) {
			text.chop(1);
		}
	}
	return text;
}

/**
	@return the values a value template of the mapping can name. The
	price is empty when it is unknown, so that a column is not filled
	with a misleading zero.
*/
QMap<QString, QString> OdooPart::fields() const
{
	QMap<QString, QString> map;
	map.insert(QStringLiteral("reference"), reference);
	map.insert(QStringLiteral("name"), name);
	map.insert(QStringLiteral("manufacturer"), manufacturer);
	map.insert(QStringLiteral("supplier"), supplier);
	map.insert(QStringLiteral("unit"), unit);
	map.insert(QStringLiteral("quantity"), number(quantity));
	map.insert(QStringLiteral("price"), price > 0.0 ? number(price) : QString());
	map.insert(QStringLiteral("cabinet"), cabinet);
	map.insert(QStringLiteral("labels"), labels.join(QStringLiteral(", ")));
	return map;
}

namespace {

QString partSelect(const QString &suffix, const QString &price)
{
	return QStringLiteral(
		"SELECT COALESCE(TRIM(location), '') AS cabinet, "
		"TRIM(manufacturer_reference%1) AS reference, "
		"COALESCE(NULLIF(TRIM(description%1), ''), NULLIF(TRIM(designation%1), ''), '') AS name, "
		"TRIM(COALESCE(manufacturer%1, '')) AS manufacturer, "
		"TRIM(COALESCE(supplier%1, '')) AS supplier, "
		"TRIM(COALESCE(unity%1, '')) AS unit, "
		"CAST(REPLACE(COALESCE(NULLIF(TRIM(quantity%1), ''), '1'), ',', '.') AS REAL) AS qty, "
		"%2 AS price, COALESCE(label, '') AS label "
		"FROM element_nomenclature_view "
		"WHERE TRIM(COALESCE(manufacturer_reference%1, '')) <> ''")
			.arg(suffix, price);
}

QString unionOfParts(bool auxiliaries)
{
		//The price is the one of the main part only: the view has no
		//price for the auxiliary blocks
	QString source = partSelect(QString(), QStringLiteral(
		"CAST(REPLACE(COALESCE(NULLIF(TRIM(unit_price), ''), '0'), ',', '.') AS REAL)"));
	if (auxiliaries)
	{
		for (int i = 1; i <= 4; ++i)
		{
			source += QStringLiteral(" UNION ALL ")
					+ partSelect(QStringLiteral("_auxiliary%1").arg(i),
								 QStringLiteral("0"));
		}
	}
	return source;
}

void mergePart(OdooPart *into, const OdooPart &from)
{
	into->quantity += from.quantity;
	if (from.price > into->price) {
		into->price = from.price;
	}
	if (into->name.isEmpty()) {
		into->name = from.name;
	}
	if (into->manufacturer.isEmpty()) {
		into->manufacturer = from.manufacturer;
	}
	if (into->supplier.isEmpty()) {
		into->supplier = from.supplier;
	}
	if (into->unit.isEmpty()) {
		into->unit = from.unit;
	}
	for (const QString &label : from.labels)
	{
		if (!label.isEmpty() && !into->labels.contains(label)) {
			into->labels.append(label);
		}
	}
	into->is_cable = into->is_cable && from.is_cable;
}

QList<OdooPart> groupByReference(const QList<OdooPart> &parts)
{
	QList<OdooPart> result;
	QMap<QString, int> index;
	for (const OdooPart &part : parts)
	{
		if (part.reference.isEmpty()) {
			continue;
		}
		const auto found = index.constFind(part.reference);
		if (found == index.constEnd())
		{
			OdooPart copy = part;
			copy.cabinet.clear();
			index.insert(part.reference, result.size());
			result.append(copy);
		}
		else {
			mergePart(&result[found.value()], part);
		}
	}
	std::sort(result.begin(), result.end(),
			  [](const OdooPart &a, const OdooPart &b) {
		return a.reference.compare(b.reference, Qt::CaseInsensitive) < 0;
	});
	return result;
}

}

/**
	@brief OdooBomBuilder::partsQuery
	@param auxiliaries true to count the auxiliary parts of the elements
	@return the SQL, to run on the project database: one row per cabinet and
	reference, with the columns cabinet, reference, name, manufacturer,
	supplier, unit, quantity, price and labels. A part without a
	reference is not in it, see unreferencedQuery().
*/
QString OdooBomBuilder::partsQuery(bool auxiliaries)
{
	return QStringLiteral(
		"SELECT cabinet, reference, MAX(name) AS name, "
		"MAX(manufacturer) AS manufacturer, MAX(supplier) AS supplier, "
		"MAX(unit) AS unit, SUM(qty) AS quantity, MAX(price) AS price, "
		"GROUP_CONCAT(DISTINCT label) AS labels FROM (%1) "
		"GROUP BY cabinet, reference ORDER BY cabinet, reference")
			.arg(unionOfParts(auxiliaries));
}

/**
	@return the SQL counting the parts that have a name but no reference:
	Odoo is keyed on the reference, so they can't be exchanged and the
	user is told how many are left out
*/
QString OdooBomBuilder::unreferencedQuery(bool auxiliaries)
{
	const auto part = [](const QString &suffix) {
		return QStringLiteral(
			"SELECT 1 FROM element_nomenclature_view "
			"WHERE TRIM(COALESCE(manufacturer_reference%1, '')) = '' "
			"AND (TRIM(COALESCE(designation%1, '')) <> '' "
			"OR TRIM(COALESCE(description%1, '')) <> '')").arg(suffix);
	};
	QString source = part(QString());
	if (auxiliaries)
	{
		for (int i = 1; i <= 4; ++i) {
			source += QStringLiteral(" UNION ALL ")
					+ part(QStringLiteral("_auxiliary%1").arg(i));
		}
	}
	return QStringLiteral("SELECT COUNT(*) FROM (%1)").arg(source);
}

/**
	@param query a query run from partsQuery()
	@return its rows
*/
QList<OdooPart> OdooBomBuilder::readParts(QSqlQuery &query)
{
	QList<OdooPart> parts;
	while (query.next())
	{
		OdooPart part;
		part.cabinet = query.value(0).toString().trimmed();
		part.reference = query.value(1).toString().trimmed();
		part.name = query.value(2).toString().trimmed();
		part.manufacturer = query.value(3).toString().trimmed();
		part.supplier = query.value(4).toString().trimmed();
		part.unit = query.value(5).toString().trimmed();
		part.quantity = query.value(6).toDouble();
		part.price = query.value(7).toDouble();
		const QStringList labels = query.value(8).toString().split(QLatin1Char(','));
		for (const QString &label : labels)
		{
			if (!label.trimmed().isEmpty()) {
				part.labels.append(label.trimmed());
			}
		}
		if (!part.reference.isEmpty()) {
			parts.append(part);
		}
	}
	return parts;
}

/**
	@param query a query run from unreferencedQuery()
	@return the count it gives
*/
int OdooBomBuilder::readCount(QSqlQuery &query)
{
	return query.next() ? query.value(0).toInt() : 0;
}

/**
	@brief OdooBomBuilder::parseLength
	@param text a length as the user typed it: "12", "12,5 m", "30 cm"
	@return the length in metres, 0 when it can't be read
*/
double OdooBomBuilder::parseLength(const QString &text)
{
	static const QRegularExpression length(
		QStringLiteral("^\\s*([0-9]+(?:[.,][0-9]+)?)\\s*(mm|cm|m)?\\b"),
		QRegularExpression::CaseInsensitiveOption);
	const QRegularExpressionMatch match = length.match(text);
	if (!match.hasMatch()) {
		return 0.0;
	}
	QString number = match.captured(1);
	number.replace(QLatin1Char(','), QLatin1Char('.'));
	double value = number.toDouble();
	const QString unit = match.captured(2).toLower();
	if (unit == QLatin1String("mm")) {
		value /= 1000.0;
	} else if (unit == QLatin1String("cm")) {
		value /= 100.0;
	}
	return value;
}

/**
	@brief OdooBomBuilder::cableParts
	The network cables of a project: the conductors carrying a
	network_reference, summed in metres by reference. The attributes are
	read as they are in the XML so that this works with the project files
	of the network cable feature without depending on its code.
	@param project the XML of the project
	@return one part per reference, in metres
*/
QList<OdooPart> OdooBomBuilder::cableParts(const QDomDocument &project)
{
	QList<OdooPart> parts;
	QMap<QString, int> index;
	const QDomNodeList conductors = project.elementsByTagName(QStringLiteral("conductor"));
	for (int i = 0; i < conductors.size(); ++i)
	{
		const QDomElement conductor = conductors.at(i).toElement();
		const QString reference = conductor.attribute(QStringLiteral("network_reference")).trimmed();
		const double length = parseLength(conductor.attribute(QStringLiteral("network_length")));
		if (reference.isEmpty() || length <= 0.0) {
			continue;
		}

		const QString label = conductor.attribute(QStringLiteral("network_cable")).trimmed();
		const auto found = index.constFind(reference);
		if (found == index.constEnd())
		{
			OdooPart part;
			part.reference = reference;
			part.unit = QStringLiteral("m");
			part.quantity = length;
			part.is_cable = true;
			if (!label.isEmpty()) {
				part.labels.append(label);
			}
			index.insert(reference, parts.size());
			parts.append(part);
		}
		else
		{
			OdooPart &part = parts[found.value()];
			part.quantity += length;
			if (!label.isEmpty() && !part.labels.contains(label)) {
				part.labels.append(label);
			}
		}
	}
	return parts;
}

/**
	@param text
	@return text reduced to what is safe in a reference: letters, digits,
	dots, dashes and underscores
*/
QString OdooBomBuilder::slug(const QString &text)
{
	QString result;
	for (const QChar c : text.trimmed())
	{
		if (c.isLetterOrNumber() || c == QLatin1Char('.')
				|| c == QLatin1Char('_')) {
			result.append(c);
		}
		else if (!result.endsWith(QLatin1Char('-'))) {
			result.append(QLatin1Char('-'));
		}
	}
	while (result.endsWith(QLatin1Char('-'))) {
		result.chop(1);
	}
	return result;
}

/**
	@brief OdooBomBuilder::buildBoms
	@param parts the rows of partsQuery() and the cables
	@param options
	@return one bill of materials for the project, or one per cabinet. The
	parts with no cabinet go into the project's one, or into a bill of
	their own named "unassigned" when the cabinets are split.
*/
QList<OdooBom> OdooBomBuilder::buildBoms(const QList<OdooPart> &parts,
										 const OdooBomOptions &options)
{
	const QString project = options.project_name.trimmed().isEmpty()
			? QStringLiteral("PROJECT") : slug(options.project_name);
	const QString project_reference = options.product_prefix + QLatin1Char('-') + project;
	const QString project_title = options.project_name.trimmed().isEmpty()
			? QStringLiteral("Project") : options.project_name.trimmed();

	QList<OdooBom> boms;
	if (!options.per_cabinet)
	{
		OdooBom bom;
		bom.code = project_reference;
		bom.product_reference = project_reference;
		bom.product_name = project_title;
		bom.lines = groupByReference(parts);
		if (!bom.lines.isEmpty()) {
			boms.append(bom);
		}
		return boms;
	}

	QMap<QString, QList<OdooPart>> by_cabinet;
	for (const OdooPart &part : parts) {
		by_cabinet[part.cabinet.trimmed()].append(part);
	}
	for (auto it = by_cabinet.constBegin(); it != by_cabinet.constEnd(); ++it)
	{
		OdooBom bom;
		bom.cabinet = it.key();
		const QString suffix = it.key().isEmpty()
				? QStringLiteral("unassigned") : slug(it.key());
		bom.code = project_reference + QLatin1Char('-') + suffix;
		bom.product_reference = bom.code;
		bom.product_name = project_title + QStringLiteral(" - ")
				+ (it.key().isEmpty() ? QStringLiteral("unassigned") : it.key());
		bom.lines = groupByReference(it.value());
		if (!bom.lines.isEmpty()) {
			boms.append(bom);
		}
	}
	return boms;
}

/**
	@param boms
	@return the distinct parts of the bills of materials, one per
	reference: what has to exist as a product in Odoo
*/
QList<OdooPart> OdooBomBuilder::products(const QList<OdooBom> &boms)
{
	QList<OdooPart> all;
	for (const OdooBom &bom : boms) {
		all.append(bom.lines);
	}
	return groupByReference(all);
}

/**
	@param parts
	@return the distinct parts, one per reference, whatever their cabinet
*/
QList<OdooPart> OdooBomBuilder::productsOfParts(const QList<OdooPart> &parts)
{
	return groupByReference(parts);
}
