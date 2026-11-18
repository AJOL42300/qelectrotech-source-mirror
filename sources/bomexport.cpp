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
#include "bomexport.h"

#include <QCoreApplication>
#include <QSaveFile>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

namespace {
QString csvField(QString value)
{
	value.replace(QLatin1Char('"'), QStringLiteral("\"\""));
	return QLatin1Char('"') + value + QLatin1Char('"');
}

}

QByteArray BomExport::csvRecord(const QStringList &values)
{
	QStringList escaped;
	for (const auto &value : values) {
		escaped.append(csvField(value));
	}
	return (escaped.join(QLatin1Char(';')) + QLatin1Char('\n')).toUtf8();
}

QStringList BomExport::defaultColumns()
{
	return {
		QStringLiteral("label"),
		QStringLiteral("designation"),
		QStringLiteral("manufacturer"),
		QStringLiteral("manufacturer_reference"),
		QStringLiteral("model"),
		QStringLiteral("description"),
		QStringLiteral("category"),
		QStringLiteral("quantity"),
		QStringLiteral("voltage_rating"),
		QStringLiteral("current_rating"),
		QStringLiteral("folio"),
		QStringLiteral("notes")
	};
}

/**
	@brief BomExport::junctionFilter
	@return a condition true for every row that is not a junction: a
	terminal-type element with no label and no part data. Some users draw
	the dots and bends where wires branch as their own terminal-type
	symbols, and each one became an empty row of the parts list (#1178).
	A terminal block with a label or a part number is kept.
*/
QString BomExport::junctionFilter()
{
	return QStringLiteral("NOT (element_type = 'terminal' "
						  "AND IFNULL(label, '') = '' "
						  "AND IFNULL(designation, '') = '' "
						  "AND IFNULL(manufacturer, '') = '' "
						  "AND IFNULL(manufacturer_reference, '') = '')");
}

/**
	@brief BomExport::defaultQuery
	@param include_slaves : false leaves out the contact blocks (slave
	elements), which otherwise appear as rows of their own
	@param include_junctions : false leaves out the junctions, see
	junctionFilter()
	@return the query of the parts list
*/
QString BomExport::defaultQuery(bool include_slaves, bool include_junctions)
{
		//Slaves and terminals are included because both are routinely
		//separately orderable hardware. A circuit breaker can carry ten or
		//twenty auxiliary blocks, each with its own order code, and a
		//terminal block is a purchased part in its own right. Neither shares
		//a line with its master: the query is ungrouped, one row per element,
		//so each appears as the distinct item it is.
		//
		//Anything that should not be ordered is kept out by setting
		//exclude_from_bom on the element, which the view already honours --
		//a relay's own auxiliary contact, say.
		//
		//Thumbnails are deliberately left out for now even though ten of
		//them in the shipped examples carry manufacturer and reference data
		//(the assembly-plan mounting-plate symbols), because that has not
		//been asked for and is a separate question. The folio report arrows
		//and the conductor definition stay out because they are not hardware.
		//
		//See discussion #847.
	const QString types = include_slaves
			? QStringLiteral("'simple', 'master', 'slave', 'terminal'")
			: QStringLiteral("'simple', 'master', 'terminal'");
	const QString junctions = include_junctions
			? QString()
			: QStringLiteral(" AND ") + junctionFilter();
	return QStringLiteral("SELECT %1 FROM element_nomenclature_view "
						  "WHERE element_type IN (%2)%3 "
						  "ORDER BY diagram_position, position, label")
			.arg(defaultColumns().join(QStringLiteral(", ")), types, junctions);
}

