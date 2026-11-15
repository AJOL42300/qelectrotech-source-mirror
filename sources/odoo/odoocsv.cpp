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
#include "odoocsv.h"

#include <QCoreApplication>
#include <QSaveFile>
#include <QSet>

/**
	@param raw the bytes of a file
	@return its text: UTF-8 with or without byte order mark, else the local
	encoding, as the material file does
*/
QString OdooCsv::decode(const QByteArray &raw)
{
	if (raw.startsWith("\xEF\xBB\xBF")) {
		return QString::fromUtf8(raw.constData() + 3, raw.size() - 3);
	}
	if (QString::fromUtf8(raw).toUtf8() == raw) {
		return QString::fromUtf8(raw);
	}
	return QString::fromLocal8Bit(raw);
}

QString OdooCsv::field(const QString &value, QChar separator)
{
	if (value.contains(separator) || value.contains(QLatin1Char('"'))
			|| value.contains(QLatin1Char('\n')) || value.contains(QLatin1Char('\r'))
			|| value != value.trimmed())
	{
		QString escaped = value;
		escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
		return QLatin1Char('"') + escaped + QLatin1Char('"');
	}
	return value;
}

QByteArray OdooCsv::serialize(const QList<QStringList> &rows,
							  const OdooMapping &mapping)
{
	QByteArray csv;
	if (mapping.utf8_bom) {
		csv.append("\xEF\xBB\xBF");
	}
	for (const QStringList &columns : rows)
	{
		QStringList cells;
		for (const QString &cell : columns) {
			cells.append(field(cell, mapping.separator));
		}
		csv += (cells.join(mapping.separator) + QLatin1Char('\n')).toUtf8();
	}
	return csv;
}

/**
	@return the cells of a row: the template of each enabled column
	expanded with fields; a column with only_if is empty when that field is
*/
QStringList OdooCsv::row(const QList<OdooCsvColumn> &columns,
						 const QMap<QString, QString> &fields)
{
	QStringList cells;
	for (const OdooCsvColumn &c : columns)
	{
		if (!c.enabled) {
			continue;
		}
		if (!c.only_if.isEmpty() && fields.value(c.only_if).isEmpty()) {
			cells.append(QString());
		} else {
			cells.append(OdooMapping::expand(c.value, fields));
		}
	}
	return cells;
}

namespace {

QStringList headerOf(const QList<OdooCsvColumn> &columns)
{
	QStringList header;
	for (const OdooCsvColumn &c : columns)
	{
		if (c.enabled) {
			header.append(c.column);
		}
	}
	return header;
}

QMap<QString, QString> bomFields(const OdooBom &bom, const OdooMapping &mapping)
{
	QMap<QString, QString> fields;
	fields.insert(QStringLiteral("bom_product"), bom.product_reference);
	fields.insert(QStringLiteral("bom_code"), bom.code);
	fields.insert(QStringLiteral("bom_name"), bom.product_name);
	fields.insert(QStringLiteral("bom_type"), mapping.bom_type);
	fields.insert(QStringLiteral("cabinet"), bom.cabinet);
	return fields;
}

}

/**
	@brief OdooCsv::productsCsv
	The CSV of the products, to import in Odoo's Inventory (or Sales)
	products list. When boms is not empty, the product of each bill of
	materials is in it too, so that the bills of materials can be imported
	right after.
	@param products one part per reference
	@param boms
	@param mapping
	@return the file
*/
QByteArray OdooCsv::productsCsv(const QList<OdooPart> &products,
								const QList<OdooBom> &boms,
								const OdooMapping &mapping)
{
	QList<QStringList> rows;
	rows.append(headerOf(mapping.product_columns));
	for (const OdooPart &part : products) {
		rows.append(row(mapping.product_columns, part.fields()));
	}
	for (const OdooBom &bom : boms)
	{
		OdooPart parent;
		parent.reference = bom.product_reference;
		parent.name = bom.product_name;
		parent.quantity = 1.0;
		rows.append(row(mapping.product_columns, parent.fields()));
	}
	return serialize(rows, mapping);
}

