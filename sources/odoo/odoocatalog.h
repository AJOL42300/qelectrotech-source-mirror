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
#ifndef ODOOCATALOG_H
#define ODOOCATALOG_H

#include "materiallist/materiallist.h"

#include <QList>
#include <QString>

/**
	@brief One difference between the user catalogue and what Odoo gives.
*/
struct OdooCatalogChange
{
	QString reference;
		//Empty for a new article, the whole line being the change
	QString column;
	QString old_value;
	QString new_value;
	bool is_new = false;
};

/**
	@brief The OdooCatalog class merges articles coming from Odoo into the
	user catalogue (the material file), keyed on the manufacturer
	reference.

	Nothing is written here: merge() works on a MaterialListData the
	caller loaded, and saveWithBackup() is the only function touching a
	file, keeping the previous content next to it.
*/
class OdooCatalog
{
	public:
		enum class Mode {
			FillEmpty,
			Overwrite
		};

		static QList<OdooCatalogChange> merge(MaterialListData *data,
											  const QList<MaterialRecord> &incoming,
											  Mode mode, bool apply);
		static bool saveWithBackup(const QString &path,
								   const MaterialListData &data,
								   QString *error = nullptr);
};

#endif // ODOOCATALOG_H
