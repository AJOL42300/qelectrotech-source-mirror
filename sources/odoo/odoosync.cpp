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
#include "odoosync.h"

#include "odooclient.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QMap>
#include <QSet>

namespace {

const QString product_model = QStringLiteral("product.product");
const QString bom_model = QStringLiteral("mrp.bom");
const QString line_model = QStringLiteral("mrp.bom.line");
const int chunk_size = 150;

QString tr(const char *text)
{
	return QCoreApplication::translate("OdooSync", text);
}

QJsonArray chunk(const QStringList &list, int from)
{
	QJsonArray array;
	for (int i = from; i < list.size() && i < from + chunk_size; ++i) {
		array.append(list.at(i));
	}
	return array;
}

bool isEmptyValue(const QJsonValue &value)
{
	if (value.isNull() || value.isUndefined()) {
		return true;
	}
	if (value.isBool()) {
		return !value.toBool();
	}
	if (value.isString()) {
		return value.toString().trimmed().isEmpty();
	}
	if (value.isDouble()) {
		return value.toDouble() == 0.0;
	}
	return false;
}

bool sameValue(const QJsonValue &odoo, const QJsonValue &wanted)
{
	if (wanted.isDouble()) {
		return qAbs(odoo.toDouble() - wanted.toDouble()) < 0.00001;
	}
	return odoo.toString() == wanted.toString();
}

QString display(const QJsonValue &value)
{
	if (value.isDouble()) {
		return OdooPart::number(value.toDouble());
	}
	return value.toString();
}

	//A many2one is read as [id, name], or false
qint64 many2oneId(const QJsonValue &value)
{
	return value.isArray() ? value.toArray().at(0).toInteger() : 0;
}

QString many2oneName(const QJsonValue &value)
{
	return value.isArray() ? value.toArray().at(1).toString() : QString();
}

/**
	@return the values to write for a product built from part, according to
	the fields of the mapping that exist in this Odoo
*/
bool valuesOf(const OdooPart &part, const OdooMapping &mapping,
			  const QMap<QString, QString> &types,
			  const QJsonObject &odoo, bool exists,
			  QJsonObject *values, QStringList *changes)
{
	const QMap<QString, QString> fields = part.fields();
	for (const OdooApiField &api : mapping.api_product_fields)
	{
		if (!api.enabled || api.field.isEmpty() || !types.contains(api.field)) {
			continue;
		}
		const QString text = OdooMapping::expand(api.value, fields).trimmed();
		if (text.isEmpty()) {
			continue;
		}
		QJsonValue wanted;
		if (api.type == QLatin1String("float"))
		{
			bool ok = false;
			const double number = QString(text).replace(QLatin1Char(','), QLatin1Char('.')).toDouble(&ok);
			if (!ok || number == 0.0) {
				continue;
			}
			wanted = number;
		} else {
			wanted = text;
		}

		if (exists)
		{
			if (api.update == QLatin1String("never") || api.field == QLatin1String("default_code")) {
				continue;
			}
			const QJsonValue current = odoo.value(api.field);
			const bool write = api.update == QLatin1String("overwrite")
					? !sameValue(current, wanted)
					: isEmptyValue(current);
			if (!write) {
				continue;
			}
			changes->append(QStringLiteral("%1: %2 -> %3").arg(api.field, display(current), display(wanted)));
		}
		values->insert(api.field, wanted);
	}
	return !values->isEmpty();
}

}

int OdooPushPlan::count(OdooAction action) const
{
	int total = 0;
	for (const OdooProductPlan &p : products) {
		total += p.action == action;
	}
	for (const OdooBomPlan &bom : boms)
	{
		total += bom.action == action;
			//The lines of a new bill of materials are sent with it
		if (bom.action == OdooAction::Create) {
			continue;
		}
		for (const OdooBomLinePlan &line : bom.lines) {
			total += line.action == action;
		}
	}
	return total;
}

/**
	@return the number of create and write calls applyPush() makes
*/
int OdooPushPlan::writeCalls() const
{
	return count(OdooAction::Create) + count(OdooAction::Update);
}

