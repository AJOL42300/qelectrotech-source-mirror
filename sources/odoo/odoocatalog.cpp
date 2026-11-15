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
#include "odoocatalog.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>

namespace {
const QString key_column = QStringLiteral("manufacturer_reference");
}

/**
	@brief OdooCatalog::merge
	@param data the catalogue, loaded by the caller
	@param incoming the articles to merge, each with a manufacturer
	reference
	@param mode FillEmpty only writes cells that are empty in the
	catalogue, Overwrite also replaces the ones that differ
	@param apply false leaves data untouched and only lists the changes
	@return the changes, one per cell, or one per new article
*/
QList<OdooCatalogChange> OdooCatalog::merge(MaterialListData *data,
											const QList<MaterialRecord> &incoming,
											Mode mode, bool apply)
{
	QList<OdooCatalogChange> changes;
	MaterialListData work = *data;

	QMap<QString, int> index;
	for (int i = 0; i < work.records.size(); ++i)
	{
		const QString reference = work.records.at(i).value(key_column).trimmed().toLower();
		if (!reference.isEmpty() && !index.contains(reference)) {
			index.insert(reference, i);
		}
	}

	const auto ensure_column = [&work](const QString &column) {
		if (!work.columns.contains(column)) {
			work.columns.append(column);
		}
	};
	if (work.columns.isEmpty()) {
		work.columns = MaterialList::defaultColumns();
	}

	for (const MaterialRecord &record : incoming)
	{
		const QString reference = record.value(key_column).trimmed();
		if (reference.isEmpty()) {
			continue;
		}

		const auto found = index.constFind(reference.toLower());
		if (found == index.constEnd())
		{
			OdooCatalogChange change;
			change.reference = reference;
			change.is_new = true;
			changes.append(change);
			for (auto it = record.values.constBegin(); it != record.values.constEnd(); ++it)
			{
				if (!it.value().isEmpty()) {
					ensure_column(it.key());
				}
			}
			index.insert(reference.toLower(), work.records.size());
			work.records.append(record);
			continue;
		}

		MaterialRecord &existing = work.records[found.value()];
		for (auto it = record.values.constBegin(); it != record.values.constEnd(); ++it)
		{
			if (it.key() == key_column || it.value().isEmpty()) {
				continue;
			}
			const QString old_value = existing.value(it.key());
			if (old_value == it.value()) {
				continue;
			}
			if (mode == Mode::FillEmpty && !old_value.trimmed().isEmpty()) {
				continue;
			}
			OdooCatalogChange change;
			change.reference = reference;
			change.column = it.key();
			change.old_value = old_value;
			change.new_value = it.value();
			changes.append(change);
			ensure_column(it.key());
			existing.setValue(it.key(), it.value());
		}
	}

	if (apply) {
		*data = work;
	}
	return changes;
}

/**
	@brief OdooCatalog::saveWithBackup
	Write the catalogue, after copying the file as it is to path.bak.
	@param path
	@param data
	@param error
	@return true on success
*/
bool OdooCatalog::saveWithBackup(const QString &path,
								 const MaterialListData &data, QString *error)
{
	if (error) {
		error->clear();
	}
	const QFileInfo info(path);
	if (info.exists() && info.size() > 0)
	{
		const QString backup = path + QStringLiteral(".bak");
		QFile::remove(backup);
		if (!QFile::copy(path, backup))
		{
			if (error) {
				*error = QCoreApplication::translate("OdooCatalog", "Cannot write %1").arg(backup);
			}
			return false;
		}
	}
	return MaterialList::writeFile(path, data, error);
}