/**
	@brief BomExport::groupedQuery
	The parts list the way it is ordered: one line per part, however many
	elements use it. Parts are the same when manufacturer, part number and
	unit are. An element with no part number is never merged with another,
	since nothing says they are the same part.
	Elements with no part number, designation or description are not
	parts and stay out, which also keeps out junctions and folio arrows.
	Columns: number (optional), quantity, name, part_number, references
	(optional), manufacturer, unit.
	Idea from PR #1650 by victorienea.
	@param options
	@return the SQL, to run on element_nomenclature_view
*/
QString BomExport::groupedQuery(const GroupedOptions &options)
{
		//The main part and each auxiliary block, as the same set of columns.
		//The row number tells two elements with no part number apart.
	const auto part = [&options](const QString &suffix) {
		QString zero_filter;
		if (options.skip_zero_quantity) {
			zero_filter = QStringLiteral(
				" AND NOT (TRIM(COALESCE(quantity%1, '')) <> '' "
				"AND TRIM(quantity%1, ' 0.,') = '')").arg(suffix);
		}
		return QStringLiteral(
			"SELECT TRIM(COALESCE(manufacturer%1, '')) AS manufacturer, "
			"TRIM(COALESCE(manufacturer_reference%1, '')) AS article, "
			"COALESCE(NULLIF(TRIM(description%1), ''), designation%1, '') AS name, "
			"label, TRIM(COALESCE(unity%1, '')) AS unit, "
			"CAST(REPLACE(COALESCE(NULLIF(TRIM(quantity%1), ''), '1'), ',', '.') AS REAL) AS qty, "
			"'row:' || CAST(ROW_NUMBER() OVER () AS TEXT) || ':%1' AS item_key "
			"FROM element_nomenclature_view "
			"WHERE (TRIM(COALESCE(manufacturer_reference%1, '')) <> '' "
			"OR TRIM(COALESCE(designation%1, '')) <> '' "
			"OR TRIM(COALESCE(description%1, '')) <> '')%2")
				.arg(suffix, zero_filter);
	};
	QString source = part(QString());
	if (options.auxiliaries)
	{
		for (int i = 1 ; i <= AuxiliaryBlocks ; ++i) {
			source += QStringLiteral(" UNION ALL ")
					+ part(QStringLiteral("_auxiliary%1").arg(i));
		}
	}

	const QString grouped = QStringLiteral(
		"SELECT manufacturer, article, MIN(name) AS name, unit, %1 AS qty, "
		"GROUP_CONCAT(DISTINCT label) AS refs FROM (%2) "
		"GROUP BY manufacturer, article, unit, "
		"CASE WHEN TRIM(article) = '' THEN COALESCE(label, '') || item_key ELSE '' END")
			.arg(options.sum_quantity ? QStringLiteral("SUM(qty)") : QStringLiteral("COUNT(*)"),
				 source);

	QStringList columns;
	if (options.numbering) {
		columns << QStringLiteral("ROW_NUMBER() OVER (ORDER BY manufacturer, article, name, refs) AS number");
	}
	columns << QStringLiteral("qty AS quantity")
			<< QStringLiteral("name")
			<< QStringLiteral("article AS part_number");
	if (options.references) {
		columns << QStringLiteral("refs AS \"references\"");
	}
	columns << QStringLiteral("manufacturer") << QStringLiteral("unit");

	return QStringLiteral("SELECT %1 FROM (%2) ORDER BY manufacturer, article, name, refs")
			.arg(columns.join(QStringLiteral(", ")), grouped);
}

