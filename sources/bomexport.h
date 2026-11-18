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
#ifndef BOMEXPORT_H
#define BOMEXPORT_H

#include <QByteArray>
#include <QString>
#include <QStringList>

class QSqlQuery;

namespace BomExport
{
	QStringList defaultColumns();
	QString defaultQuery(bool include_slaves = true,
						 bool include_junctions = true);
	QString junctionFilter();

		/// What the grouped parts list shows, see groupedQuery()
	struct GroupedOptions
	{
			/// Add up each part's quantity field; false counts the rows
		bool sum_quantity = true;
			/// A column with the labels of the elements using the part
		bool references = true;
			/// A first column numbering the lines
		bool numbering = true;
			/// The auxiliary blocks of an element are parts of their own
		bool auxiliaries = true;
			/// A part whose quantity is 0 is not ordered (an accessory only
			/// listed as compatible): leave it out of the sums and of the
			/// count of rows
		bool skip_zero_quantity = true;
	};
	QString groupedQuery(const GroupedOptions &options = GroupedOptions());

		/// Auxiliary blocks stored by an element (auxiliary1 to
		/// auxiliary4 in the element informations and the database).
		/// More articles on one device are written as accessory elements
		/// whose label starts with the label of the device: the
		/// hierarchical list has no limit for those.
	constexpr int AuxiliaryBlocks = 4;

		/// What the hierarchical parts list shows, see hierarchicalQuery()
	struct HierarchicalOptions
	{
			/// The auxiliary blocks of an element are listed under its
			/// main article
		bool auxiliaries = true;
			/// A column with the quantity to order: the quantity of the
			/// line multiplied by those of its parent devices
		bool cumulative_quantity = true;
	};
	QString hierarchicalQuery(
			const HierarchicalOptions &options = HierarchicalOptions());
		/// @return true when \a query is a hierarchicalQuery()
	bool isHierarchicalQuery(const QString &query);
		/// @return the heading of a column of the hierarchical list,
		/// \a field_name itself for any other column
	QString hierarchicalHeader(const QString &field_name);
		/// One CSV record: every field quoted, ';' between them, '\n'
		/// after -- the format every CSV export of this program writes.
		/// Public so a list which is not read from the project database
		/// (the cable list) writes the same bytes.
	QByteArray csvRecord(const QStringList &values);
	QByteArray toCsv(QSqlQuery &query, const QStringList &headers,
					 bool include_headers = true, int *row_count = nullptr);
	bool writeCsv(const QString &file_path, const QByteArray &csv,
				  QString *error = nullptr);
}

#endif // BOMEXPORT_H
