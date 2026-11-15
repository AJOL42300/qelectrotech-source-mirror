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
#ifndef ODOOCSV_H
#define ODOOCSV_H

#include "odoobom.h"
#include "odoomapping.h"
#include "materiallist/materiallist.h"

#include <QByteArray>
#include <QList>
#include <QStringList>

/**
	@brief What was read from a CSV file written by Odoo.
*/
struct OdooImportResult
{
	QList<MaterialRecord> records;
	QStringList warnings;
	int skipped_rows = 0;
};

/**
	@brief The OdooCsv class writes the CSV files Odoo's import screen
	reads, and reads the CSV files Odoo exports.

	The columns come from an OdooMapping. No network is involved.
*/
class OdooCsv
{
	public:
		static QByteArray productsCsv(const QList<OdooPart> &products,
									  const QList<OdooBom> &boms,
									  const OdooMapping &mapping);
		static QByteArray bomCsv(const QList<OdooBom> &boms,
								 const OdooMapping &mapping);
		static bool writeFile(const QString &path, const QByteArray &csv,
							  QString *error = nullptr);

		static bool importProducts(const QByteArray &csv,
								   const OdooMapping &mapping,
								   OdooImportResult *result,
								   QString *error = nullptr);

		static QString decode(const QByteArray &raw);

	private:
		static QString field(const QString &value, QChar separator);
		static QByteArray serialize(const QList<QStringList> &rows,
									const OdooMapping &mapping);
		static QStringList row(const QList<OdooCsvColumn> &columns,
							   const QMap<QString, QString> &fields);
};

#endif // ODOOCSV_H