/**
	@brief BomExport::hierarchicalQuery
	The parts list as a tree: each device, then what belongs to it, one
	indentation more for each level.

	What belongs to a device:
	@li its auxiliary blocks, listed under its main article;
	@li the elements whose label is the label of the device followed by
	a separator ('.', '-' or '_') and something more, so -QS1.1 and
	-QS1-MC belong to -QS1. The device with the longest label that fits is
	the parent, so -QS1.1.2 belongs to -QS1.1 when there is one, else to
	-QS1. Labels are compared as typed: -QS10 is no accessory of -QS1, and
	-qs1.1 is none of -QS1.

	Lines are never merged and a quantity of 0 stays: the accessories only
	listed as compatible are on the list, but are not ordered.

	Columns: level, label, quantity, total_quantity (optional), name
	(indented), part_number, manufacturer, unit, orphan. total_quantity
	is the quantity of the line multiplied by those of its parents, so
	nothing is ordered under a parent which is not. orphan is 1 for a line
	whose label looks like that of an accessory (-QS1.1) but which has no
	parent on the list (no -QS1): it stays at the first level, to be checked.

	Everything is done in SQL, to run on element_nomenclature_view, so
	that the export and a folio table can both use it. Lines come in the
	order of the project (folio, then position) at each level.
	@param options
	@return the SQL
*/
QString BomExport::hierarchicalQuery(const HierarchicalOptions &options)
{
	const int last_block = options.auxiliaries ? AuxiliaryBlocks : 0;

	QStringList block_values;
	for (int i = 0 ; i <= last_block ; ++i) {
		block_values << QStringLiteral("(%1)").arg(i);
	}

		//The same field of the main article (block 0) and of each
		//auxiliary block, chosen by the block of the row
	const auto field = [last_block](const QString &name) {
		if (last_block == 0) {
			return QStringLiteral("e.") + name;
		}
		QString expression = QStringLiteral("CASE b.blk");
		for (int i = 0 ; i <= last_block ; ++i)
		{
			expression += QStringLiteral(" WHEN %1 THEN e.").arg(i) + name;
			if (i > 0) {
				expression += QStringLiteral("_auxiliary%1").arg(i);
			}
		}
		return expression + QStringLiteral(" END");
	};
	const QString manufacturer = field(QStringLiteral("manufacturer"));
	const QString reference = field(QStringLiteral("manufacturer_reference"));
	const QString designation = field(QStringLiteral("designation"));
	const QString description = field(QStringLiteral("description"));
	const QString quantity = field(QStringLiteral("quantity"));
	const QString unity = field(QStringLiteral("unity"));

	QString sql = QStringLiteral(
		"WITH RECURSIVE bom_blocks(blk) AS (VALUES @BLOCKS@), "
			//One row per element; its number only orders the elements
		"bom_elem AS (SELECT ROW_NUMBER() OVER "
		"(ORDER BY diagram_position, position, label) AS eid, v.* "
		"FROM element_nomenclature_view v), "
			//One row per article: the main one and each auxiliary block
		"bom_art AS (SELECT e.eid AS eid, b.blk AS blk, "
		"TRIM(COALESCE(e.label, '')) AS label, "
		"TRIM(COALESCE(@MANUFACTURER@, '')) AS manufacturer, "
		"TRIM(COALESCE(@REFERENCE@, '')) AS article, "
		"COALESCE(NULLIF(TRIM(@DESCRIPTION@), ''), @DESIGNATION@, '') AS name, "
		"TRIM(COALESCE(@UNITY@, '')) AS unit, "
		"CAST(REPLACE(COALESCE(NULLIF(TRIM(@QUANTITY@), ''), '1'), ',', '.') AS REAL) AS qty, "
		"CAST(REPLACE(COALESCE(NULLIF(TRIM(e.quantity), ''), '1'), ',', '.') AS REAL) AS main_qty "
		"FROM bom_elem e, bom_blocks b "
		"WHERE (TRIM(COALESCE(@REFERENCE@, '')) <> '' "
		"OR TRIM(COALESCE(@DESIGNATION@, '')) <> '' "
		"OR TRIM(COALESCE(@DESCRIPTION@, '')) <> '')), "
			//The devices, by label, and the one each belongs to: the
			//longest other label followed by a separator
		"bom_nodes AS (SELECT label, MIN(eid) AS first_eid "
		"FROM bom_art GROUP BY label), "
		"bom_link AS (SELECT n.label AS label, n.first_eid AS first_eid, "
		"COALESCE((SELECT a.qty FROM bom_art a "
		"WHERE a.label = n.label AND a.blk = 0 "
		"ORDER BY a.eid LIMIT 1), 1) AS own_qty, "
		"(SELECT p.label FROM bom_nodes p WHERE p.label <> '' "
		"AND length(p.label) + 1 < length(n.label) "
		"AND substr(n.label, 1, length(p.label)) = p.label "
		"AND substr(n.label, length(p.label) + 1, 1) IN ('.', '-', '_') "
		"ORDER BY length(p.label) DESC LIMIT 1) AS parent "
		"FROM bom_nodes n), "
		"bom_ranked AS (SELECT label, parent, own_qty, "
		"ROW_NUMBER() OVER (ORDER BY first_eid) AS rk FROM bom_link), "
			//The tree: depth, the sort key and the quantity of the parents
		"bom_hierarchy_tree(label, depth, path, mult, own_qty) AS ("
		"SELECT label, 0, printf('%06d', rk), 1.0, own_qty "
		"FROM bom_ranked WHERE parent IS NULL "
		"UNION ALL "
		"SELECT r.label, t.depth + 1, t.path || '/' || printf('%06d', r.rk), "
		"t.mult * t.own_qty, r.own_qty "
		"FROM bom_ranked r JOIN bom_hierarchy_tree t ON r.parent = t.label) "
		"SELECT t.depth + (CASE WHEN a.blk > 0 THEN 1 ELSE 0 END) AS level, "
		"a.label AS label, "
		"a.qty AS quantity, "
		"@TOTAL@"
		"printf('%*s%s', 2 * (t.depth + (CASE WHEN a.blk > 0 THEN 1 ELSE 0 END)), '', a.name) AS name, "
		"a.article AS part_number, "
		"a.manufacturer AS manufacturer, "
		"a.unit AS unit, "
		"CASE WHEN t.depth = 0 AND a.blk = 0 "
		"AND LTRIM(a.label, '=+-') GLOB '*[A-Za-z0-9][-._][A-Za-z0-9]*' "
		"THEN 1 ELSE 0 END AS orphan "
		"FROM bom_art a JOIN bom_hierarchy_tree t ON t.label = a.label "
		"ORDER BY t.path, a.eid, a.blk");

	const QString total = options.cumulative_quantity
			? QStringLiteral("t.mult * (CASE WHEN a.blk > 0 THEN a.main_qty ELSE 1 END) "
							 "* a.qty AS total_quantity, ")
			: QString();
	sql.replace(QStringLiteral("@BLOCKS@"), block_values.join(QStringLiteral(", ")));
	sql.replace(QStringLiteral("@MANUFACTURER@"), manufacturer);
	sql.replace(QStringLiteral("@REFERENCE@"), reference);
	sql.replace(QStringLiteral("@DESIGNATION@"), designation);
	sql.replace(QStringLiteral("@DESCRIPTION@"), description);
	sql.replace(QStringLiteral("@QUANTITY@"), quantity);
	sql.replace(QStringLiteral("@UNITY@"), unity);
	sql.replace(QStringLiteral("@TOTAL@"), total);
	return sql;
}

