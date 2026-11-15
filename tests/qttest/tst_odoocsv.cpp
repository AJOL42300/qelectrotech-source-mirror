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
#include "odoo/odoobom.h"
#include "odoo/odoocatalog.h"
#include "odoo/odoocsv.h"
#include "odoo/odoomapping.h"

#include <QDomDocument>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtTest>

/**
	Exchange with Odoo, the parts that need no network: the mapping file,
	the CSV files in both directions, the grouping of the parts of a project
	into bills of materials and the merge into the catalogue.
*/
class tst_OdooCsv : public QObject
{
	Q_OBJECT

	private:
		QSqlDatabase m_db;

		void add(const QString &label, const QString &location,
				 const QString &reference, const QString &designation,
				 const QString &quantity = QString(),
				 const QString &aux_reference = QString(),
				 const QString &price = QString())
		{
			QSqlQuery q(m_db);
			q.prepare(QStringLiteral(
				"INSERT INTO element_nomenclature_view (label, location, "
				"manufacturer, supplier, manufacturer_reference, designation, "
				"quantity, unit_price, manufacturer_reference_auxiliary1) "
				"VALUES (?,?,?,?,?,?,?,?,?)"));
			for (const QString &v : {label, location, QStringLiteral("ACME"),
									 QStringLiteral("Rexel"), reference,
									 designation, quantity, price, aux_reference}) {
				q.addBindValue(v);
			}
			QVERIFY(q.exec());
		}

		QList<OdooPart> parts()
		{
			QSqlQuery q(m_db);
			if (!q.exec(OdooBomBuilder::partsQuery(true))) {
				qWarning() << q.lastError().text();
			}
			return OdooBomBuilder::readParts(q);
		}