QStringList OdooPushPlan::describe() const
{
	QStringList lines;
	for (const OdooProductPlan &p : products)
	{
		if (p.action == OdooAction::Unchanged) {
			continue;
		}
		const QString what = p.action == OdooAction::Create
				? tr("Create product") : tr("Update product");
		lines << QStringLiteral("%1 %2 (%3)").arg(what, p.reference, p.name)
				 + (p.changes.isEmpty() ? QString() : QStringLiteral(" [%1]").arg(p.changes.join(QStringLiteral("; "))));
	}
	for (const OdooBomPlan &bom : boms)
	{
		lines << QStringLiteral("%1 %2 -> %3")
				 .arg(bom.action == OdooAction::Create ? tr("Create bill of materials") : tr("Update bill of materials"),
					  bom.code, bom.product_reference);
		for (const OdooBomLinePlan &line : bom.lines)
		{
			if (line.action == OdooAction::Unchanged) {
				continue;
			}
			lines << QStringLiteral("    %1 %2 x %3")
					 .arg(line.action == OdooAction::Create ? QStringLiteral("+") : QStringLiteral("~"),
						  line.reference, OdooPart::number(line.quantity));
		}
		for (const QString &extra : bom.extra_lines) {
			lines << QStringLiteral("    ! %1 %2").arg(extra, tr("is only in Odoo, left as it is"));
		}
	}
	for (const QString &warning : warnings) {
		lines << QStringLiteral("? %1").arg(warning);
	}
	return lines;
}

OdooSync::OdooSync(OdooClient *client, const OdooMapping &mapping) :
	m_client(client),
	m_mapping(mapping)
{}