bool BomExport::isHierarchicalQuery(const QString &query)
{
	return query.contains(QLatin1String("bom_hierarchy_tree"));
}

QString BomExport::hierarchicalHeader(const QString &field_name)
{
	const auto context = "BomExport";
	if (field_name == QLatin1String("level"))
		return QCoreApplication::translate(context, "Level");
	if (field_name == QLatin1String("label"))
		return QCoreApplication::translate(context, "Label");
	if (field_name == QLatin1String("quantity"))
		return QCoreApplication::translate(context, "Quantity");
	if (field_name == QLatin1String("total_quantity"))
		return QCoreApplication::translate(context, "Total quantity");
	if (field_name == QLatin1String("name"))
		return QCoreApplication::translate(context, "Name");
	if (field_name == QLatin1String("part_number"))
		return QCoreApplication::translate(context, "Part number");
	if (field_name == QLatin1String("manufacturer"))
		return QCoreApplication::translate(context, "Manufacturer");
	if (field_name == QLatin1String("unit"))
		return QCoreApplication::translate(context, "Unit");
	if (field_name == QLatin1String("orphan"))
		return QCoreApplication::translate(context, "No parent");
	return field_name;
}

QByteArray BomExport::toCsv(QSqlQuery &query, const QStringList &headers,
							bool include_headers, int *row_count)
{
	QByteArray csv("\xEF\xBB\xBF");
	if (include_headers) {
		csv += csvRecord(headers);
	}
	int rows = 0;
	while (query.next())
	{
		QStringList values;
		for (int i = 0; i < query.record().count(); ++i) {
			values.append(query.value(i).toString());
		}
		csv += csvRecord(values);
		++rows;
	}
	if (row_count) {
		*row_count = rows;
	}
	return csv;
}

bool BomExport::writeCsv(const QString &file_path, const QByteArray &csv,
						 QString *error)
{
	if (error) {
		error->clear();
	}
	QSaveFile file(file_path);
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
