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
#include "odoomapping.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>

namespace {

OdooCsvColumn csvColumn(const QString &column, const QString &value,
						const QString &only_if = QString())
{
	OdooCsvColumn c;
	c.column = column;
	c.value = value;
	c.only_if = only_if;
	return c;
}

OdooApiField apiField(const QString &field, const QString &value,
					  const QString &type, const QString &update)
{
	OdooApiField f;
	f.field = field;
	f.value = value;
	f.type = type;
	f.update = update;
	return f;
}

OdooPullField pullField(const QString &odoo, const QString &column)
{
	OdooPullField f;
	f.odoo = odoo;
	f.column = column;
	return f;
}

QJsonArray csvColumnsToJson(const QList<OdooCsvColumn> &columns)
{
	QJsonArray array;
	for (const OdooCsvColumn &c : columns)
	{
		QJsonObject o;
		o.insert(QStringLiteral("column"), c.column);
		o.insert(QStringLiteral("value"), c.value);
		if (!c.only_if.isEmpty()) {
			o.insert(QStringLiteral("only_if"), c.only_if);
		}
		o.insert(QStringLiteral("enabled"), c.enabled);
		array.append(o);
	}
	return array;
}

bool csvColumnsFromJson(const QJsonValue &value, QList<OdooCsvColumn> *columns,
						const QString &what, QString *error)
{
	if (value.isUndefined()) {
		return true;
	}
	if (!value.isArray())
	{
		if (error) {
			*error = QCoreApplication::translate("OdooMapping", "\"%1\" must be a list.").arg(what);
		}
		return false;
	}
	QList<OdooCsvColumn> result;
	for (const QJsonValue &item : value.toArray())
	{
		const QJsonObject o = item.toObject();
		OdooCsvColumn c;
		c.column = o.value(QStringLiteral("column")).toString();
		c.value = o.value(QStringLiteral("value")).toString();
		c.only_if = o.value(QStringLiteral("only_if")).toString();
		c.enabled = o.value(QStringLiteral("enabled")).toBool(true);
		if (c.column.isEmpty())
		{
			if (error) {
				*error = QCoreApplication::translate("OdooMapping", "An entry of \"%1\" has no \"column\".").arg(what);
			}
			return false;
		}
		result.append(c);
	}
	*columns = result;
	return true;
}

QString separatorToString(QChar separator)
{
	return separator == QLatin1Char('\t') ? QStringLiteral("tab") : QString(separator);
}

QChar separatorFromString(const QString &text, QChar fallback)
{
	if (text == QLatin1String("tab") || text == QLatin1String("\\t")) {
		return QLatin1Char('\t');
	}
	if (text == QLatin1String(",") || text == QLatin1String(";")) {
		return text.at(0);
	}
	return fallback;
}

}

/**
	@return the names usable between braces in a value template
*/
QStringList OdooMapping::fieldNames()
{
	return {
		QStringLiteral("reference"),
		QStringLiteral("name"),
		QStringLiteral("manufacturer"),
		QStringLiteral("supplier"),
		QStringLiteral("unit"),
		QStringLiteral("quantity"),
		QStringLiteral("price"),
		QStringLiteral("cabinet"),
		QStringLiteral("labels"),
		QStringLiteral("bom_product"),
		QStringLiteral("bom_code"),
		QStringLiteral("bom_name"),
		QStringLiteral("bom_type"),
		QStringLiteral("project")
	};
}

