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
#include "cablecatalog.h"

#include "../conductorproperties.h"
#include "../qetapp.h"
#include "networkcable.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QSaveFile>
#include <QStringList>

namespace CableCatalog
{
/**
	@return the examples shipped with the program. Only facts read from
	the manufacturers' sites (article number, designation, cores,
	section, sheath); check them against the data sheet before use.
*/
QList<Entry> builtin()
{
	auto make = [](const QString &manufacturer, const QString &reference,
		       const QString &designation, const QString &family,
		       int conductors, int pairs, const QString &section,
		       const QString &diameter, const QString &shield,
		       const QString &sheath, const QString &color_code,
		       const QString &usage, const QString &length) {
		Entry e;
		e.manufacturer = manufacturer;
		e.reference = reference;
		e.designation = designation;
		e.family = family;
		e.conductors = conductors;
		e.pairs = pairs;
		e.section = section;
		e.diameter = diameter;
		e.shield = shield;
		e.sheath = sheath;
		e.color_code = color_code;
		e.usage = usage;
		e.length = length;
		return e;
	};
	const QString lapp = QStringLiteral("LAPP");
	const QString murr = QStringLiteral("Murrelektronik");
	QList<Entry> l;
	l << make(lapp, "1119304", "4G1,5", "OELFLEX CLASSIC 110",
		  4, 0, "1,5", "7,2", "", "PVC", "numbers", "control", "");
	l << make(lapp, "0027969", "4G1,5+2x(2x0,75)",
		  "OELFLEX SERVO FD 796 CP", 0, 0, "1,5", "", "braid",
		  "PUR", "", "servo", "");
	l << make(murr, "7000-40041-2150500",
		  "M12 male 0 deg / M12 female 0 deg, A-coded, 5 m",
		  "", 5, 0, "0,34", "", "", "PVC grey", "", "m12", "5");
	l << make(murr, "7000-40021-6340500",
		  "M12 male 0 deg / M12 female 0 deg, A-coded, 5 m",
		  "", 4, 0, "0,34", "", "", "PUR", "", "m12", "5");
	l << make(murr, "7000-40701-0130200",
		  "Y-distributor M12 male / 2x female 90 deg, A-coded, 2 m",
		  "", 0, 0, "", "", "", "PVC yellow", "", "y_m12", "2");
	l << make(murr, "7000-42401-6360060",
		  "Y-distributor M12 male / valve plug MSUD A-18 mm, 0,6 m",
		  "", 0, 0, "0,75", "", "", "", "", "y_m12", "0,6");

	const QString beckhoff = QStringLiteral("Beckhoff");
	const struct {
		const char *suffix;
		const char *metres;
	} lengths[] = {{"0005", "0,5"}, {"0010", "1"}, {"0020", "2"},
		       {"0050", "5"}, {"0100", "10"}};
	for (const auto &length : lengths) {
		const QString metres = QString::fromLatin1(length.metres);
		const QString suffix = QString::fromLatin1(length.suffix);
		l << make(beckhoff, "ZK1090-9191-" + suffix,
			  "EtherCAT/Ethernet patch cable RJ45, Cat.5, "
			  "4x2xAWG26, " + metres + " m",
			  "", 0, 4, "0,14", "5,9", "yes", "PUR green", "",
			  "ethercat", metres);
		l << make(beckhoff, "ZK7000-0102-" + suffix,
			  "EtherCAT P cable M8, AWG22, " + metres + " m",
			  "", 4, 0, "0,34", "6,5", "yes",
			  "PUR black, red stripe", "", "ethercat_p", metres);
	}
	return l;
}

/**
	@return the file holding the catalogue of the user
*/
QString userFile()
{
	return QETApp::configDir() + QStringLiteral("/cable_catalog.json");
}

static QJsonObject toJson(const Entry &e)
{
	QJsonObject o;
	auto put = [&o](const char *key, const QString &value) {
		if (!value.isEmpty()) {
			o.insert(QLatin1String(key), value);
		}
	};
	put("manufacturer", e.manufacturer);
	put("reference", e.reference);
	put("designation", e.designation);
	put("family", e.family);
	if (e.conductors) {
		o.insert(QStringLiteral("conductors"), e.conductors);
	}
	if (e.pairs) {
		o.insert(QStringLiteral("pairs"), e.pairs);
	}
	put("section", e.section);
	put("diameter", e.diameter);
	put("shield", e.shield);
	put("sheath", e.sheath);
	put("color_code", e.color_code);
	put("usage", e.usage);
	put("length", e.length);
	return o;
}

static Entry fromJson(const QJsonObject &o)
{
	Entry e;
	e.manufacturer = o.value("manufacturer").toString();
	e.reference = o.value("reference").toString().trimmed();
	e.designation = o.value("designation").toString();
	e.family = o.value("family").toString();
	e.conductors = o.value("conductors").toInt();
	e.pairs = o.value("pairs").toInt();
	e.section = o.value("section").toString();
	e.diameter = o.value("diameter").toString();
	e.shield = o.value("shield").toString();
	e.sheath = o.value("sheath").toString();
	e.color_code = o.value("color_code").toString();
	e.usage = o.value("usage").toString();
	e.length = o.value("length").toString();
	return e;
}

/**
	@return the cables of a catalogue file. A missing file gives an
	empty list without error.
*/
QList<Entry> readJson(const QString &path, QString *error)
{
	QList<Entry> l;
	QFile file(path);
	if (!file.exists()) {
		return l;
	}
	if (!file.open(QIODevice::ReadOnly)) {
		if (error) {
			*error = file.errorString();
		}
		return l;
	}
	QJsonParseError parse_error;
	const QJsonDocument doc =
		QJsonDocument::fromJson(file.readAll(), &parse_error);
	if (!doc.isObject()) {
		if (error) {
			*error = parse_error.errorString();
		}
		return l;
	}
	const QJsonArray cables = doc.object().value("cables").toArray();
	for (const QJsonValue &v : cables) {
		const Entry e = fromJson(v.toObject());
		if (!e.reference.isEmpty()) {
			l << e;
		}
	}
	return l;
}

bool writeJson(const QString &path, const QList<Entry> &entries)
{
	QJsonArray cables;
	for (const Entry &e : entries) {
		cables.append(toJson(e));
	}
	QJsonObject root;
	root.insert(QStringLiteral("version"), 1);
	root.insert(QStringLiteral("cables"), cables);
	QDir().mkpath(QFileInfo(path).absolutePath());
	QSaveFile file(path);
	if (!file.open(QIODevice::WriteOnly)) {
		return false;
	}
	file.write(QJsonDocument(root).toJson());
	return file.commit();
}

/**
	Split CSV text into rows of fields. Fields may be quoted, with ""
	for a quote and line breaks inside quotes.
*/
static QList<QStringList> parseCsv(const QString &text, QChar delimiter)
{
	QList<QStringList> rows;
	QStringList row;
	QString field;
	bool quoted = false;
	bool pending = false;
	for (int i = 0; i < text.size(); ++i) {
		const QChar c = text.at(i);
		if (quoted) {
			if (c == '"') {
				if (i + 1 < text.size()
				    && text.at(i + 1) == '"') {
					field += '"';
					++i;
				} else {
					quoted = false;
				}
			} else {
				field += c;
			}
		} else if (c == '"') {
			quoted = true;
			pending = true;
		} else if (c == delimiter) {
			row << field;
			field.clear();
			pending = true;
		} else if (c == '\n' || c == '\r') {
			if (c == '\r' && i + 1 < text.size()
			    && text.at(i + 1) == '\n') {
				++i;
			}
			if (pending || !field.isEmpty()) {
				row << field;
				rows << row;
			}
			row.clear();
			field.clear();
			pending = false;
		} else {
			field += c;
			pending = true;
		}
	}
	if (pending || !field.isEmpty()) {
		row << field;
		rows << row;
	}
	return rows;
}

static QString normalisedHeader(const QString &header)
{
	QString s;
	for (const QChar &c : header.toLower()) {
		if (c.isLetterOrNumber()) {
			s += c;
		}
	}
	return s;
}

/**
	@return the catalogue field a CSV header stands for, or an empty
	string. English, German and French names of the usual exports are
	known.
*/
static QString fieldOfHeader(const QString &header)
{
	static const QMap<QString, QString> names = []() {
		QMap<QString, QString> m;
		auto add = [&m](const QString &field,
				const QStringList &list) {
			for (const QString &name : list) {
				m.insert(name, field);
			}
		};
		add("manufacturer", {"manufacturer", "brand", "hersteller",
				     "marke", "fabricant", "marque"});
		add("reference", {"reference", "ref", "article",
				  "articlenumber", "artikelnummer",
				  "partnumber", "mfrpartnumber",
				  "referencefabricant", "artnr"});
		add("designation", {"designation", "description",
				    "bezeichnung", "productname", "name",
				    "type", "designationfr"});
		add("family", {"family", "series", "produktfamilie",
			       "gamme", "serie"});
		add("conductors", {"conductors", "cores", "numberofcores",
				   "noofcores", "adernzahl", "anzahladern",
				   "nombredeconducteurs", "poles",
				   "noofpoles", "polzahl", "pole"});
		add("pairs", {"pairs", "numberofpairs", "noofpairs",
			      "paarzahl", "paires"});
		add("section", {"section", "crosssection",
				"conductorcrosssection",
				"nominalconductorcrosssectionmetric",
				"querschnitt", "leiterquerschnitt"});
		add("diameter", {"diameter", "outerdiameter",
				 "nominalouterdiameter", "aussendurchmesser",
				 "au\u00dfendurchmesser", "diametreexterieur"});
		add("shield", {"shield", "shielding", "screen", "schirm",
			       "schirmung", "blindage"});
		add("sheath", {"sheath", "jacket", "outersheath",
			       "jacketcolor", "mantel", "mantelfarbe",
			       "gaine"});
		add("color_code", {"colorcode", "coreidentification",
				   "coloridentification",
				   "adernkennzeichnung", "codecouleur"});
		add("usage", {"usage", "application", "use", "verwendung"});
		add("length", {"length", "cablelength", "laenge",
			       "l\u00e4nge", "longueur"});
		return m;
	}();
	return names.value(normalisedHeader(header));
}

static QString inferredUsage(const Entry &e)
{
	const QString text =
		(e.family + QLatin1Char(' ') + e.designation).toLower();
	if (text.contains("servo")) {
		return QStringLiteral("servo");
	}
	if (text.contains("ethercat p")) {
		return QStringLiteral("ethercat_p");
	}
	if (text.contains("ethercat")) {
		return QStringLiteral("ethercat");
	}
	if (text.contains("etherline") || text.contains("unitronic lan")
	    || text.contains("ethernet") || text.contains("cat.5")
	    || text.contains("cat5") || text.contains("cat.6")
	    || text.contains("cat6")) {
		return QStringLiteral("ethernet");
	}
	if (text.contains("m12")) {
		return QStringLiteral("m12");
	}
	if (text.contains("motor")) {
		return QStringLiteral("motor");
	}
	return QString();
}

/**
	@return the cables of a CSV file as exported from a manufacturer
	catalogue. The delimiter (; , or tab), the encoding (UTF-8 or
	Latin 1) and the column names are detected. Rows without article
	number are skipped.
*/
QList<Entry> readCsv(const QString &path, QString *error)
{
	QList<Entry> l;
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		if (error) {
			*error = file.errorString();
		}
		return l;
	}
	QByteArray data = file.readAll();
	if (data.startsWith("\xEF\xBB\xBF")) {
		data.remove(0, 3);
	}
	QString text = QString::fromUtf8(data);
	if (text.contains(QChar::ReplacementCharacter)) {
		text = QString::fromLatin1(data);
	}

