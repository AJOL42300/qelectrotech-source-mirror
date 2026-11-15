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
#ifndef ODOOBOM_H
#define ODOOBOM_H

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

class QDomDocument;
class QSqlQuery;

/**
	@brief One purchasable part of a project, as Odoo knows a product.

	The reference is the manufacturer reference of QElectroTech, used as
	the internal reference (default_code) of the Odoo product: it is the
	key of every exchange.
*/
struct OdooPart
{
	QString reference;
	QString name;
	QString manufacturer;
	QString supplier;
	QString unit;
	QString cabinet;
	QStringList labels;
	double quantity = 0.0;
	double price = 0.0;
	bool is_cable = false;

	static QString number(double value);
	QMap<QString, QString> fields() const;
};

/**
	@brief A bill of materials: the parts of one cabinet, or of the whole
	project, and the Odoo product it is the bill of materials of.
*/
struct OdooBom
{
	QString cabinet;
	QString code;
	QString product_reference;
	QString product_name;
	QList<OdooPart> lines;
};

/**
	@brief What goes into the bills of materials.
*/
struct OdooBomOptions
{
	bool per_cabinet = false;
	bool auxiliaries = true;
	QString project_name;
		//Reference of the Odoo product of a bill of materials: this prefix,
		//a dash, the project, and for a cabinet a dash and its name.
	QString product_prefix = QStringLiteral("QET");
};

/**
	@brief The OdooBomBuilder class collects the parts of a project and
	groups them by reference into bills of materials.

	It works on the nomenclature view of the project database and on the
	project XML, and knows nothing of Odoo's network side.
*/
class OdooBomBuilder
{
	public:
		static QString partsQuery(bool auxiliaries = true);
		static QString unreferencedQuery(bool auxiliaries = true);

		static QList<OdooPart> readParts(QSqlQuery &query);
		static int readCount(QSqlQuery &query);

		static QList<OdooPart> cableParts(const QDomDocument &project);

		static QList<OdooBom> buildBoms(const QList<OdooPart> &parts,
										const OdooBomOptions &options);
		static QList<OdooPart> products(const QList<OdooBom> &boms);
		static QList<OdooPart> productsOfParts(const QList<OdooPart> &parts);

		static double parseLength(const QString &text);
		static QString slug(const QString &text);
};

#endif // ODOOBOM_H
