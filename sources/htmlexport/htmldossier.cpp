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
#include "htmldossier.h"

#include "../cli_export.h"
#include "../dataBase/projectdatabase.h"
#include "../diagram.h"
#include "../diagramcontext.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/crossrefitem.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/independenttextitem.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetproject.h"
#include "dossierfunctionmap.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QGraphicsTextItem>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QMap>
#include <QPainter>
#include <QPainterPath>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSvgGenerator>
#include <QUuid>
#include <QVariantMap>

namespace {

class Tr
{
	Q_DECLARE_TR_FUNCTIONS(HtmlDossier)
};

using Row = QVariantMap;

QString esc(const QString &text)
{
	return text.toHtmlEscaped();
}

QString normUuid(const QString &text)
{
	const QUuid uuid = QUuid::fromString(text.trimmed());
	return uuid.isNull() ? text.trimmed().toLower()
						 : uuid.toString(QUuid::WithoutBraces);
}

QString normUuid(const QUuid &uuid)
{
	return uuid.toString(QUuid::WithoutBraces);
}

QString sv(const Row &row, const char *key)
{
	return row.value(QLatin1String(key)).toString().trimmed();
}

QString num(qreal value)
{
	return QString::number(value, 'f', 1);
}

/// The pixel rect of the page of a folio: the same as the other exports
QRect pageRect(Diagram *diagram)
{
	QRectF r = diagram->border_and_titleblock.borderAndTitleBlockRect();
	r.adjust(0, 0, 1, 1);
	return r.toAlignedRect();
}

QString pathData(const QPainterPath &path, const QPointF &origin)
{
	QString d;
	auto point = [&](const QPointF &p) {
		return num(p.x() - origin.x()) + QLatin1Char(',')
			+ num(p.y() - origin.y());
	};
	for (int i = 0; i < path.elementCount(); ++i) {
		const QPainterPath::Element e = path.elementAt(i);
		switch (e.type) {
		case QPainterPath::MoveToElement:
			d += QLatin1Char('M') + point(e);
			break;
		case QPainterPath::LineToElement:
			d += QLatin1Char('L') + point(e);
			break;
		case QPainterPath::CurveToElement:
			if (i + 2 < path.elementCount()) {
				d += QLatin1Char('C') + point(e) + QLatin1Char(' ')
					+ point(path.elementAt(i + 1)) + QLatin1Char(' ')
					+ point(path.elementAt(i + 2));
				i += 2;
			}
			break;
		default:
			break;
		}
	}
	return d;
}

bool writeFile(const QString &path, const QByteArray &data)
{
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;
	return file.write(data) == data.size();
}

bool writeFile(const QString &path, const QString &text)
{
	return writeFile(path, text.toUtf8());
}

QByteArray resource(const QString &name)
{
	QFile file(QStringLiteral(":/htmlexport/assets/") + name);
	return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

QString jsonScript(const QString &variable, const QJsonValue &value)
{
	QJsonDocument doc;
	if (value.isArray())
		doc = QJsonDocument(value.toArray());
	else
		doc = QJsonDocument(value.toObject());
	return QStringLiteral("window.") + variable + QLatin1Char('=')
		+ QString::fromUtf8(doc.toJson(QJsonDocument::Compact))
		+ QStringLiteral(";\n");
}

QString trimmedNumber(double value)
{
	QString text = QString::number(value, 'f', 2);
	while (text.endsWith(QLatin1Char('0')))
		text.chop(1);
	if (text.endsWith(QLatin1Char('.')))
		text.chop(1);
	return text;
}

/// One cross reference of an element, from the database
struct XRef {
	int folio = 0;
	QString label;
	QString uuid;
	QString position;
};

/// A line of the bill of materials
struct BomGroup {
	QString designation, manufacturer, article, supplier, model;
	double quantity = 0.0;
	struct Ref {
		QString label, uuid;
		int folio = 0;
	};
	QList<Ref> refs;
};

/// Everything the pages share while a dossier is being written
struct Context {
	QETProject *project = nullptr;
	QString dir;
	HtmlDossier::Options options;
	QList<Diagram *> folios;      ///< folio n is at index n - 1
	QSet<int> exported;
	QHash<QString, int> uuidToFolio;
	QHash<int, int> numberToFolio; ///< folio_number of the database -> n
	QHash<QString, Row> devices;  ///< by element uuid
	QHash<QString, int> bomOf;    ///< element uuid -> BomGroup index
	QList<BomGroup> bom;
	QHash<QString, QList<Row>> ioOf; ///< slave uuid -> qet_plc_io rows
	QHash<QString, QList<XRef>> xrefs;
	QStringList warnings;
	QList<QJsonArray> search;     ///< entries of the global search
	bool hasPdf = false;
	int count() const { return folios.size(); }

	int folioOfNumber(const QString &text) const
	{
		bool ok = false;
		const int number = text.toInt(&ok);
		if (!ok)
			return 0;
		return numberToFolio.value(number, number);
	}
	QString href(int folio, const QString &uuid = QString()) const
	{
		if (!exported.contains(folio))
			return QString();
		QString h = HtmlDossier::folioPageName(folio, count());
		if (!uuid.isEmpty())
			h += QStringLiteral("#e=") + uuid;
		return h;
	}
	QString link(int folio, const QString &uuid, const QString &text) const
	{
		const QString h = href(folio, uuid);
		return h.isEmpty() ? esc(text)
						   : QStringLiteral("<a href=\"%1\">%2</a>")
								 .arg(h, esc(text));
	}
	void addSearch(const QString &kind, const QString &text,
				   const QString &url, const QString &extra = QString())
	{
		if (text.trimmed().isEmpty() || url.isEmpty())
			return;
		search.append(QJsonArray{kind, text, url, extra});
	}
};

QList<Row> runQuery(Context &c, const QString &sql)
{
	QList<Row> rows;
	QString error;
	QSqlQuery query = c.project->dataBase()->newQuery(sql, &error);
	if (error.isEmpty() && query.lastError().isValid())
		error = query.lastError().text();
	if (!error.isEmpty()) {
		c.warnings << Tr::tr("Database query failed (%1): %2")
						  .arg(sql.left(60), error);
		return rows;
	}
	while (query.next()) {
		const QSqlRecord record = query.record();
		Row row;
		for (int i = 0; i < record.count(); ++i)
			row.insert(record.fieldName(i), query.value(i));
		rows.append(row);
	}
	return rows;
}

QString folioStem(int folio, int count)
{
	const int digits = qMax(2, QString::number(count).size());
	return QStringLiteral("folio-%1")
		.arg(folio, digits, 10, QLatin1Char('0'));
}

QString elementLabel(Element *element)
{
	const QString label =
		element->elementInformations()["label"].toString().trimmed();
	return label.isEmpty() ? element->name() : label;
}

/// The words of the pages; the keys are the English texts the scripts ask for
QString commonScript(const Context &c)
{
	QJsonObject i18n;
	auto add = [&](const char *key, const QString &text) {
		i18n.insert(QLatin1String(key), text);
	};
	add("Name", Tr::tr("Name"));
	add("Element", Tr::tr("Element"));
	add("Terminal", Tr::tr("Terminal"));
	add("Wire", Tr::tr("Wire"));
	add("Connected to", Tr::tr("Connected to"));
	add("Function", Tr::tr("Function"));
	add("Colour", Tr::tr("Colour"));
	add("Section", Tr::tr("Section"));
	add("From", Tr::tr("From"));
	add("To", Tr::tr("To"));
	add("Cable", Tr::tr("Cable"));
	add("I/O", Tr::tr("I/O"));
	add("Go to", Tr::tr("Go to"));
	add("No folio", Tr::tr("No folio"));
	add("line", Tr::tr("line"));
	add("Click an element to see its details.",
		Tr::tr("Click an element to see its details."));
	add("Bill of materials:", Tr::tr("Bill of materials:"));
	add("Devices list:", Tr::tr("Devices list:"));
	add("Cross reference:", Tr::tr("Cross reference:"));
	add("label", Tr::tr("Label"));
	add("function", Tr::tr("Function"));
	add("designation", Tr::tr("Designation"));
	add("description", Tr::tr("Description"));
	add("manufacturer", Tr::tr("Manufacturer"));
	add("manufacturer_reference", Tr::tr("Manufacturer reference"));
	add("supplier", Tr::tr("Supplier"));
	add("comment", Tr::tr("Comment"));
	add("plant", Tr::tr("Plant"));
	add("location", Tr::tr("Location"));
	add("quantity", Tr::tr("Quantity"));
	add("formula", Tr::tr("Formula"));
	QJsonObject files;
	for (int n = 1; n <= c.count(); ++n)
		files.insert(QString::number(n),
					 HtmlDossier::folioPageName(n, c.count()));
	return jsonScript(QStringLiteral("DOSSIER_I18N"), i18n)
		+ jsonScript(QStringLiteral("DOSSIER_FILES"), files);
}

struct NavItem {
	QString file, title;
};

QString pageHead(const Context &c, const QString &title, const QString &page,
				 const QString &active)
{
	const QList<NavItem> nav = {
		{QStringLiteral("index.html"), Tr::tr("Overview")},
		{QStringLiteral("nomenclature.html"), Tr::tr("Bill of materials")},
		{QStringLiteral("equipment.html"), Tr::tr("Devices")},
		{QStringLiteral("io.html"), Tr::tr("I/O")},
		{QStringLiteral("wires.html"), Tr::tr("Wires")},
		{QStringLiteral("terminals.html"), Tr::tr("Terminals")},
		{QStringLiteral("cables.html"), Tr::tr("Cables")},
		{QStringLiteral("search.html"), Tr::tr("Search")},
	};
	QString html = QStringLiteral(
		"<!DOCTYPE html>\n<html lang=\"%1\">\n<head>\n"
		"<meta charset=\"utf-8\">\n"
		"<meta name=\"viewport\" content=\"width=device-width,"
		"initial-scale=1\">\n<title>%2</title>\n"
		"<link rel=\"stylesheet\" href=\"dossier.css\">\n</head>\n"
		"<body data-page=\"%3\">\n<header class=\"top\">\n"
		"<span class=\"title\">%4</span>\n")
		.arg(QLocale().name().left(2), esc(title), page,
			 esc(c.project->title().isEmpty()
					 ? QFileInfo(c.project->filePath()).completeBaseName()
					 : c.project->title()));
	for (const NavItem &item : nav)
		html += QStringLiteral("<a href=\"%1\"%2>%3</a>\n")
					.arg(item.file,
						 item.file == active
							 ? QStringLiteral(" class=\"active\"")
							 : QString(),
						 esc(item.title));
	if (c.hasPdf)
		html += QStringLiteral("<a href=\"dossier.pdf\">PDF</a>\n");
	html += QStringLiteral(
		"<form action=\"search.html\" method=\"get\">"
		"<input type=\"search\" name=\"q\" placeholder=\"%1\"></form>\n"
		"</header>\n").arg(esc(Tr::tr("Search the dossier...")));
	return html;
}

QString scripts(const QString &data = QString())
{
	QString html = QStringLiteral(
		"<script src=\"data/common.js\"></script>\n");
	if (!data.isEmpty())
		html += QStringLiteral("<script src=\"%1\"></script>\n").arg(data);
	html += QStringLiteral("<script src=\"dossier.js\"></script>\n"
						   "</body>\n</html>\n");
	return html;
}

struct TableRow {
	QString id;
	QStringList cells; ///< HTML
};

bool tablePage(const Context &c, const QString &file, const QString &title,
			   const QString &intro, const QStringList &headers,
			   const QList<TableRow> &rows)
{
	QString html = pageHead(c, title, QStringLiteral("table"), file);
	html += QStringLiteral("<main>\n<h1>%1</h1>\n").arg(esc(title));
	if (!intro.isEmpty())
		html += QStringLiteral("<p class=\"muted\">%1</p>\n").arg(esc(intro));
	html += QStringLiteral(
		"<div class=\"bar\"><input id=\"filter\" type=\"search\" "
		"placeholder=\"%1\" autofocus> <span id=\"count\" "
		"class=\"muted\"></span></div>\n"
		"<table class=\"data\">\n<thead><tr>")
		.arg(esc(Tr::tr("Filter...")));
	for (const QString &h : headers)
		html += QStringLiteral("<th>%1</th>").arg(esc(h));
	html += QStringLiteral("</tr></thead>\n<tbody>\n");
	for (const TableRow &row : rows) {
		html += row.id.isEmpty()
			? QStringLiteral("<tr>")
			: QStringLiteral("<tr id=\"%1\">").arg(esc(row.id));
		for (const QString &cell : row.cells)
			html += QStringLiteral("<td>%1</td>").arg(cell);
		html += QStringLiteral("</tr>\n");
	}
	html += QStringLiteral("</tbody>\n</table>\n</main>\n") + scripts();
	return writeFile(QDir(c.dir).filePath(file), html);
}

/// The text of the folios a function is deduced from
void collectTexts(Diagram *diagram, QStringList *texts)
{
	QSet<QString> seen;
	auto add = [&](const QString &text) {
		const QString t = text.simplified();
		if (t.isEmpty() || t.size() > 200 || seen.contains(t))
			return;
		seen.insert(t);
		texts->append(t);
	};
	add(diagram->title());
	const auto items = diagram->items();
	for (QGraphicsItem *item : items) {
		if (auto *text = dynamic_cast<QGraphicsTextItem *>(item))
			add(text->toPlainText());
	}
}

} // anonymous namespace

namespace HtmlDossier {

QString folioPageName(int number, int count)
{
	return folioStem(number, count) + QStringLiteral(".html");
}

namespace {

/// Data of the database that the folio pages and the tables share
void loadDatabase(Context &c)
{
	c.project->dataBase()->updateDB();

	const QList<Row> folioRows = runQuery(
		c, QStringLiteral("SELECT * FROM qet_folios"));
	for (const Row &row : folioRows) {
		const int number = sv(row, "folio_number").toInt();
		const int folio =
			c.uuidToFolio.value(normUuid(sv(row, "folio_uuid")), 0);
		if (number > 0 && folio > 0)
			c.numberToFolio.insert(number, folio);
	}

	QMap<QString, BomGroup> groups;
	const QList<Row> devices = runQuery(
		c, QStringLiteral("SELECT * FROM qet_devices"));
	for (const Row &row : devices) {
		const QString uuid = normUuid(sv(row, "device_uuid"));
		c.devices.insert(uuid, row);
		const QString kind = sv(row, "kind").toLower();
		if (kind == QLatin1String("slave") || kind.contains("report"))
			continue;
		const QString designation = sv(row, "designation");
		const QString manufacturer = sv(row, "manufacturer");
		const QString article = sv(row, "article");
		const QString supplier = sv(row, "supplier");
		const QString model = sv(row, "model");
		if (designation.isEmpty() && manufacturer.isEmpty()
			&& article.isEmpty() && supplier.isEmpty() && model.isEmpty())
			continue;
		const QString key = designation.toLower() + QChar(0x1f)
			+ manufacturer.toLower() + QChar(0x1f) + article.toLower()
			+ QChar(0x1f) + supplier.toLower() + QChar(0x1f)
			+ model.toLower();
		BomGroup &group = groups[key];
		group.designation = designation;
		group.manufacturer = manufacturer;
		group.article = article;
		group.supplier = supplier;
		group.model = model;
		double quantity = sv(row, "quantity").replace(',', '.').toDouble();
		if (quantity <= 0.0)
			quantity = 1.0;
		group.quantity += quantity;
		group.refs.append({sv(row, "label"), uuid,
						   c.folioOfNumber(sv(row, "folio_number"))});
	}
	int index = 0;
	for (auto it = groups.begin(); it != groups.end(); ++it, ++index) {
		for (const BomGroup::Ref &ref : std::as_const(it->refs))
			c.bomOf.insert(ref.uuid, index);
		c.bom.append(it.value());
	}

	const QList<Row> ios = runQuery(
		c, QStringLiteral("SELECT * FROM qet_plc_io"));
	for (const Row &row : ios) {
		const QString slave = normUuid(sv(row, "slave_uuid"));
		if (!slave.isEmpty())
			c.ioOf[slave].append(row);
	}

	const QList<Row> links = runQuery(
		c, QStringLiteral("SELECT * FROM qet_links"));
	for (const Row &row : links) {
		const QString a = normUuid(sv(row, "device_uuid"));
		const QString b = normUuid(sv(row, "linked_uuid"));
		c.xrefs[a].append({c.folioOfNumber(sv(row, "linked_folio_number")),
						   sv(row, "linked_device"), b,
						   sv(row, "linked_position")});
		c.xrefs[b].append({c.folioOfNumber(sv(row, "folio_number")),
						   sv(row, "device"), a, sv(row, "position")});
	}
}

/// A cell: the number of a folio, linked to it
QString folioCell(const Context &c, const QString &number,
				  const QString &uuid = QString())
{
	const int folio = c.folioOfNumber(number);
	return folio > 0 ? c.link(folio, uuid, number) : esc(number);
}

bool writeBomPage(Context &c)
{
	QList<TableRow> rows;
	int index = 0;
	for (const BomGroup &g : std::as_const(c.bom)) {
		TableRow row;
		row.id = QStringLiteral("b%1").arg(index);
		QStringList refs;
		for (const BomGroup::Ref &ref : g.refs)
			refs << c.link(ref.folio, ref.uuid, ref.label);
		row.cells = {QString::number(++index), trimmedNumber(g.quantity),
					 esc(g.designation), esc(g.manufacturer),
					 esc(g.article), esc(g.supplier), esc(g.model),
					 refs.join(QStringLiteral(", "))};
		rows.append(row);
		const QString url = QStringLiteral("nomenclature.html#") + row.id;
		c.addSearch(Tr::tr("Bill of materials"),
					g.designation + QLatin1Char(' ') + g.article
						+ QLatin1Char(' ') + g.manufacturer + QLatin1Char(' ')
						+ g.model, url);
	}
	return tablePage(c, QStringLiteral("nomenclature.html"),
					 Tr::tr("Bill of materials"),
					 Tr::tr("Symbols with the same designation, manufacturer, "
							"article, supplier and model are one line. "
							"The references lead to the symbols."),
					 {Tr::tr("No."), Tr::tr("Quantity"), Tr::tr("Designation"),
					  Tr::tr("Manufacturer"), Tr::tr("Article"),
					  Tr::tr("Supplier"), Tr::tr("Model"),
					  Tr::tr("References")},
					 rows);
}

bool writeEquipmentPage(Context &c)
{
	QList<TableRow> rows;
	QList<Row> sorted = c.devices.values();
	std::sort(sorted.begin(), sorted.end(), [](const Row &a, const Row &b) {
		const int fa = sv(a, "folio_number").toInt();
		const int fb = sv(b, "folio_number").toInt();
		if (fa != fb)
			return fa < fb;
		return sv(a, "position") < sv(b, "position");
	});
	for (const Row &d : std::as_const(sorted)) {
		const QString kind = sv(d, "kind");
		if (kind.toLower().contains("report"))
			continue;
		const QString uuid = normUuid(sv(d, "device_uuid"));
		const int bom = c.bomOf.value(uuid, -1);
		TableRow row;
		row.id = QStringLiteral("d-") + uuid;
		row.cells = {esc(sv(d, "label")),
					 folioCell(c, sv(d, "folio_number"), uuid),
					 esc(sv(d, "position")), esc(kind),
					 esc(sv(d, "function")), esc(sv(d, "description")),
					 esc(sv(d, "designation")), esc(sv(d, "article")),
					 bom < 0 ? QString()
							 : QStringLiteral("<a href=\"nomenclature.html#b%1\">"
											  "%2</a>").arg(bom).arg(bom + 1)};
		rows.append(row);
		c.addSearch(Tr::tr("Device"),
					sv(d, "label") + QLatin1Char(' ') + sv(d, "function")
						+ QLatin1Char(' ') + sv(d, "designation")
						+ QLatin1Char(' ') + sv(d, "article"),
					c.href(c.folioOfNumber(sv(d, "folio_number")), uuid),
					Tr::tr("Folio %1").arg(sv(d, "folio_number")));
	}
	return tablePage(c, QStringLiteral("equipment.html"), Tr::tr("Devices"),
					 Tr::tr("Every symbol of the project with a label."),
					 {Tr::tr("Label"), Tr::tr("Folio"), Tr::tr("Position"),
					  Tr::tr("Kind"), Tr::tr("Function"),
					  Tr::tr("Description"), Tr::tr("Designation"),
					  Tr::tr("Article"), Tr::tr("Bill of materials")},
					 rows);
}

bool writeIoPage(Context &c)
{
	QList<TableRow> rows;
	const QList<Row> ios = runQuery(
		c, QStringLiteral("SELECT * FROM qet_plc_io"));
	for (const Row &io : ios) {
		const QString slave = normUuid(sv(io, "slave_uuid"));
		const QString plcFolio = sv(io, "plc_folio_number");
		TableRow row;
		row.cells = {esc(sv(io, "plc")), esc(sv(io, "entry")),
					 esc(sv(io, "address")), esc(sv(io, "type")),
					 esc(sv(io, "function")), esc(sv(io, "comment")),
					 folioCell(c, sv(io, "folio_number"), slave),
					 folioCell(c, plcFolio, normUuid(sv(io, "plc_uuid")))};
		rows.append(row);
		const int target = c.folioOfNumber(
			sv(io, "folio_number").isEmpty() ? plcFolio
											 : sv(io, "folio_number"));
		c.addSearch(Tr::tr("I/O"),
					sv(io, "address") + QLatin1Char(' ') + sv(io, "plc")
						+ QLatin1Char(' ') + sv(io, "function")
						+ QLatin1Char(' ') + sv(io, "comment"),
					c.href(target, slave.isEmpty() ? normUuid(sv(io, "plc_uuid"))
												   : slave));
	}
	return tablePage(c, QStringLiteral("io.html"), Tr::tr("I/O"),
					 Tr::tr("The entries of the I/O tables of the controllers "
							"and the symbols they are linked to."),
					 {Tr::tr("Controller"), Tr::tr("Entry"),
					  Tr::tr("Address"), Tr::tr("Type"), Tr::tr("Function"),
					  Tr::tr("Comment"), Tr::tr("Folio"),
					  Tr::tr("Controller folio")},
					 rows);
}

bool writeWiresPage(Context &c)
{
	QList<TableRow> rows;
	const QList<Row> wires = runQuery(
		c, QStringLiteral("SELECT * FROM qet_wires"));
	for (const Row &w : wires) {
		auto end = [&](const char *device, const char *terminal) {
			return sv(w, device)
				+ (sv(w, terminal).isEmpty() ? QString()
											 : QLatin1Char(':')
												   + sv(w, terminal));
		};
		const QString from = end("from_device", "from_terminal");
		const QString to = end("to_device", "to_terminal");
		const QString fromUuid = normUuid(sv(w, "from_device_uuid"));
		TableRow row;
		row.cells = {esc(sv(w, "wire_number")), esc(from), esc(to),
					 esc(sv(w, "colour")), esc(sv(w, "section")),
					 esc(sv(w, "cable")), esc(sv(w, "function")),
					 esc(sv(w, "voltage")),
					 folioCell(c, sv(w, "folio_number"), fromUuid)};
		rows.append(row);
		c.addSearch(Tr::tr("Wire"),
					sv(w, "wire_number") + QLatin1Char(' ') + from
						+ QStringLiteral(" > ") + to + QLatin1Char(' ')
						+ sv(w, "function"),
					c.href(c.folioOfNumber(sv(w, "folio_number")), fromUuid),
					Tr::tr("Folio %1").arg(sv(w, "folio_number")));
	}
	return tablePage(c, QStringLiteral("wires.html"), Tr::tr("Wires"), QString(),
					 {Tr::tr("Wire"), Tr::tr("From"), Tr::tr("To"),
					  Tr::tr("Colour"), Tr::tr("Section"), Tr::tr("Cable"),
					  Tr::tr("Function"), Tr::tr("Voltage"), Tr::tr("Folio")},
					 rows);
}

/// A table of every column of a view of the database, but its identifiers
bool writeViewPage(Context &c, const QString &file, const QString &title,
				   const QString &view, const QString &kind)
{
	const QList<Row> rows = runQuery(
		c, QStringLiteral("SELECT * FROM ") + view);
	QStringList columns;
	if (!rows.isEmpty()) {
		for (auto it = rows.first().begin(); it != rows.first().end(); ++it)
			if (!it.key().endsWith(QLatin1String("uuid")))
				columns << it.key();
	}
	QList<TableRow> out;
	for (const Row &r : rows) {
		TableRow row;
		const QString uuid = normUuid(sv(r, "element_uuid"));
		QString text;
		for (const QString &col : std::as_const(columns)) {
			const QString value = r.value(col).toString().trimmed();
			row.cells << (col == QLatin1String("folio_number")
							  ? folioCell(c, value, uuid)
							  : esc(value));
			text += value + QLatin1Char(' ');
		}
		out.append(row);
		c.addSearch(kind, text,
					c.href(c.folioOfNumber(sv(r, "folio_number")), uuid));
	}
	QStringList headers;
	for (const QString &col : std::as_const(columns)) {
		QString h = col;
		h.replace(QLatin1Char('_'), QLatin1Char(' '));
		headers << h;
	}
	return tablePage(c, file, title, QString(), headers, out);
}

/// The data and the page of a folio, and its picture
struct FolioResult {
	QStringList texts;
	bool ok = false;
};

FolioResult writeFolio(Context &c, int number)
{
	FolioResult result;
	Diagram *diagram = c.folios.at(number - 1);
	c.project->buildFolioToShow(diagram);

	const QString stem = folioStem(number, c.count());
	const QRect page = pageRect(diagram);
	const QPointF origin = page.topLeft();
	const QRectF pageF(QPointF(0, 0), QSizeF(page.size()));

	// The picture: the SVG export of the other commands
	{
		QSvgGenerator generator;
		generator.setFileName(QDir(c.dir).filePath(stem + QStringLiteral(".svg")));
		generator.setSize(page.size());
		generator.setViewBox(QRect(0, 0, page.width(), page.height()));
		generator.setTitle(diagram->title());
		QPainter painter(&generator);
		const bool grid = diagram->displayGrid();
		const bool guides = diagram->displayGuides();
		const bool terminals = diagram->drawTerminals();
		const bool terminalNames = diagram->drawTerminalNames();
		diagram->setDisplayGrid(false);
		diagram->setDisplayGuides(false);
		diagram->setDrawTerminals(c.options.showTerminals);
		diagram->setDrawTerminalNames(c.options.showTerminals);
		diagram->setSmallPictures(true);
		diagram->render(&painter, QRectF(QPointF(0, 0), QSizeF(page.size())),
						page, Qt::KeepAspectRatio);
		diagram->setSmallPictures(false);
		diagram->setDisplayGrid(grid);
		diagram->setDisplayGuides(guides);
		diagram->setDrawTerminals(terminals);
		diagram->setDrawTerminalNames(terminalNames);
		painter.end();
	}

	auto sceneRect = [&](QGraphicsItem *item) {
		return item->mapRectToScene(item->boundingRect())
			.translated(-origin).intersected(pageF);
	};

	QJsonArray elementsJson, conductorsJson, terminalsJson, linksJson;
	QHash<Element *, int> indexOf;
	const QList<Element *> elements = diagram->elements();
	for (Element *element : elements) {
		const QRectF r = sceneRect(element);
		if (!r.isValid() || r.isEmpty())
			continue;
		const QString uuid = normUuid(element->uuid());
		const QString label = elementLabel(element);
		QJsonObject info;
		const DiagramContext context = element->elementInformations();
		for (const QString &key : context.keys()) {
			const QString value = context.value(key).toString().trimmed();
			if (!value.isEmpty())
				info.insert(key, value);
		}
		QJsonArray ioJson;
		for (const Row &io : c.ioOf.value(uuid))
			ioJson.append(QJsonObject{
				{"address", sv(io, "address")}, {"type", sv(io, "type")},
				{"entry", sv(io, "entry")}, {"function", sv(io, "function")},
				{"comment", sv(io, "comment")}});
		QJsonArray xrefJson;
		for (const XRef &x : c.xrefs.value(uuid))
			xrefJson.append(QJsonObject{
				{"folio", x.folio}, {"label", x.label}, {"uuid", x.uuid},
				{"position", x.position}});
		QJsonObject json{
			{"uuid", uuid}, {"x", r.x()}, {"y", r.y()}, {"w", r.width()},
			{"h", r.height()}, {"label", label}, {"name", element->name()},
			{"info", info}, {"io", ioJson}, {"xref", xrefJson},
			{"bom", c.bomOf.value(uuid, -1)}};
		indexOf.insert(element, elementsJson.size());
		elementsJson.append(json);

		if (element->linkType() & Element::AllReport) {
			const QList<XRef> refs = c.xrefs.value(uuid);
			if (!refs.isEmpty()) {
				const QString h = c.href(refs.first().folio, refs.first().uuid);
				if (!h.isEmpty())
					linksJson.append(QJsonObject{
						{"x", r.x()}, {"y", r.y()}, {"w", r.width()},
						{"h", r.height()}, {"href", h},
						{"tip", Tr::tr("Folio %1 - %2")
									.arg(refs.first().folio)
									.arg(refs.first().label)}});
			}
		}
		result.texts << label;
		for (auto it = info.begin(); it != info.end(); ++it)
			result.texts << it.value().toString();
	}

	for (Conductor *conductor : diagram->conductors()) {
		const ConductorProperties props = conductor->properties();
		auto end = [](Terminal *t) {
			if (!t)
				return QString();
			Element *parent = t->parentElement();
			return (parent ? elementLabel(parent) : QString())
				+ QLatin1Char(':') + t->name();
		};
		const QString d = pathData(conductor->mapToScene(conductor->path()),
								   origin);
		if (d.isEmpty())
			continue;
		conductorsJson.append(QJsonObject{
			{"d", d}, {"wire", props.text}, {"from", end(conductor->terminal1)},
			{"to", end(conductor->terminal2)},
			{"function", props.m_function}, {"colour", props.m_wire_color},
			{"section", props.m_wire_section}, {"cable", props.m_cable}});
		result.texts << props.text;
	}

	for (Element *element : elements) {
		const int ei = indexOf.value(element, -1);
		const QString uuid = normUuid(element->uuid());
		const QList<Row> ios = c.ioOf.value(uuid);
		for (Terminal *terminal : element->terminals()) {
			const QPointF p = terminal->scenePos() - origin;
			if (!pageF.contains(p))
				continue;
			QString wire, to, function, colour, section;
			for (Conductor *conductor : terminal->conductors()) {
				const ConductorProperties props = conductor->properties();
				Terminal *other = conductor->terminal1 == terminal
					? conductor->terminal2 : conductor->terminal1;
				auto join = [](QString *into, const QString &value) {
					if (!value.isEmpty() && !into->contains(value))
						*into += (into->isEmpty() ? QString()
												  : QStringLiteral(", ")) + value;
				};
				join(&wire, props.text);
				if (other && other->parentElement())
					join(&to, elementLabel(other->parentElement())
								  + QLatin1Char(':') + other->name());
				join(&function, props.m_function);
				join(&colour, props.m_wire_color);
				join(&section, props.m_wire_section);
			}
			QJsonArray ioJson;
			for (const Row &io : ios)
				if (sv(io, "entry") == terminal->name())
					ioJson.append(QJsonObject{
						{"address", sv(io, "address")}, {"type", sv(io, "type")},
						{"function", sv(io, "function")}});
			terminalsJson.append(QJsonObject{
				{"e", ei}, {"name", terminal->name()}, {"x", p.x()},
				{"y", p.y()}, {"wire", wire}, {"to", to},
				{"function", function}, {"colour", colour},
				{"section", section}, {"io", ioJson}});
		}
	}

	for (QGraphicsItem *item : diagram->items()) {
		if (auto *xref = dynamic_cast<CrossRefItem *>(item)) {
			const auto &contacts = xref->hoveredContactsMap();
			for (auto it = contacts.begin(); it != contacts.end(); ++it) {
				Element *target = it.key();
				if (!target || !target->diagram())
					continue;
				const int folio = c.uuidToFolio.value(
					normUuid(target->diagram()->uuid()), 0);
				const QString h = c.href(folio, normUuid(target->uuid()));
				const QRectF r = xref->mapRectToScene(it.value())
					.translated(-origin);
				if (h.isEmpty() || !r.isValid())
					continue;
				linksJson.append(QJsonObject{
					{"x", r.x()}, {"y", r.y()}, {"w", r.width()},
					{"h", r.height()}, {"href", h},
					{"tip", Tr::tr("Folio %1 - %2")
								.arg(folio).arg(elementLabel(target))}});
			}
		} else if (auto *text = dynamic_cast<IndependentTextItem *>(item)) {
			if (text->destination().isNull())
				continue;
			const int folio = c.uuidToFolio.value(
				normUuid(text->destination()), 0);
			const QString h = c.href(folio);
			const QRectF r = sceneRect(text);
			if (h.isEmpty() || !r.isValid())
				continue;
			linksJson.append(QJsonObject{
				{"x", r.x()}, {"y", r.y()}, {"w", r.width()},
				{"h", r.height()}, {"href", h},
				{"tip", Tr::tr("Folio %1 - %2")
							.arg(folio).arg(c.folios.at(folio - 1)->title())}});
		}
	}

	collectTexts(diagram, &result.texts);

	QJsonObject folio{
		{"n", number}, {"title", diagram->title()}, {"w", page.width()},
		{"h", page.height()}, {"elements", elementsJson},
		{"conductors", conductorsJson}, {"terminals", terminalsJson},
		{"links", linksJson}};
	const QString prev = number > 1 && c.exported.contains(number - 1)
		? QString::number(number - 1) : QString();
	const QString next = number < c.count() && c.exported.contains(number + 1)
		? QString::number(number + 1) : QString();
	folio.insert("prev", prev.toInt());
	folio.insert("next", next.toInt());
	if (!writeFile(QDir(c.dir).filePath(QStringLiteral("data/") + stem
										+ QStringLiteral(".js")),
				   jsonScript(QStringLiteral("DOSSIER_FOLIO"), folio)))
		return result;

	QString html = pageHead(c, QStringLiteral("%1 - %2")
								   .arg(number).arg(diagram->title()),
							QStringLiteral("folio"), QString());
	html += QStringLiteral("<main>\n<div class=\"bar\">\n");
	html += prev.isEmpty()
		? QString()
		: QStringLiteral("<a class=\"btn\" href=\"%1\">&larr; %2</a>\n")
			  .arg(HtmlDossier::folioPageName(number - 1, c.count()),
				   esc(Tr::tr("Previous")));
	html += QStringLiteral("<strong>%1 / %2 - %3</strong>\n")
				.arg(number).arg(c.count()).arg(esc(diagram->title()));
	html += next.isEmpty()
		? QString()
		: QStringLiteral("<a class=\"btn\" href=\"%1\">%2 &rarr;</a>\n")
			  .arg(HtmlDossier::folioPageName(number + 1, c.count()),
				   esc(Tr::tr("Next")));
	html += QStringLiteral(
		"<a class=\"btn\" href=\"index.html#toc\">%1</a>\n"
		"<button id=\"zout\" title=\"%2\">-</button>"
		"<button id=\"zin\" title=\"%3\">+</button>"
		"<button id=\"zfit\">%4</button><button id=\"z100\">100%</button>\n"
		"<a class=\"btn\" href=\"%5.svg\">SVG</a>\n</div>\n")
		.arg(esc(Tr::tr("Folios")), esc(Tr::tr("Zoom out")),
			 esc(Tr::tr("Zoom in")), esc(Tr::tr("Fit")), stem);
	html += QStringLiteral(
		"<div class=\"folio-layout\">\n<div id=\"scroll\">"
		"<div id=\"viewer\"><img id=\"pic\" src=\"%1.svg\" alt=\"%2\">"
		"</div></div>\n<div id=\"details\"></div>\n</div>\n"
		"<div id=\"tip\"></div>\n</main>\n")
		.arg(stem, esc(diagram->title()));
	html += scripts(QStringLiteral("data/") + stem + QStringLiteral(".js"));
	result.ok = writeFile(QDir(c.dir).filePath(stem + QStringLiteral(".html")),
						  html);
	return result;
}

/// The overview picture, with the small script which tells the home page
/// which view is shown
bool copyOverview(Context &c, const QString &path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		c.warnings << Tr::tr("The overview picture '%1' cannot be read.")
						  .arg(path);
		return false;
	}
	QByteArray svg = file.readAll();
	file.close();
	const QByteArray script = QByteArray("<script type=\"text/javascript\">"
		"<![CDATA[\n") + resource(QStringLiteral("overview-bridge.js"))
		+ QByteArray("\n]]></script>\n");
	const int end = svg.lastIndexOf("</svg>");
	if (end < 0) {
		c.warnings << Tr::tr("'%1' is not an SVG picture.").arg(path);
		return false;
	}
	svg.insert(end, script);
	return writeFile(QDir(c.dir).filePath(QStringLiteral("overview.svg")), svg);
}

} // anonymous namespace

Result exportDossier(QETProject &project, const QString &dir,
					 const Options &options)
{
	Result result;
	QElapsedTimer timer;
	timer.start();

	Context c;
	c.project = &project;
	c.dir = dir;
	c.options = options;
	c.hasPdf = options.pdf;
	c.folios = project.folios();
	if (c.folios.isEmpty()) {
		result.error = Tr::tr("The project has no folio to export.");
		return result;
	}
	if (!QDir().mkpath(QDir(dir).filePath(QStringLiteral("data")))) {
		result.error = Tr::tr("The folder '%1' cannot be created.").arg(dir);
		return result;
	}
	for (int i = 0; i < c.folios.size(); ++i)
		c.uuidToFolio.insert(normUuid(c.folios.at(i)->uuid()), i + 1);

	QList<int> wanted = DossierFunctionMap::parseFolioList(
		options.folios, c.count());
	if (options.folios.trimmed().isEmpty()) {
		wanted.clear();
		for (int n = 1; n <= c.count(); ++n)
			wanted << n;
	}
	if (wanted.isEmpty()) {
		result.error = Tr::tr("No folio matches '%1'.").arg(options.folios);
		return result;
	}
	for (int n : std::as_const(wanted))
		c.exported.insert(n);

	const int total = wanted.size() + 3;
	int done = 0;
	auto progress = [&](const QString &what) {
		if (options.progress)
			options.progress(done, total, what);
	};

	progress(Tr::tr("Reading the database"));
	loadDatabase(c);
	++done;

	QList<QStringList> folioTexts;
	for (int n = 1; n <= c.count(); ++n)
		folioTexts << QStringList();
	for (int n : std::as_const(wanted)) {
		progress(Tr::tr("Folio %1 of %2").arg(n).arg(c.count()));
		const FolioResult folio = writeFolio(c, n);
		if (!folio.ok) {
			result.error = Tr::tr("The page of folio %1 cannot be written.")
							   .arg(n);
			result.warnings = c.warnings;
			return result;
		}
		folioTexts[n - 1] = folio.texts;
		++done;
	}

	progress(Tr::tr("Writing the tables"));
	bool tables = writeBomPage(c) && writeEquipmentPage(c) && writeIoPage(c)
		&& writeWiresPage(c)
		&& writeViewPage(c, QStringLiteral("terminals.html"),
						 Tr::tr("Terminals"), QStringLiteral("qet_terminals"),
						 Tr::tr("Terminal"))
		&& writeViewPage(c, QStringLiteral("cables.html"), Tr::tr("Cables"),
						 QStringLiteral("qet_cables"), Tr::tr("Cable"));
	if (!tables) {
		result.error = Tr::tr("A table cannot be written in '%1'.").arg(dir);
		return result;
	}

	for (int n : std::as_const(wanted)) {
		QStringList seen;
		Diagram *diagram = c.folios.at(n - 1);
		c.addSearch(Tr::tr("Folio"),
					QString::number(n) + QLatin1Char(' ') + diagram->title(),
					c.href(n));
		for (const QString &text : std::as_const(folioTexts[n - 1])) {
			if (seen.contains(text))
				continue;
			seen << text;
			c.addSearch(Tr::tr("Text"), text, c.href(n),
						Tr::tr("Folio %1").arg(n));
		}
	}

	// The functions of the overview picture and their folios
	QString overviewName;
	QString overviewAnchor = QStringLiteral("vue-ensemble");
	QList<DossierFunctionMap::Function> functions;
	if (!options.overviewSvg.trimmed().isEmpty()) {
		QString path = options.overviewSvg.trimmed();
		const int hash = path.indexOf(QLatin1Char('#'));
		if (hash >= 0) {
			overviewAnchor = path.mid(hash + 1);
			path.truncate(hash);
		}
		QFile file(path);
		if (file.open(QIODevice::ReadOnly)) {
			functions = DossierFunctionMap::functionsFromSvg(file.readAll());
			file.close();
			if (copyOverview(c, path))
				overviewName = QStringLiteral("overview.svg");
		} else {
			c.warnings << Tr::tr("The overview picture '%1' cannot be read.")
							  .arg(path);
		}
	}

	DossierFunctionMap::Map map = DossierFunctionMap::deduce(
		functions, folioTexts);
	QString mapPath = options.mapFile.trimmed();
	if (mapPath.isEmpty()) {
		const QFileInfo info(project.filePath());
		const QString base = info.absoluteDir().filePath(info.completeBaseName());
		for (const QString &suffix : {QStringLiteral(".functions.json"),
									  QStringLiteral(".functions.csv")})
			if (QFileInfo::exists(base + suffix)) {
				mapPath = base + suffix;
				break;
			}
	}
	if (!mapPath.isEmpty()) {
		QFile file(mapPath);
		DossierFunctionMap::Map given;
		QString error;
		if (!file.open(QIODevice::ReadOnly)) {
			result.error = Tr::tr("The function map '%1' cannot be read.")
							   .arg(mapPath);
			result.warnings = c.warnings;
			return result;
		}
		if (!DossierFunctionMap::parse(file.readAll(), c.count(), &given,
									   &error)) {
			result.error = Tr::tr("The function map '%1' is not valid: %2")
							   .arg(mapPath, error);
			result.warnings = c.warnings;
			return result;
		}
		for (auto it = given.begin(); it != given.end(); ++it)
			map.insert(it.key(), it.value());
	}
	for (auto it = map.begin(); it != map.end(); ++it) {
		QList<int> kept;
		for (int n : it.value())
			if (c.exported.contains(n))
				kept << n;
		it.value() = kept;
	}
	writeFile(QDir(dir).filePath(QStringLiteral("function-map.json")),
			  DossierFunctionMap::toJson(map, functions));

	QJsonArray functionsJson;
	for (const DossierFunctionMap::Function &f : std::as_const(functions)) {
		QJsonArray folios;
		for (int n : map.value(DossierFunctionMap::canonicalId(f.id)))
			folios.append(n);
		functionsJson.append(QJsonObject{
			{"id", f.id}, {"anchor", f.anchor}, {"tag", f.tag},
			{"title", f.title}, {"folios", folios}});
		c.addSearch(Tr::tr("Function"), f.id + QLatin1Char(' ') + f.tag
						+ QLatin1Char(' ') + f.title,
					QStringLiteral("index.html#") + f.anchor);
	}
	QJsonArray foliosJson;
	for (int n : std::as_const(wanted))
		foliosJson.append(QJsonObject{
			{"n", n}, {"title", c.folios.at(n - 1)->title()}});

	if (options.pdf) {
		progress(Tr::tr("Writing the PDF"));
		const int status = CLIExport::exportPdfFile(
			project, QDir(dir).filePath(QStringLiteral("dossier.pdf")), true,
			options.showTerminals);
		c.hasPdf = status == 0;
		if (!c.hasPdf)
			c.warnings << Tr::tr("The PDF could not be written.");
	}
	++done;

	// Home page
	QString html = pageHead(c, Tr::tr("Technical dossier"),
							QStringLiteral("home"), QStringLiteral("index.html"));
	html += QStringLiteral("<main>\n<h1>%1</h1>\n")
				.arg(esc(Tr::tr("Technical dossier")));
	html += QStringLiteral("<div class=\"home\">\n");
	if (!overviewName.isEmpty())
		html += QStringLiteral(
			"<div class=\"overview\"><iframe id=\"overview\" "
			"src=\"%1#%2\" data-src=\"%1\" title=\"%3\"></iframe></div>\n")
			.arg(overviewName, esc(overviewAnchor),
				 esc(Tr::tr("Overview of the machine")));
	if (!functions.isEmpty())
		html += QStringLiteral(
			"<aside><h2>%1</h2><ul class=\"fnlist\" id=\"fnlist\"></ul>"
			"</aside>\n").arg(esc(Tr::tr("Functions and folios")));
	html += QStringLiteral("</div>\n<h2>%1</h2>\n"
		"<div class=\"bar\"><input id=\"tocfilter\" type=\"search\" "
		"placeholder=\"%2\"></div>\n<div class=\"toc\" id=\"toc\"></div>\n"
		"</main>\n")
		.arg(esc(Tr::tr("Folios")), esc(Tr::tr("Filter...")));
	html += scripts(QStringLiteral("data/index.js"));
	bool ok = writeFile(QDir(dir).filePath(QStringLiteral("index.html")), html);
	ok = ok && writeFile(QDir(dir).filePath(QStringLiteral("data/index.js")),
		jsonScript(QStringLiteral("DOSSIER_INDEX"),
				   QJsonObject{{"functions", functionsJson},
							   {"folios", foliosJson}}));

	// Search page
	QString searchHtml = pageHead(c, Tr::tr("Search"), QStringLiteral("search"),
								  QStringLiteral("search.html"));
	searchHtml += QStringLiteral(
		"<main>\n<h1>%1</h1>\n<div class=\"bar\"><input id=\"filter\" "
		"type=\"search\" placeholder=\"%2\" autofocus> "
		"<span id=\"count\" class=\"muted\"></span></div>\n"
		"<div id=\"results\"></div>\n</main>\n")
		.arg(esc(Tr::tr("Search")),
			 esc(Tr::tr("Labels, texts, references, wires, addresses...")));
	searchHtml += scripts(QStringLiteral("data/search-index.js"));
	QJsonArray searchJson;
	for (const QJsonArray &entry : std::as_const(c.search))
		searchJson.append(entry);
	ok = ok && writeFile(QDir(dir).filePath(QStringLiteral("search.html")),
						 searchHtml)
		&& writeFile(QDir(dir).filePath(QStringLiteral("data/search-index.js")),
					 jsonScript(QStringLiteral("DOSSIER_SEARCH"), searchJson));

	ok = ok && writeFile(QDir(dir).filePath(QStringLiteral("data/common.js")),
						 commonScript(c))
		&& writeFile(QDir(dir).filePath(QStringLiteral("dossier.css")),
					 resource(QStringLiteral("dossier.css")))
		&& writeFile(QDir(dir).filePath(QStringLiteral("dossier.js")),
					 resource(QStringLiteral("dossier.js")));
	if (!ok) {
		result.error = Tr::tr("A file cannot be written in '%1'.").arg(dir);
		result.warnings = c.warnings;
		return result;
	}

	result.ok = true;
	result.folios = wanted.size();
	result.milliseconds = timer.elapsed();
	result.warnings = c.warnings;
	return result;
}

} // namespace HtmlDossier