	const QString first_line = text.section('\n', 0, 0);
	QChar delimiter = ';';
	int best = first_line.count(';');
	for (const QChar &c : {QChar(','), QChar('\t')}) {
		if (first_line.count(c) > best) {
			best = first_line.count(c);
			delimiter = c;
		}
	}

	const QList<QStringList> rows = parseCsv(text, delimiter);
	if (rows.isEmpty()) {
		return l;
	}
	QStringList fields;
	for (const QString &header : rows.first()) {
		fields << fieldOfHeader(header);
	}
	if (!fields.contains("reference")) {
		if (error) {
			*error = QStringLiteral("no article number column");
		}
		return l;
	}

	for (int r = 1; r < rows.size(); ++r) {
		Entry e;
		for (int c = 0; c < rows.at(r).size() && c < fields.size();
		     ++c) {
			const QString &f = fields.at(c);
			const QString value = rows.at(r).at(c).trimmed();
			if (f == "manufacturer") e.manufacturer = value;
			else if (f == "reference") e.reference = value;
			else if (f == "designation") e.designation = value;
			else if (f == "family") e.family = value;
			else if (f == "conductors") e.conductors = value.toInt();
			else if (f == "pairs") e.pairs = value.toInt();
			else if (f == "section") {
				e.section = value;
				e.section.remove(QStringLiteral("mm\u00b2"));
				e.section.remove(QStringLiteral("mm2"));
				e.section = e.section.trimmed();
			}
			else if (f == "diameter") e.diameter = value;
			else if (f == "shield") e.shield = value;
			else if (f == "sheath") e.sheath = value;
			else if (f == "color_code") e.color_code = value;
			else if (f == "usage") e.usage = value;
			else if (f == "length") e.length = value;
		}
		if (e.reference.isEmpty()) {
			continue;
		}
		if (e.usage.isEmpty()) {
			e.usage = inferredUsage(e);
		}
		l << e;
	}
	return l;
}

