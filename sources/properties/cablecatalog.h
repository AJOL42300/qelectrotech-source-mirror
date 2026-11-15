// SPDX-License-Identifier: GPL-2.0-or-later
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
#ifndef CABLECATALOG_H
#define CABLECATALOG_H

#include <QList>
#include <QString>

class ConductorProperties;

/**
	@brief Catalogue of manufacturer cables.

	Plain facts about cables (reference, designation, cores, section,
	diameter, shield, sheath, usage), kept in a JSON file of the user
	configuration folder, outside the project files. A small set of
	examples is built in. A catalogue exported by the user from the
	manufacturer site can be imported from a CSV file.
*/
namespace CableCatalog
{
	struct Entry
	{
		QString manufacturer;
			/// Manufacturer article number, the key of the catalogue
		QString reference;
		QString designation;
		QString family;
			/// Number of cores, 0 when unknown
		int conductors = 0;
		int pairs = 0;
			/// Cross-section in mm², free text such as "1,5"
		QString section;
			/// Outer diameter in mm
		QString diameter;
		QString shield;
		QString sheath;
			/// Core identification, for example "numbers"
		QString color_code;
			/// motor, servo, ethernet, ethercat, ethercat_p, m12,
			/// y_m12 or control; empty when unknown
		QString usage;
			/// Length in metres of a made-up cable, empty for a
			/// cable sold by the metre
		QString length;
	};

	QList<Entry> builtin();
	QString userFile();
	QList<Entry> readJson(const QString &path, QString *error = nullptr);
	bool writeJson(const QString &path, const QList<Entry> &entries);
	QList<Entry> readCsv(const QString &path, QString *error = nullptr);
	QList<Entry> merged(const QList<Entry> &base, const QList<Entry> &extra);
	QList<Entry> entries();
	int importCsv(const QString &path, QString *error = nullptr);

	QString networkTypeId(const Entry &entry);
	QString title(const Entry &entry);
	void apply(ConductorProperties &properties, const Entry &entry);
}

#endif // CABLECATALOG_H