/**
	@brief OdooSync::planPush
	Compare what QElectroTech has with what Odoo has. Only reads.
	@param products the parts to create or update as Odoo products
	@param boms the bills of materials
	@param options
	@param plan receives the result
	@param error
	@return false when Odoo could not be read
*/
bool OdooSync::planPush(const QList<OdooPart> &products, const QList<OdooBom> &boms,
						const OdooPushOptions &options, OdooPushPlan *plan,
						QString *error)
{
	*plan = OdooPushPlan();
	QString problem;

	const QMap<QString, QString> types = m_client->fieldTypes(product_model, &problem);
	if (types.isEmpty())
	{
		if (error) {
			*error = problem.isEmpty() ? tr("The products of Odoo could not be read.") : problem;
		}
		return false;
	}

		//Every product to know: the parts and the bills' own products
	QList<OdooPart> wanted = products;
	QSet<QString> known;
	for (const OdooPart &part : products) {
		known.insert(part.reference);
	}
	if (options.with_boms)
	{
		for (const OdooBom &bom : boms)
		{
			if (bom.product_reference.isEmpty() || known.contains(bom.product_reference)) {
				continue;
			}
			OdooPart parent;
			parent.reference = bom.product_reference;
			parent.name = bom.product_name;
			wanted.append(parent);
			known.insert(parent.reference);
		}
	}
	QSet<QString> parent_references;
	for (const OdooBom &bom : boms) {
		parent_references.insert(bom.product_reference);
	}

	QStringList references;
	for (const OdooPart &part : wanted) {
		references << part.reference;
	}

	QStringList read_fields{QStringLiteral("default_code"), QStringLiteral("product_tmpl_id")};
	for (const OdooApiField &api : m_mapping.api_product_fields) {
		if (api.enabled && types.contains(api.field) && !read_fields.contains(api.field)) {
			read_fields << api.field;
		}
	}
	QMap<QString, QJsonObject> existing;
	for (int from = 0; from < references.size(); from += chunk_size)
	{
		QJsonArray rows;
		if (!m_client->searchRead(product_model,
								  OdooClient::domain(QStringLiteral("default_code"), QStringLiteral("in"), chunk(references, from)),
								  read_fields, 0, &rows, &problem))
		{
			if (error) {
				*error = problem;
			}
			return false;
		}
		for (const QJsonValue &row : rows)
		{
			const QJsonObject object = row.toObject();
			const QString code = object.value(QStringLiteral("default_code")).toString();
			if (existing.contains(code)) {
				plan->warnings << tr("Several Odoo products have the reference %1: the first one is used.").arg(code);
			} else {
				existing.insert(code, object);
			}
		}
	}

	for (const OdooPart &part : wanted)
	{
		OdooProductPlan item;
		item.reference = part.reference;
		item.name = part.name;
		item.is_bom_product = parent_references.contains(part.reference);
		const auto found = existing.constFind(part.reference);
		const bool exists = found != existing.constEnd();
		if (exists)
		{
			item.id = found->value(QStringLiteral("id")).toInteger();
			item.template_id = many2oneId(found->value(QStringLiteral("product_tmpl_id")));
			if (!options.create_only
					&& valuesOf(part, m_mapping, types, *found, true, &item.values, &item.changes)) {
				item.action = OdooAction::Update;
			}
		}
		else
		{
			item.action = OdooAction::Create;
			QStringList unused;
			valuesOf(part, m_mapping, types, QJsonObject(), false, &item.values, &unused);
			item.values.insert(QStringLiteral("default_code"), part.reference);
			if (!item.values.contains(QStringLiteral("name"))) {
				item.values.insert(QStringLiteral("name"), part.name.isEmpty() ? part.reference : part.name);
			}
		}
		plan->products << item;
	}

	if (!options.with_boms || boms.isEmpty()) {
		return true;
	}

	bool has_bom = false;
	if (!m_client->modelExists(bom_model, &has_bom, &problem))
	{
		if (error) {
			*error = problem;
		}
		return false;
	}
	if (!has_bom)
	{
		plan->warnings << tr("The Manufacturing application is not installed in this Odoo: only the products are exchanged.");
		return true;
	}
	plan->boms_supported = true;

	QMap<QString, qint64> product_ids;
	for (auto it = existing.constBegin(); it != existing.constEnd(); ++it) {
		product_ids.insert(it.key(), it->value(QStringLiteral("id")).toInteger());
	}

	for (const OdooBom &bom : boms)
	{
		OdooBomPlan item;
		item.code = bom.code;
		item.product_reference = bom.product_reference;

		QJsonArray rows;
		if (!m_client->searchRead(bom_model,
								  OdooClient::domain(QStringLiteral("code"), QStringLiteral("="), bom.code),
								  {QStringLiteral("code"), QStringLiteral("product_tmpl_id")}, 1, &rows, &problem))
		{
			if (error) {
				*error = problem;
			}
			return false;
		}
		QMap<QString, QPair<qint64, double>> odoo_lines;
		QMap<qint64, QString> reference_of;
		for (auto it = product_ids.constBegin(); it != product_ids.constEnd(); ++it) {
			reference_of.insert(it.value(), it.key());
		}
		if (!rows.isEmpty())
		{
			item.action = OdooAction::Update;
			item.bom_id = rows.at(0).toObject().value(QStringLiteral("id")).toInteger();
			QJsonArray lines;
			if (!m_client->searchRead(line_model,
									  OdooClient::domain(QStringLiteral("bom_id"), QStringLiteral("="), item.bom_id),
									  {QStringLiteral("product_id"), QStringLiteral("product_qty")}, 0, &lines, &problem))
			{
				if (error) {
					*error = problem;
				}
				return false;
			}
			for (const QJsonValue &line : lines)
			{
				const QJsonObject object = line.toObject();
				const qint64 product = many2oneId(object.value(QStringLiteral("product_id")));
				QString reference = reference_of.value(product);
				if (reference.isEmpty())
				{
						//A product of Odoo that QElectroTech does not know
						//by its reference: read it.
					QJsonArray found;
					if (m_client->searchRead(product_model,
											 OdooClient::domain(QStringLiteral("id"), QStringLiteral("="), product),
											 {QStringLiteral("default_code")}, 1, &found, &problem) && !found.isEmpty()) {
						reference = found.at(0).toObject().value(QStringLiteral("default_code")).toString();
					}
					if (reference.isEmpty()) {
						reference = many2oneName(object.value(QStringLiteral("product_id")));
					}
				}
				odoo_lines.insert(reference, {object.value(QStringLiteral("id")).toInteger(),
											  object.value(QStringLiteral("product_qty")).toDouble()});
			}
		}

		QSet<QString> in_qet;
		for (const OdooPart &part : bom.lines)
		{
			OdooBomLinePlan line;
			line.reference = part.reference;
			line.quantity = part.quantity;
			in_qet.insert(part.reference);
			const auto current = odoo_lines.constFind(part.reference);
			if (current == odoo_lines.constEnd()) {
				line.action = OdooAction::Create;
			}
			else
			{
				line.line_id = current->first;
				line.old_quantity = current->second;
				line.action = qAbs(current->second - part.quantity) < 0.00001
						? OdooAction::Unchanged : OdooAction::Update;
			}
			item.lines << line;
		}
		for (auto it = odoo_lines.constBegin(); it != odoo_lines.constEnd(); ++it) {
			if (!in_qet.contains(it.key())) {
				item.extra_lines << it.key();
			}
		}
		if (item.action == OdooAction::Update)
		{
			bool changed = false;
			for (const OdooBomLinePlan &line : item.lines) {
				changed |= line.action != OdooAction::Unchanged;
			}
			if (!changed) {
				item.action = OdooAction::Unchanged;
			}
		}
		plan->boms << item;
	}
	return true;
}