/**
	@return base completed with extra; a cable of extra replaces the
	one of base with the same manufacturer and article number.
*/
QList<Entry> merged(const QList<Entry> &base, const QList<Entry> &extra)
{
	QList<Entry> result = base;
	for (const Entry &e : extra) {
		bool replaced = false;
		for (Entry &r : result) {
			if (r.reference == e.reference
			    && r.manufacturer == e.manufacturer) {
				r = e;
				replaced = true;
				break;
			}
		}
		if (!replaced) {
			result << e;
		}
	}
	return result;
}

/**
	@return the built-in examples completed by the catalogue of the user
*/
QList<Entry> entries()
{
	return merged(builtin(), readJson(userFile()));
}

/**
	Add the cables of a CSV file to the catalogue of the user.
	@return how many cables were read, -1 when the file is not usable
*/
int importCsv(const QString &path, QString *error)
{
	const QList<Entry> imported = readCsv(path, error);
	if (imported.isEmpty()) {
		return error && !error->isEmpty() ? -1 : 0;
	}
	QString read_error;
	const QList<Entry> user = readJson(userFile(), &read_error);
	if (!read_error.isEmpty()) {
		if (error) {
			*error = read_error;
		}
		return -1;
	}
	if (!writeJson(userFile(), merged(user, imported))) {
		if (error) {
			*error = QStringLiteral("cannot write ") + userFile();
		}
		return -1;
	}
	return imported.size();
}