/**
	@brief OdooCsv::bomCsv
	The CSV of the bills of materials, to import in Manufacturing, Bills of
	Materials. Odoo reads a bill of materials over several rows: the first
	has the columns of the bill and its first line, the next ones only
	their line.
	@param boms
	@param mapping
	@return the file
*/
QByteArray OdooCsv::bomCsv(const QList<OdooBom> &boms, const OdooMapping &mapping)
{
	QList<QStringList> rows;
	rows.append(headerOf(mapping.bom_header_columns) + headerOf(mapping.bom_line_columns));
	const int header_size = headerOf(mapping.bom_header_columns).size();
	for (const OdooBom &bom : boms)
	{
		bool first = true;
		for (const OdooPart &part : bom.lines)
		{
			QMap<QString, QString> fields = part.fields();
			const QMap<QString, QString> bom_map = bomFields(bom, mapping);
			for (auto it = bom_map.constBegin(); it != bom_map.constEnd(); ++it)
			{
				if (it.key() != QLatin1String("cabinet")) {
					fields.insert(it.key(), it.value());
				}
			}
			QStringList header_cells = row(mapping.bom_header_columns, fields);
			if (!first)
			{
				header_cells = QStringList();
				for (int i = 0; i < header_size; ++i) {
					header_cells.append(QString());
				}
			}
			rows.append(header_cells + row(mapping.bom_line_columns, fields));
			first = false;
		}
	}
	return serialize(rows, mapping);
}

bool OdooCsv::writeFile(const QString &path, const QByteArray &csv, QString *error)
{
	if (error) {
		error->clear();
	}
	QSaveFile file(path);
	if (!file.open(QIODevice::WriteOnly) || file.write(csv) != csv.size())
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
	@brief OdooCsv::importProducts
	Read the CSV of Odoo's products (an export of the list, or a file
	written by productsCsv()) as records of the material file. Columns are
	matched on their technical name or their label, case ignored, as listed
	by the mapping. A row without an internal reference is a continuation
	line of the previous product (the vendors of an export) and is ignored.
	@param csv
	@param mapping
	@param result
	@param error
	@return false when the file has no internal reference column
*/
bool OdooCsv::importProducts(const QByteArray &csv, const OdooMapping &mapping,
							 OdooImportResult *result, QString *error)
{
	if (error) {
		error->clear();
	}
	*result = OdooImportResult();

	const QString content = decode(csv);
	if (content.trimmed().isEmpty())
	{
		if (error) {
			*error = QCoreApplication::translate("OdooCsv", "The file is empty.");
		}
		return false;
	}
	const int eol = content.indexOf(QLatin1Char('\n'));
	const QChar separator = MaterialList::detectSeparator(eol < 0 ? content : content.left(eol));
	QList<QStringList> rows = MaterialList::parseCsv(content, separator);
	if (rows.isEmpty()) {
		return true;
	}
	const QStringList header = rows.takeFirst();

	QMap<QString, int> column_index;
	for (auto it = mapping.import_columns.constBegin();
		 it != mapping.import_columns.constEnd(); ++it)
	{
		for (const QString &alias : it.value())
		{
			int found = -1;
			for (int i = 0; i < header.size(); ++i)
			{
				if (header.at(i).trimmed().compare(alias, Qt::CaseInsensitive) == 0)
				{
					found = i;
					break;
				}
			}
			if (found >= 0)
			{
				column_index.insert(it.key(), found);
				break;
			}
		}
	}

	const QString key_column = QStringLiteral("manufacturer_reference");
	if (!column_index.contains(key_column))
	{
		if (error) {
			*error = QCoreApplication::translate("OdooCsv", "No internal reference column was found. Expected one of: %1.")
					.arg(mapping.import_columns.value(key_column).join(QStringLiteral(", ")));
		}
		return false;
	}

	QSet<QString> seen;
	for (const QStringList &cells : std::as_const(rows))
	{
		const QString reference = column_index.value(key_column) < cells.size()
				? cells.at(column_index.value(key_column)).trimmed() : QString();
		if (reference.isEmpty())
		{
			++result->skipped_rows;
			continue;
		}
		if (seen.contains(reference))
		{
			result->warnings.append(QCoreApplication::translate("OdooCsv", "Reference %1 appears more than once: the first line is kept.").arg(reference));
			continue;
		}
		seen.insert(reference);

		MaterialRecord record;
		for (auto it = column_index.constBegin(); it != column_index.constEnd(); ++it)
		{
			if (it.value() < cells.size() && !cells.at(it.value()).trimmed().isEmpty()) {
				record.setValue(it.key(), cells.at(it.value()).trimmed());
			}
		}
		result->records.append(record);
	}
	return true;
}