/**
	@brief OdooSync::applyPush
	Do what the plan says. In a simulation nothing is sent: the client is
	not even called.
	@param plan
	@param simulate
	@return what was done, or what would have been
*/
OdooPushResult OdooSync::applyPush(const OdooPushPlan &plan, bool simulate)
{
	OdooPushResult result;
	result.simulated = simulate;
	if (simulate)
	{
		result.created = plan.count(OdooAction::Create);
		result.updated = plan.count(OdooAction::Update);
		result.messages = plan.describe();
		return result;
	}

	QString error;
	QMap<QString, qint64> product_ids;
	QMap<QString, qint64> template_ids;
	auto fail = [&result](const QString &message) {
		result.ok = false;
		result.messages << message;
		return result;
	};

	for (const OdooProductPlan &product : plan.products)
	{
		qint64 id = product.id;
		qint64 template_id = product.template_id;
		if (product.action == OdooAction::Create)
		{
			if (!m_client->create(product_model, product.values, &id, &error)) {
				return fail(tr("Product %1: %2").arg(product.reference, error));
			}
			++result.created;
			result.messages << tr("Created product %1").arg(product.reference);
			if (product.is_bom_product || plan.boms_supported)
			{
				QJsonArray rows;
				if (m_client->searchRead(product_model,
										 OdooClient::domain(QStringLiteral("id"), QStringLiteral("="), id),
										 {QStringLiteral("product_tmpl_id")}, 1, &rows, &error) && !rows.isEmpty()) {
					template_id = many2oneId(rows.at(0).toObject().value(QStringLiteral("product_tmpl_id")));
				}
			}
		}
		else if (product.action == OdooAction::Update)
		{
			if (!m_client->write(product_model, {id}, product.values, &error)) {
				return fail(tr("Product %1: %2").arg(product.reference, error));
			}
			++result.updated;
			result.messages << tr("Updated product %1").arg(product.reference);
		}
		product_ids.insert(product.reference, id);
		template_ids.insert(product.reference, template_id);
	}

	QMap<QString, QString> bom_types;
	if (!plan.boms.isEmpty()) {
		bom_types = m_client->fieldTypes(bom_model, &error);
	}
	for (const OdooBomPlan &bom : plan.boms)
	{
		if (bom.action == OdooAction::Unchanged) {
			continue;
		}
		qint64 bom_id = bom.bom_id;
		if (bom.action == OdooAction::Create)
		{
			const qint64 template_id = template_ids.value(bom.product_reference);
			if (template_id <= 0) {
				return fail(tr("Bill of materials %1: the product %2 is unknown.").arg(bom.code, bom.product_reference));
			}
			QJsonObject values;
			values.insert(QStringLiteral("product_tmpl_id"), template_id);
			values.insert(QStringLiteral("code"), bom.code);
			values.insert(QStringLiteral("product_qty"), 1);
			if (bom_types.contains(QStringLiteral("type")) && !m_mapping.bom_type.isEmpty()) {
				values.insert(QStringLiteral("type"), m_mapping.bom_type);
			}
			QJsonArray lines;
			for (const OdooBomLinePlan &line : bom.lines)
			{
				const qint64 product = product_ids.value(line.reference);
				if (product <= 0) {
					return fail(tr("Bill of materials %1: the product %2 is unknown.").arg(bom.code, line.reference));
				}
				lines.append(QJsonArray{0, 0, QJsonObject{
					{QStringLiteral("product_id"), product},
					{QStringLiteral("product_qty"), line.quantity}}});
			}
			values.insert(QStringLiteral("bom_line_ids"), lines);
			if (!m_client->create(bom_model, values, &bom_id, &error)) {
				return fail(tr("Bill of materials %1: %2").arg(bom.code, error));
			}
			++result.created;
			result.messages << tr("Created bill of materials %1").arg(bom.code);
			continue;
		}

		for (const OdooBomLinePlan &line : bom.lines)
		{
			if (line.action == OdooAction::Update)
			{
				if (!m_client->write(line_model, {line.line_id},
									 QJsonObject{{QStringLiteral("product_qty"), line.quantity}}, &error)) {
					return fail(tr("Bill of materials %1, %2: %3").arg(bom.code, line.reference, error));
				}
				++result.updated;
			}
			else if (line.action == OdooAction::Create)
			{
				const qint64 product = product_ids.value(line.reference);
				if (product <= 0) {
					return fail(tr("Bill of materials %1: the product %2 is unknown.").arg(bom.code, line.reference));
				}
				if (!m_client->create(line_model,
									  QJsonObject{{QStringLiteral("bom_id"), bom_id},
												  {QStringLiteral("product_id"), product},
												  {QStringLiteral("product_qty"), line.quantity}}, nullptr, &error)) {
					return fail(tr("Bill of materials %1, %2: %3").arg(bom.code, line.reference, error));
				}
				++result.created;
			}
		}
		result.messages << tr("Updated bill of materials %1").arg(bom.code);
	}
	return result;
}

