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
#ifndef ODOOMAPPING_H
#define ODOOMAPPING_H

#include <QByteArray>
#include <QChar>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

/**
	@brief One column of a CSV written for Odoo.

	The value is a template: {name} is replaced by a field of the part
	(see OdooMapping::fieldNames()). A column marked only_if is written
	for the parts having that field filled, and left empty for the others.
*/
struct OdooCsvColumn
{
	QString column;
	QString value;
	QString only_if;
	bool enabled = true;
};

/**
	@brief One field of an Odoo record written through the API.

	update tells what happens when the product already exists in Odoo:
	"never" leaves the field as it is, "fill_empty" writes it when it is
	empty in Odoo, "overwrite" replaces it. type is "string" or "float".
*/
struct OdooApiField
{
	QString field;
	QString value;
	QString type = QStringLiteral("string");
	QString update = QStringLiteral("fill_empty");
	bool enabled = true;
};

/**
	@brief One value read back from Odoo and the column of the material
	file receiving it.
*/
struct OdooPullField
{
	QString odoo;
	QString column;
	bool enabled = true;
};

/**
	@brief The OdooMapping class says which columns QElectroTech writes
	and reads when it exchanges a bill of materials with Odoo.

	It is plain data, loaded from and saved to a JSON file so that the
	columns can be adapted (a custom field, another language of headers)
	without rebuilding the program. Nothing here knows Odoo's network side,
	nor any credential.
*/
class OdooMapping
{
	public:
		static QStringList fieldNames();
		static OdooMapping defaults();

		static bool fromJson(const QByteArray &json, OdooMapping *mapping,
							 QString *error = nullptr);
		QByteArray toJson() const;
		static bool load(const QString &path, OdooMapping *mapping,
						 QString *error = nullptr);
		bool save(const QString &path, QString *error = nullptr) const;

		static QString expand(const QString &pattern,
							  const QMap<QString, QString> &fields);

			//Odoo's import accepts the comma and the semicolon; the byte
			//order mark would end up in the first header, so it is off.
		QChar separator = QLatin1Char(',');
		bool utf8_bom = false;

		QString bom_type = QStringLiteral("normal");

			//CSV for Odoo's import screen
		QList<OdooCsvColumn> product_columns;
		QList<OdooCsvColumn> bom_header_columns;
		QList<OdooCsvColumn> bom_line_columns;

			//Odoo's own export read back: material column -> headers
		QMap<QString, QStringList> import_columns;

			//API
		QList<OdooApiField> api_product_fields;
		QList<OdooPullField> api_pull_fields;
};

#endif // ODOOMAPPING_H