/**
	@return the mapping used when no file is given. The headers are the
	technical names of Odoo's fields, which its import screen always
	recognises, whatever the language of the user interface.
*/
OdooMapping OdooMapping::defaults()
{
	OdooMapping m;
	m.product_columns = {
		csvColumn(QStringLiteral("default_code"), QStringLiteral("{reference}")),
		csvColumn(QStringLiteral("name"), QStringLiteral("{name}")),
		csvColumn(QStringLiteral("standard_price"), QStringLiteral("{price}")),
		csvColumn(QStringLiteral("description_purchase"), QStringLiteral("{manufacturer}")),
		csvColumn(QStringLiteral("seller_ids/partner_id"), QStringLiteral("{supplier}"), QStringLiteral("supplier")),
		csvColumn(QStringLiteral("seller_ids/product_code"), QStringLiteral("{reference}"), QStringLiteral("supplier"))
	};
	m.bom_header_columns = {
		csvColumn(QStringLiteral("product_tmpl_id"), QStringLiteral("{bom_product}")),
		csvColumn(QStringLiteral("code"), QStringLiteral("{bom_code}")),
		csvColumn(QStringLiteral("product_qty"), QStringLiteral("1")),
		csvColumn(QStringLiteral("type"), QStringLiteral("{bom_type}"))
	};
	m.bom_line_columns = {
		csvColumn(QStringLiteral("bom_line_ids/product_id"), QStringLiteral("{reference}")),
		csvColumn(QStringLiteral("bom_line_ids/product_qty"), QStringLiteral("{quantity}"))
	};

	m.import_columns.insert(QStringLiteral("manufacturer_reference"),
		{QStringLiteral("default_code"), QStringLiteral("Internal Reference"),
		 QStringLiteral("R\u00E9f\u00E9rence interne")});
	m.import_columns.insert(QStringLiteral("designation"),
		{QStringLiteral("name"), QStringLiteral("Name"), QStringLiteral("Nom"),
		 QStringLiteral("Product Name"), QStringLiteral("Nom du produit")});
	m.import_columns.insert(QStringLiteral("unit_price"),
		{QStringLiteral("standard_price"), QStringLiteral("Cost"),
		 QStringLiteral("Co\u00FBt")});
	m.import_columns.insert(QStringLiteral("supplier"),
		{QStringLiteral("seller_ids/partner_id"),
		 QStringLiteral("seller_ids/partner_id/name"),
		 QStringLiteral("Vendors/Vendor"), QStringLiteral("Vendor"),
		 QStringLiteral("Fournisseurs/Fournisseur"), QStringLiteral("Fournisseur")});
	m.import_columns.insert(QStringLiteral("manufacturer"),
		{QStringLiteral("manufacturer"), QStringLiteral("Manufacturer"),
		 QStringLiteral("Fabricant"),
		 QStringLiteral("description_purchase"),
		 QStringLiteral("Purchase Description"),
		 QStringLiteral("Description d'achat")});
	m.import_columns.insert(QStringLiteral("stock"),
		{QStringLiteral("qty_available"), QStringLiteral("Quantity On Hand"),
		 QStringLiteral("Quantit\u00E9 en stock")});

	m.api_product_fields = {
		apiField(QStringLiteral("default_code"), QStringLiteral("{reference}"), QStringLiteral("string"), QStringLiteral("never")),
		apiField(QStringLiteral("name"), QStringLiteral("{name}"), QStringLiteral("string"), QStringLiteral("fill_empty")),
		apiField(QStringLiteral("standard_price"), QStringLiteral("{price}"), QStringLiteral("float"), QStringLiteral("fill_empty")),
		apiField(QStringLiteral("description_purchase"), QStringLiteral("{manufacturer}"), QStringLiteral("string"), QStringLiteral("fill_empty"))
	};
	m.api_pull_fields = {
		pullField(QStringLiteral("name"), QStringLiteral("designation")),
		pullField(QStringLiteral("standard_price"), QStringLiteral("unit_price")),
		pullField(QStringLiteral("qty_available"), QStringLiteral("stock")),
		pullField(QStringLiteral("supplier"), QStringLiteral("supplier"))
	};
	return m;
}

/**
	@brief OdooMapping::fromJson
	Missing sections keep their default, so a file may only list what it
	changes.
	@param json
	@param mapping receives the result, left untouched on error
	@param error
	@return true on success
*/
bool OdooMapping::fromJson(const QByteArray &json, OdooMapping *mapping,
						   QString *error)
{
	QJsonParseError parse_error;
	const QJsonDocument doc = QJsonDocument::fromJson(json, &parse_error);
	if (doc.isNull() || !doc.isObject())
	{
		if (error) {
			*error = parse_error.error == QJsonParseError::NoError
					? QCoreApplication::translate("OdooMapping", "The mapping must be a JSON object.")
					: parse_error.errorString();
		}
		return false;
	}

	OdooMapping m = defaults();
	const QJsonObject root = doc.object();

	const QJsonObject csv = root.value(QStringLiteral("csv")).toObject();
	m.separator = separatorFromString(
				csv.value(QStringLiteral("separator")).toString(), m.separator);
	m.utf8_bom = csv.value(QStringLiteral("utf8_bom")).toBool(m.utf8_bom);
	m.bom_type = csv.value(QStringLiteral("bom_type")).toString(m.bom_type);
	if (!csvColumnsFromJson(csv.value(QStringLiteral("products")), &m.product_columns, QStringLiteral("products"), error)
			|| !csvColumnsFromJson(csv.value(QStringLiteral("bom_header")), &m.bom_header_columns, QStringLiteral("bom_header"), error)
			|| !csvColumnsFromJson(csv.value(QStringLiteral("bom_line")), &m.bom_line_columns, QStringLiteral("bom_line"), error)) {
		return false;
	}

	const QJsonObject imports = root.value(QStringLiteral("import")).toObject();
	for (auto it = imports.constBegin(); it != imports.constEnd(); ++it)
	{
		QStringList headers;
		if (it.value().isArray())
		{
			for (const QJsonValue &h : it.value().toArray()) {
				headers << h.toString();
			}
		}
		else if (it.value().isString()) {
			headers << it.value().toString();
		}
		m.import_columns.insert(it.key(), headers);
	}

	const QJsonObject api = root.value(QStringLiteral("api")).toObject();
	if (api.value(QStringLiteral("product_fields")).isArray())
	{
		m.api_product_fields.clear();
		for (const QJsonValue &item : api.value(QStringLiteral("product_fields")).toArray())
		{
			const QJsonObject o = item.toObject();
			OdooApiField f;
			f.field = o.value(QStringLiteral("field")).toString();
			f.value = o.value(QStringLiteral("value")).toString();
			f.type = o.value(QStringLiteral("type")).toString(f.type);
			f.update = o.value(QStringLiteral("update")).toString(f.update);
			f.enabled = o.value(QStringLiteral("enabled")).toBool(true);
			if (f.field.isEmpty()) {
				continue;
			}
			if (f.type != QLatin1String("string") && f.type != QLatin1String("float"))
			{
				if (error) {
					*error = QCoreApplication::translate("OdooMapping", "Unknown type \"%1\" for the field \"%2\".").arg(f.type, f.field);
				}
				return false;
			}
			if (f.update != QLatin1String("never") && f.update != QLatin1String("fill_empty")
					&& f.update != QLatin1String("overwrite"))
			{
				if (error) {
					*error = QCoreApplication::translate("OdooMapping", "Unknown update mode \"%1\" for the field \"%2\".").arg(f.update, f.field);
				}
				return false;
			}
			m.api_product_fields.append(f);
		}
	}
	if (api.value(QStringLiteral("pull_fields")).isArray())
	{
		m.api_pull_fields.clear();
		for (const QJsonValue &item : api.value(QStringLiteral("pull_fields")).toArray())
		{
			const QJsonObject o = item.toObject();
			OdooPullField f;
			f.odoo = o.value(QStringLiteral("odoo")).toString();
			f.column = o.value(QStringLiteral("column")).toString();
			f.enabled = o.value(QStringLiteral("enabled")).toBool(true);
			if (!f.odoo.isEmpty() && !f.column.isEmpty()) {
				m.api_pull_fields.append(f);
			}
		}
	}

	*mapping = m;
	return true;
}