/**
	@brief OdooSync::fetchProducts
	Read from Odoo what the mapping asks for, as material records ready to
	be merged into the user catalogue.
	@param references the references to read; all the products having one if empty
	@param limit maximum of products when references is empty, 0 for no limit
	@param result
	@param error
	@return false when Odoo could not be read
*/
bool OdooSync::fetchProducts(const QStringList &references, int limit,
							 OdooPullResult *result, QString *error)
{
	*result = OdooPullResult();
	QString problem;
	const QMap<QString, QString> types = m_client->fieldTypes(product_model, &problem);
	if (types.isEmpty())
	{
		if (error) {
			*error = problem.isEmpty() ? tr("The products of Odoo could not be read.") : problem;
		}
		return false;
	}

	QStringList fields{QStringLiteral("default_code")};
	bool with_supplier = false;
	for (const OdooPullField &pull : m_mapping.api_pull_fields)
	{
		if (!pull.enabled) {
			continue;
		}
		if (pull.odoo == QLatin1String("supplier")) {
			with_supplier = types.contains(QStringLiteral("seller_ids"));
		} else if (types.contains(pull.odoo)) {
			fields << pull.odoo;
		} else {
			result->warnings << tr("This Odoo has no field %1.").arg(pull.odoo);
		}
	}
	if (with_supplier) {
		fields << QStringLiteral("seller_ids");
	}

	QList<QJsonObject> products;
	if (references.isEmpty())
	{
		QJsonArray rows;
		if (!m_client->searchRead(product_model,
								  OdooClient::domain(QStringLiteral("default_code"), QStringLiteral("!="), false),
								  fields, limit, &rows, error)) {
			return false;
		}
		for (const QJsonValue &row : rows) {
			products << row.toObject();
		}
	}
	else
	{
		for (int from = 0; from < references.size(); from += chunk_size)
		{
			QJsonArray rows;
			if (!m_client->searchRead(product_model,
									  OdooClient::domain(QStringLiteral("default_code"), QStringLiteral("in"), chunk(references, from)),
									  fields, 0, &rows, error)) {
				return false;
			}
			for (const QJsonValue &row : rows) {
				products << row.toObject();
			}
		}
	}

		//First supplier of each product
	QMap<qint64, QString> supplier_of;
	if (with_supplier)
	{
		QJsonArray seller_ids;
		for (const QJsonObject &product : products) {
			const QJsonArray ids = product.value(QStringLiteral("seller_ids")).toArray();
			if (!ids.isEmpty()) {
				seller_ids.append(ids.at(0));
			}
		}
		if (!seller_ids.isEmpty())
		{
			QJsonArray sellers;
			if (m_client->searchRead(QStringLiteral("product.supplierinfo"),
									 OdooClient::domain(QStringLiteral("id"), QStringLiteral("in"), seller_ids),
									 {QStringLiteral("partner_id")}, 0, &sellers, &problem))
			{
				for (const QJsonValue &seller : sellers) {
					supplier_of.insert(seller.toObject().value(QStringLiteral("id")).toInteger(),
									   many2oneName(seller.toObject().value(QStringLiteral("partner_id"))));
				}
			} else {
				result->warnings << tr("The suppliers could not be read: %1").arg(problem);
			}
		}
	}

	QSet<QString> found;
	for (const QJsonObject &product : products)
	{
		const QString reference = product.value(QStringLiteral("default_code")).toString();
		if (reference.isEmpty()) {
			continue;
		}
		found.insert(reference.toLower());
		MaterialRecord record;
		record.setValue(QStringLiteral("manufacturer_reference"), reference);
		for (const OdooPullField &pull : m_mapping.api_pull_fields)
		{
			if (!pull.enabled || pull.column.isEmpty()) {
				continue;
			}
			QString text;
			if (pull.odoo == QLatin1String("supplier"))
			{
				const QJsonArray ids = product.value(QStringLiteral("seller_ids")).toArray();
				text = ids.isEmpty() ? QString() : supplier_of.value(ids.at(0).toInteger());
			}
			else
			{
				const QJsonValue value = product.value(pull.odoo);
				text = value.isBool() ? QString() : display(value);
			}
			if (!text.isEmpty()) {
				record.setValue(pull.column, text);
			}
		}
		result->records << record;
	}
	for (const QString &reference : references) {
		if (!found.contains(reference.toLower())) {
			result->not_found << reference;
		}
	}
	return true;
}