	private slots:
		void init()
		{
			m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("odoocsv"));
			m_db.setDatabaseName(QStringLiteral(":memory:"));
			QVERIFY(m_db.open());
			QSqlQuery q(m_db);
			QVERIFY(q.exec(QStringLiteral(
				"CREATE TABLE element_nomenclature_view (label, location, "
				"manufacturer, supplier, manufacturer_reference, designation, "
				"description, quantity, unity, unit_price, "
				"manufacturer_auxiliary1, supplier_auxiliary1, manufacturer_reference_auxiliary1, "
				"designation_auxiliary1, description_auxiliary1, quantity_auxiliary1, unity_auxiliary1, "
				"manufacturer_auxiliary2, supplier_auxiliary2, manufacturer_reference_auxiliary2, "
				"designation_auxiliary2, description_auxiliary2, quantity_auxiliary2, unity_auxiliary2, "
				"manufacturer_auxiliary3, supplier_auxiliary3, manufacturer_reference_auxiliary3, "
				"designation_auxiliary3, description_auxiliary3, quantity_auxiliary3, unity_auxiliary3, "
				"manufacturer_auxiliary4, supplier_auxiliary4, manufacturer_reference_auxiliary4, "
				"designation_auxiliary4, description_auxiliary4, quantity_auxiliary4, unity_auxiliary4)")));
		}

		void cleanup()
		{
			m_db.close();
			m_db = QSqlDatabase();
			QSqlDatabase::removeDatabase(QStringLiteral("odoocsv"));
		}

		void mappingRoundTrip()
		{
			const OdooMapping defaults = OdooMapping::defaults();
			OdooMapping read;
			QString error;
			QVERIFY2(OdooMapping::fromJson(defaults.toJson(), &read, &error), qPrintable(error));
			QCOMPARE(read.product_columns.size(), defaults.product_columns.size());
			QCOMPARE(read.product_columns.at(0).column, QStringLiteral("default_code"));
			QCOMPARE(read.separator, defaults.separator);
			QCOMPARE(read.import_columns.value(QStringLiteral("stock")),
					 defaults.import_columns.value(QStringLiteral("stock")));
			QCOMPARE(read.api_product_fields.size(), defaults.api_product_fields.size());
		}

		void mappingFileOnlyListsWhatItChanges()
		{
			OdooMapping m;
			QString error;
			QVERIFY2(OdooMapping::fromJson(
						 R"({"csv":{"separator":";","utf8_bom":true,
							 "products":[{"column":"ref","value":"{reference}"},
										 {"column":"x_cabinet","value":"{cabinet}"}]}})",
						 &m, &error), qPrintable(error));
			QCOMPARE(m.separator, QLatin1Char(';'));
			QVERIFY(m.utf8_bom);
			QCOMPARE(m.product_columns.size(), 2);
			QCOMPARE(m.bom_line_columns.size(), OdooMapping::defaults().bom_line_columns.size());

			OdooMapping untouched = OdooMapping::defaults();
			QVERIFY(!OdooMapping::fromJson("{not json", &untouched, &error));
			QVERIFY(!error.isEmpty());
			QVERIFY(!OdooMapping::fromJson(R"({"csv":{"products":[{"value":"x"}]}})", &untouched, &error));
			QCOMPARE(untouched.product_columns.size(), OdooMapping::defaults().product_columns.size());
		}

		void partsAreGroupedByReference()
		{
			add("K1", "", "A9", "Contactor", QString(), QString(), "12,5");
			add("K2", "", "A9", "Contactor");
			add("F1", "", "B1", "Breaker", "2");
				//An auxiliary block counts as one more part
			add("K3", "", "A9", "Contactor", QString(), "AUX1");
			add("X1", "", "", "No reference at all");
			const QList<OdooPart> list = parts();
			QCOMPARE(list.size(), 3);
			QMap<QString, OdooPart> by_reference;
			for (const OdooPart &p : list) {
				by_reference.insert(p.reference, p);
			}
			QCOMPARE(by_reference.value("A9").quantity, 3.0);
			QCOMPARE(by_reference.value("A9").price, 12.5);
			QCOMPARE(by_reference.value("A9").labels, (QStringList{"K1", "K2", "K3"}));
			QCOMPARE(by_reference.value("B1").quantity, 2.0);
			QCOMPARE(by_reference.value("AUX1").quantity, 1.0);

			QSqlQuery q(m_db);
			QVERIFY(q.exec(OdooBomBuilder::unreferencedQuery(true)));
			QCOMPARE(OdooBomBuilder::readCount(q), 1);
		}

		void oneBomForTheProjectOrOnePerCabinet()
		{
			add("K1", "Cab1", "A9", "Contactor");
			add("K2", "Cab2", "A9", "Contactor");
			add("F1", "Cab2", "B1", "Breaker");
			add("Q1", "", "C1", "Free standing");
			OdooBomOptions options;
			options.project_name = "My Plant 22";

			QList<OdooBom> boms = OdooBomBuilder::buildBoms(parts(), options);
			QCOMPARE(boms.size(), 1);
			QCOMPARE(boms.first().code, QStringLiteral("QET-My-Plant-22"));
			QCOMPARE(boms.first().lines.size(), 3);

			options.per_cabinet = true;
			boms = OdooBomBuilder::buildBoms(parts(), options);
			QCOMPARE(boms.size(), 3);
			QStringList codes;
			for (const OdooBom &b : boms) {
				codes << b.code;
			}
			QVERIFY(codes.contains("QET-My-Plant-22-Cab1"));
			QVERIFY(codes.contains("QET-My-Plant-22-Cab2"));
			QVERIFY(codes.contains("QET-My-Plant-22-unassigned"));
			QCOMPARE(OdooBomBuilder::products(boms).size(), 3);
		}

		void cablesAreSummedInMetres()
		{
			QDomDocument doc;
			QVERIFY(doc.setContent(QStringLiteral(
				"<project><conductor network_reference=\"CAT6\" network_length=\"10,5\" network_cable=\"W1\"/>"
				"<conductor network_reference=\"CAT6\" network_length=\"250 cm\" network_cable=\"W2\"/>"
				"<conductor network_reference=\"\" network_length=\"3\"/>"
				"<conductor network_reference=\"EC\" network_length=\"0\"/></project>")));
			const QList<OdooPart> list = OdooBomBuilder::cableParts(doc);
			QCOMPARE(list.size(), 1);
			QCOMPARE(list.first().reference, QStringLiteral("CAT6"));
			QCOMPARE(list.first().quantity, 13.0);
			QCOMPARE(list.first().unit, QStringLiteral("m"));
			QVERIFY(list.first().is_cable);
		}

		void productsCsvIsReadyForOdooImport()
		{
			OdooPart a;
			a.reference = "A9";
			a.name = "Contactor, 3 poles";
			a.manufacturer = "ACME";
			a.supplier = "Rexel";
			a.price = 12.5;
			OdooPart b;
			b.reference = "B1";
			b.name = "Breaker";
			OdooBom bom;
			bom.code = "QET-P";
			bom.product_reference = "QET-P";
			bom.product_name = "Plant";
			bom.lines = {a, b};

			const QString csv = QString::fromUtf8(OdooCsv::productsCsv({a, b}, {bom}, OdooMapping::defaults()));
			const QStringList lines = csv.split('\n', Qt::SkipEmptyParts);
			QCOMPARE(lines.size(), 4);
			QCOMPARE(lines.at(0), QStringLiteral("default_code,name,standard_price,description_purchase,seller_ids/partner_id,seller_ids/product_code"));
			QCOMPARE(lines.at(1), QStringLiteral("A9,\"Contactor, 3 poles\",12.5,ACME,Rexel,A9"));
				//No supplier: the vendor columns stay empty
			QCOMPARE(lines.at(2), QStringLiteral("B1,Breaker,,,,"));
			QVERIFY(lines.at(3).startsWith(QStringLiteral("QET-P,Plant")));
			QVERIFY(!csv.startsWith(QStringLiteral("\uFEFF")));
		}

		void bomCsvHasOneHeaderRowPerBom()
		{
			OdooPart a;
			a.reference = "A9";
			a.quantity = 3;
			OdooPart b;
			b.reference = "CAT6";
			b.quantity = 13.5;
			OdooBom bom;
			bom.code = "QET-P";
			bom.product_reference = "QET-P";
			bom.lines = {a, b};
			const QStringList lines = QString::fromUtf8(OdooCsv::bomCsv({bom}, OdooMapping::defaults()))
					.split('\n', Qt::SkipEmptyParts);
			QCOMPARE(lines.size(), 3);
			QCOMPARE(lines.at(0), QStringLiteral("product_tmpl_id,code,product_qty,type,bom_line_ids/product_id,bom_line_ids/product_qty"));
			QCOMPARE(lines.at(1), QStringLiteral("QET-P,QET-P,1,normal,A9,3"));
			QCOMPARE(lines.at(2), QStringLiteral(",,,,CAT6,13.5"));
		}

		void csvOptionsFollowTheMapping()
		{
			OdooMapping m = OdooMapping::defaults();
			m.separator = ';';
			m.utf8_bom = true;
			OdooPart a;
			a.reference = "A9";
			a.name = "x;y";
			const QByteArray csv = OdooCsv::productsCsv({a}, {}, m);
			QVERIFY(csv.startsWith("\xEF\xBB\xBF"));
			QVERIFY(csv.contains("default_code;name;"));
			QVERIFY(csv.contains("A9;\"x;y\";"));
		}

		void odooExportIsReadIntoCatalogueRecords()
		{
			const QByteArray csv =
					"\xEF\xBB\xBF" "R\xC3\xA9" "f\xC3\xA9rence interne;Nom;Co\xC3\xBBt;Fournisseurs/Fournisseur;Quantit\xC3\xA9 en stock\n"
					"A9;Contacteur;12,5;Rexel;4\n"
					";;;Schneider;\n"
					"B1;Disjoncteur;;;\n"
					"A9;Doublon;1;;\n";
			OdooImportResult result;
			QString error;
			QVERIFY2(OdooCsv::importProducts(csv, OdooMapping::defaults(), &result, &error), qPrintable(error));
			QCOMPARE(result.records.size(), 2);
			QCOMPARE(result.skipped_rows, 1);
			QCOMPARE(result.warnings.size(), 1);
			const MaterialRecord &a9 = result.records.at(0);
			QCOMPARE(a9.value("manufacturer_reference"), QStringLiteral("A9"));
			QCOMPARE(a9.value("designation"), QStringLiteral("Contacteur"));
			QCOMPARE(a9.value("unit_price"), QStringLiteral("12,5"));
			QCOMPARE(a9.value("supplier"), QStringLiteral("Rexel"));
			QCOMPARE(a9.value("stock"), QStringLiteral("4"));
			QVERIFY(!result.records.at(1).values.contains("supplier"));
		}

		void csvWrittenByQetIsReadBack()
		{
			OdooPart a;
			a.reference = "A9";
			a.name = "Contactor, 3 poles";
			a.manufacturer = "ACME";
			a.supplier = "Rexel";
			a.price = 12.5;
			OdooImportResult result;
			QVERIFY(OdooCsv::importProducts(OdooCsv::productsCsv({a}, {}, OdooMapping::defaults()),
											OdooMapping::defaults(), &result));
			QCOMPARE(result.records.size(), 1);
			QCOMPARE(result.records.first().value("designation"), QStringLiteral("Contactor, 3 poles"));
			QCOMPARE(result.records.first().value("unit_price"), QStringLiteral("12.5"));
			QCOMPARE(result.records.first().value("manufacturer"), QStringLiteral("ACME"));
		}

		void importWithoutReferenceColumnFails()
		{
			OdooImportResult result;
			QString error;
			QVERIFY(!OdooCsv::importProducts("Name,Cost\nx,1\n", OdooMapping::defaults(), &result, &error));
			QVERIFY(!error.isEmpty());
			QVERIFY(!OdooCsv::importProducts("", OdooMapping::defaults(), &result, &error));
		}

		void catalogueMergeFillsOrOverwrites()
		{
			MaterialListData data;
			data.columns = MaterialList::defaultColumns();
			MaterialRecord existing;
			existing.setValue("manufacturer_reference", "A9");
			existing.setValue("designation", "My name");
			data.records << existing;

			MaterialRecord incoming;
			incoming.setValue("manufacturer_reference", "a9");
			incoming.setValue("designation", "Odoo name");
			incoming.setValue("unit_price", "12.5");
			incoming.setValue("stock", "4");
			MaterialRecord created;
			created.setValue("manufacturer_reference", "B1");
			created.setValue("designation", "New");

				//A preview changes nothing
			auto changes = OdooCatalog::merge(&data, {incoming, created}, OdooCatalog::Mode::FillEmpty, false);
			QCOMPARE(data.records.size(), 1);
			QCOMPARE(data.records.first().value("unit_price"), QString());
			QCOMPARE(changes.size(), 3);

			changes = OdooCatalog::merge(&data, {incoming, created}, OdooCatalog::Mode::FillEmpty, true);
			QCOMPARE(data.records.size(), 2);
			QCOMPARE(data.records.first().value("designation"), QStringLiteral("My name"));
			QCOMPARE(data.records.first().value("unit_price"), QStringLiteral("12.5"));
			QVERIFY(data.columns.contains("stock"));

			OdooCatalog::merge(&data, {incoming}, OdooCatalog::Mode::Overwrite, true);
			QCOMPARE(data.records.first().value("designation"), QStringLiteral("Odoo name"));
		}

		void catalogueIsBackedUpBeforeBeingWritten()
		{
			QTemporaryDir dir;
			QVERIFY(dir.isValid());
			const QString path = dir.filePath("catalogue.csv");
			QString error;
			QVERIFY(MaterialList::createFile(path, &error));
			MaterialListData data;
			QVERIFY(MaterialList::load(path, &data, &error));
			MaterialRecord record;
			record.setValue("manufacturer_reference", "A9");
			record.setValue("designation", "Contactor");
			OdooCatalog::merge(&data, {record}, OdooCatalog::Mode::FillEmpty, true);
			const qint64 before = QFileInfo(path).size();
			QVERIFY2(OdooCatalog::saveWithBackup(path, data, &error), qPrintable(error));
			QVERIFY(QFileInfo::exists(path + ".bak"));
			QCOMPARE(QFileInfo(path + ".bak").size(), before);
			MaterialListData reread;
			QVERIFY(MaterialList::load(path, &reread, &error));
			QCOMPARE(reread.records.size(), 1);
		}

		void lengthsAndSlugs()
		{
			QCOMPARE(OdooBomBuilder::parseLength("12"), 12.0);
			QCOMPARE(OdooBomBuilder::parseLength("12,5 m"), 12.5);
			QCOMPARE(OdooBomBuilder::parseLength("30 cm"), 0.3);
			QCOMPARE(OdooBomBuilder::parseLength("500 mm"), 0.5);
			QCOMPARE(OdooBomBuilder::parseLength("abc"), 0.0);
			QCOMPARE(OdooBomBuilder::slug(" Cab 1 / left "), QStringLiteral("Cab-1-left"));
		}
};

QTEST_GUILESS_MAIN(tst_OdooCsv)
#include "tst_odoocsv.moc"
