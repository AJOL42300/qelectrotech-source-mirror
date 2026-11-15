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
#ifndef ODOOSYNC_H
#define ODOOSYNC_H

#include "odoobom.h"
#include "odoomapping.h"
#include "materiallist/materiallist.h"

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

class OdooClient;

enum class OdooAction {
	Create,
	Update,
	Unchanged
};

/**
	@brief What is to be done to one product of Odoo.
*/
struct OdooProductPlan
{
	QString reference;
	QString name;
	OdooAction action = OdooAction::Unchanged;
	qint64 id = 0;
	qint64 template_id = 0;
	QJsonObject values;
	QStringList changes;
	bool is_bom_product = false;
};

/**
	@brief What is to be done to one line of a bill of materials.
*/
struct OdooBomLinePlan
{
	QString reference;
	double quantity = 0.0;
	double old_quantity = 0.0;
	OdooAction action = OdooAction::Create;
	qint64 line_id = 0;
};

/**
	@brief What is to be done to one bill of materials. Lines that Odoo
	has and QElectroTech does not are only reported in extra_lines:
	nothing is ever removed.
*/
struct OdooBomPlan
{
	QString code;
	QString product_reference;
	OdooAction action = OdooAction::Create;
	qint64 bom_id = 0;
	QList<OdooBomLinePlan> lines;
	QStringList extra_lines;
};

struct OdooPushOptions
{
		//Create the missing products but leave the existing ones alone
	bool create_only = false;
	bool with_boms = true;
};

/**
	@brief The result of planPush(): everything that applyPush() would do,
	computed from reads only.
*/
struct OdooPushPlan
{
	QList<OdooProductPlan> products;
	QList<OdooBomPlan> boms;
	QStringList warnings;
	bool boms_supported = false;

	int count(OdooAction action) const;
	int writeCalls() const;
	QStringList describe() const;
};

struct OdooPushResult
{
	bool ok = true;
	bool simulated = true;
	int created = 0;
	int updated = 0;
	QStringList messages;
};

struct OdooPullResult
{
	QList<MaterialRecord> records;
	QStringList not_found;
	QStringList warnings;
};

/**
	@brief The OdooSync class plans and applies the exchange of products
	and bills of materials with Odoo.

	Planning only reads. Applying writes, unless it is a simulation, and
	only creates records or updates fields: there is no code anywhere that
	deletes a record of Odoo.
*/
class OdooSync
{
	public:
		OdooSync(OdooClient *client, const OdooMapping &mapping);

		bool planPush(const QList<OdooPart> &products, const QList<OdooBom> &boms,
					  const OdooPushOptions &options, OdooPushPlan *plan,
					  QString *error);
		OdooPushResult applyPush(const OdooPushPlan &plan, bool simulate);

		bool fetchProducts(const QStringList &references, int limit,
						   OdooPullResult *result, QString *error);

	private:
		OdooClient *m_client;
		OdooMapping m_mapping;
};

#endif // ODOOSYNC_H