/**
	@return the mapping as the JSON text fromJson() reads
*/
QByteArray OdooMapping::toJson() const
{
	QJsonObject csv;
	csv.insert(QStringLiteral("separator"), separatorToString(separator));
	csv.insert(QStringLiteral("utf8_bom"), utf8_bom);
	csv.insert(QStringLiteral("bom_type"), bom_type);
	csv.insert(QStringLiteral("products"), csvColumnsToJson(product_columns));
	csv.insert(QStringLiteral("bom_header"), csvColumnsToJson(bom_header_columns));
	csv.insert(QStringLiteral("bom_line"), csvColumnsToJson(bom_line_columns));

	QJsonObject imports;
	for (auto it = import_columns.constBegin(); it != import_columns.constEnd(); ++it) {
		imports.insert(it.key(), QJsonArray::fromStringList(it.value()));
	}

	QJsonArray fields;
	for (const OdooApiField &f : api_product_fields)
	{
		QJsonObject o;
		o.insert(QStringLiteral("field"), f.field);
		o.insert(QStringLiteral("value"), f.value);
		o.insert(QStringLiteral("type"), f.type);
		o.insert(QStringLiteral("update"), f.update);
		o.insert(QStringLiteral("enabled"), f.enabled);
		fields.append(o);
	}
	QJsonArray pulls;
	for (const OdooPullField &f : api_pull_fields)
	{
		QJsonObject o;
		o.insert(QStringLiteral("odoo"), f.odoo);
		o.insert(QStringLiteral("column"), f.column);
		o.insert(QStringLiteral("enabled"), f.enabled);
		pulls.append(o);
	}
	QJsonObject api;
	api.insert(QStringLiteral("product_fields"), fields);
	api.insert(QStringLiteral("pull_fields"), pulls);

	QJsonObject root;
	root.insert(QStringLiteral("version"), 1);
	root.insert(QStringLiteral("csv"), csv);
	root.insert(QStringLiteral("import"), imports);
	root.insert(QStringLiteral("api"), api);
	return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

bool OdooMapping::load(const QString &path, OdooMapping *mapping, QString *error)
{
	if (error) {
		error->clear();
	}
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
	{
		if (error) {
			*error = file.errorString();
		}
		return false;
	}
	return fromJson(file.readAll(), mapping, error);
}

bool OdooMapping::save(const QString &path, QString *error) const
{
	if (error) {
		error->clear();
	}
	QSaveFile file(path);
	const QByteArray json = toJson();
	if (!file.open(QIODevice::WriteOnly) || file.write(json) != json.size())
	{
		if (error) {
			*error = file.errorString();
		}
		file.cancelWriting();
		return false;
	}
	if (!file.commit())
	{
		if (error) {
			*error = file.errorString();
		}
		return false;
	}
	return true;
}

/**
	@brief OdooMapping::expand
	Replace each {name} of pattern by its value. An unknown name is
	replaced by nothing.
	@param pattern
	@param fields
	@return the text
*/
QString OdooMapping::expand(const QString &pattern,
							const QMap<QString, QString> &fields)
{
	static const QRegularExpression placeholder(QStringLiteral("\\{([a-z_]+)\\}"));
	QString result;
	int last = 0;
	QRegularExpressionMatchIterator it = placeholder.globalMatch(pattern);
	while (it.hasNext())
	{
		const QRegularExpressionMatch match = it.next();
		result += pattern.mid(last, match.capturedStart() - last);
		result += fields.value(match.captured(1));
		last = match.capturedEnd();
	}
	result += pattern.mid(last);
	return result;
}