/**
	@return the id of the network cable type which suits a cable
*/
QString networkTypeId(const Entry &entry)
{
	QString usage;
	for (const QChar &c : entry.usage.toLower()) {
		if (c.isLetterOrNumber()) {
			usage += c;
		}
	}
	if (usage == "motor" || usage == "servo") {
		return QStringLiteral("motor");
	}
	if (usage == "ethernet") {
		return QStringLiteral("ethernet");
	}
	if (usage == "ethercat") {
		return QStringLiteral("ethercat");
	}
	if (usage == "ethercatp") {
		return QStringLiteral("ethercat_p");
	}
	if (usage == "ym12" || usage == "y") {
		return QStringLiteral("y_m12");
	}
	if (usage == "m12") {
		if (entry.conductors == 4) {
			return QStringLiteral("m12_4");
		}
		if (entry.conductors == 8) {
			return QStringLiteral("m12_8");
		}
		return QStringLiteral("m12_5");
	}
	return QStringLiteral("generic");
}

/**
	@return the text which stands for a cable in a list
*/
QString title(const Entry &entry)
{
	QString t = entry.reference + QStringLiteral(" - ");
	if (!entry.manufacturer.isEmpty()) {
		t += entry.manufacturer + QLatin1Char(' ');
	}
	if (!entry.family.isEmpty()) {
		t += entry.family + QLatin1Char(' ');
	}
	return (t + entry.designation).trimmed();
}

/**
	Make properties describe the cable of the catalogue. The cable type
	already chosen is kept, otherwise the one which suits the cable is
	taken.
*/
void apply(ConductorProperties &properties, const Entry &entry)
{
	if (properties.m_network_cable.isEmpty()) {
		NetworkCables::apply(properties, networkTypeId(entry));
	}
	properties.m_network_reference = entry.reference;
	QString designation = entry.designation;
	if (!entry.family.isEmpty()) {
		designation = entry.family + QLatin1Char(' ') + designation;
	}
	properties.m_conductor_type = designation.trimmed();
	if (!entry.section.isEmpty()) {
		properties.m_wire_section = entry.section;
	}
	if (!entry.length.isEmpty()) {
		properties.m_network_length = entry.length;
	}
}
}
