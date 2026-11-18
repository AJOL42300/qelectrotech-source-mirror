// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "bomexport.h"

// BomExport::groupedQuery() on a table shaped like element_nomenclature_view:
// identical parts become one line with the quantities added up.
class tst_groupedbom : public QObject
{
	Q_OBJECT

	QSqlDatabase m_db;
	int m_position = 0;

	void add(const QString &label, const QString &manufacturer, const QString &article,
			 const QString &designation, const QString &quantity = QString(),
			 const QString &aux_article = QString())
	{
		QSqlQuery q(m_db);
		q.prepare(QStringLiteral(
			"INSERT INTO element_nomenclature_view (label, manufacturer, manufacturer_reference, "
			"designation, quantity, manufacturer_reference_auxiliary1, diagram_position, position) "
			"VALUES (?,?,?,?,?,?,1,?)"));
		for (const QString &v : {label, manufacturer, article, designation, quantity, aux_article})
			q.addBindValue(v);
		q.addBindValue(++m_position);
		QVERIFY(q.exec());
	}

		// An element with any of its fields, in the order they are added
	void addRow(const QVariantMap &values)
	{
		QStringList names;
		QStringList marks;
		for (auto it = values.cbegin(); it != values.cend(); ++it) {
			names << it.key();
			marks << QStringLiteral("?");
		}
		names << QStringLiteral("diagram_position") << QStringLiteral("position");
		marks << QStringLiteral("1") << QStringLiteral("?");
		QSqlQuery q(m_db);
		q.prepare(QStringLiteral("INSERT INTO element_nomenclature_view (%1) VALUES (%2)")
				  .arg(names.join(','), marks.join(',')));
		for (auto it = values.cbegin(); it != values.cend(); ++it)
			q.addBindValue(it.value());
		q.addBindValue(++m_position);
		QVERIFY(q.exec());
	}

	QList<QStringList> rowsOf(const QString &sql, QStringList *columns = nullptr)
	{
		QSqlQuery q(m_db);
		if (!q.exec(sql)) {
			qWarning() << q.lastError().text();
			return {};
		}
		if (columns) {
			columns->clear();
			for (int i = 0; i < q.record().count(); ++i)
				columns->append(q.record().fieldName(i));
		}
		QList<QStringList> rows;
		while (q.next()) {
			QStringList row;
			for (int i = 0; i < q.record().count(); ++i)
				row << q.value(i).toString();
			rows << row;
		}
		return rows;
	}

	QList<QStringList> run(const BomExport::GroupedOptions &options, QStringList *columns = nullptr)
	{
		return rowsOf(BomExport::groupedQuery(options), columns);
	}

		// level, label, quantity, total_quantity, part_number, orphan
	QList<QStringList> tree(const BomExport::HierarchicalOptions &options = {})
	{
		QStringList columns;
		const auto rows = rowsOf(BomExport::hierarchicalQuery(options), &columns);
		const qsizetype wanted[] = {columns.indexOf("level"), columns.indexOf("label"),
							  columns.indexOf("quantity"), columns.indexOf("total_quantity"),
							  columns.indexOf("part_number"), columns.indexOf("orphan")};
		QList<QStringList> result;
		for (const auto &row : rows) {
			QStringList line;
			for (qsizetype i : wanted)
				line << (i < 0 ? QString() : row[i]);
			result << line;
		}
		return result;
	}

	private slots:
		void init()
		{
			m_position = 0;
			m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("groupedbom"));
			m_db.setDatabaseName(QStringLiteral(":memory:"));
			QVERIFY(m_db.open());
				// a table is enough: the query only reads these columns
			QSqlQuery q(m_db);
			QVERIFY(q.exec(QStringLiteral(
				"CREATE TABLE element_nomenclature_view (label, manufacturer, manufacturer_reference, "
				"designation, description, quantity, unity, diagram_position, position, "
				"manufacturer_auxiliary1, manufacturer_reference_auxiliary1, designation_auxiliary1, "
				"description_auxiliary1, quantity_auxiliary1, unity_auxiliary1, "
				"manufacturer_auxiliary2, manufacturer_reference_auxiliary2, designation_auxiliary2, "
				"description_auxiliary2, quantity_auxiliary2, unity_auxiliary2, "
				"manufacturer_auxiliary3, manufacturer_reference_auxiliary3, designation_auxiliary3, "
				"description_auxiliary3, quantity_auxiliary3, unity_auxiliary3, "
				"manufacturer_auxiliary4, manufacturer_reference_auxiliary4, designation_auxiliary4, "
				"description_auxiliary4, quantity_auxiliary4, unity_auxiliary4)")));
		}

		void cleanup()
		{
			m_db.close();
			m_db = QSqlDatabase();
			QSqlDatabase::removeDatabase(QStringLiteral("groupedbom"));
		}

		void samePartIsOneLine()
		{
			add("K1", "ABB", "A9", "Contactor");
			add("K2", "ABB", "A9", "Contactor");
			add("F1", "ABB", "B1", "Breaker");
			BomExport::GroupedOptions o;
			QStringList columns;
			const auto rows = run(o, &columns);
			QCOMPARE(columns, (QStringList{"number", "quantity", "name", "part_number",
										   "references", "manufacturer", "unit"}));
			QCOMPARE(rows.size(), 2);
			QCOMPARE(rows[0], (QStringList{"1", "2", "Contactor", "A9", "K1,K2", "ABB", ""}));
			QCOMPARE(rows[1][1], QStringLiteral("1"));
		}

		void strayBlanksDoNotSplitAPart()
		{
			add("K1", "ABB", "A9", "Contactor");
			add("K2", " ABB ", "A9 ", "Contactor");
			BomExport::GroupedOptions o;
			QCOMPARE(run(o).size(), 1);
		}

		void quantityFieldIsAddedUp()
		{
			add("W1", "", "CAB3", "Cable", "2,5");
			add("W2", "", "CAB3", "Cable", "4");
			BomExport::GroupedOptions o;
			QCOMPARE(run(o).first()[1], QStringLiteral("6.5"));
			o.sum_quantity = false;
			QCOMPARE(run(o).first()[1], QStringLiteral("2"));
		}

		void noPartNumberIsNeverMerged()
		{
			add("X1", "", "", "Terminal");
			add("X1", "", "", "Terminal");
			add("X2", "", "", "Terminal");
			BomExport::GroupedOptions o;
			QCOMPARE(run(o).size(), 3);
		}

		void rowsWithNothingToOrderStayOut()
		{
			add("J1", "", "", "");
			add("K1", "ABB", "A9", "Contactor");
			BomExport::GroupedOptions o;
			QCOMPARE(run(o).size(), 1);
		}

		void auxiliaryBlocksAreParts()
		{
			add("QF1", "ABB", "B1", "Breaker", "", "AUX9");
			BomExport::GroupedOptions o;
			QCOMPARE(run(o).size(), 2);
			o.auxiliaries = false;
			QCOMPARE(run(o).size(), 1);
		}

		void columnsFollowTheOptions()
		{
			add("K1", "ABB", "A9", "Contactor");
			BomExport::GroupedOptions o;
			o.references = false;
			o.numbering = false;
			QStringList columns;
			run(o, &columns);
			QCOMPARE(columns, (QStringList{"quantity", "name", "part_number", "manufacturer", "unit"}));
		}

		void zeroQuantityIsNotCounted()
		{
			add("QS1", "SE", "SW220", "Switch", "1");
			add("QS1.1", "SE", "ACC1", "Accessory", "0");
			add("QS1.2", "SE", "ACC1", "Accessory", "0,0");
			add("QS1.3", "SE", "ACC1", "Accessory", "2");
			BomExport::GroupedOptions o;
			o.sum_quantity = false;
			auto rows = run(o);
			QCOMPARE(rows.size(), 2);
			QCOMPARE(rows[0][3], QStringLiteral("ACC1"));
			QCOMPARE(rows[0][1], QStringLiteral("1"));
			o.sum_quantity = true;
			QCOMPARE(run(o)[0][1], QStringLiteral("2"));
			o.skip_zero_quantity = false;
			QCOMPARE(run(o)[0][1], QStringLiteral("2"));
			o.sum_quantity = false;
			QCOMPARE(run(o)[0][1], QStringLiteral("3"));
		}

		void zeroQuantityOfAnAuxiliaryIsNotCounted()
		{
			addRow({{"label", "QS1"}, {"manufacturer_reference", "SW220"},
					{"manufacturer_reference_auxiliary1", "ACC1"},
					{"quantity_auxiliary1", "0"},
					{"manufacturer_reference_auxiliary2", "ACC2"},
					{"quantity_auxiliary2", "3"}});
			BomExport::GroupedOptions o;
			o.sum_quantity = false;
			const auto rows = run(o);
			QCOMPARE(rows.size(), 2);
			QStringList parts;
			for (const auto &row : rows)
				parts << row[3];
			QCOMPARE(parts, (QStringList{"ACC2", "SW220"}));
		}

		void accessoriesFollowTheirParentByPrefix()
		{
			add("-QS1", "SE", "SW220", "Switch");
			add("-QS1.1", "SE", "ACC1", "Accessory");
			add("-QS1-MC", "SE", "ACC2", "Contact");
			add("-QS1_OF", "SE", "ACC3", "Contact");
			add("-K1", "ABB", "A9", "Contactor");
			add("-QS10", "SE", "SW63", "Other switch");
			const auto rows = tree();
			QCOMPARE(rows.size(), 6);
			QCOMPARE(rows[0].mid(0, 2), (QStringList{"0", "-QS1"}));
			QCOMPARE(rows[1].mid(0, 2), (QStringList{"1", "-QS1.1"}));
			QCOMPARE(rows[2].mid(0, 2), (QStringList{"1", "-QS1-MC"}));
			QCOMPARE(rows[3].mid(0, 2), (QStringList{"1", "-QS1_OF"}));
			QCOMPARE(rows[4].mid(0, 2), (QStringList{"0", "-K1"}));
				// -QS10 is no accessory of -QS1
			QCOMPARE(rows[5].mid(0, 2), (QStringList{"0", "-QS10"}));
			for (const auto &row : rows)
				QCOMPARE(row[5], QStringLiteral("0"));
		}

		void longestPrefixIsTheParent()
		{
			add("-QS1", "SE", "SW220", "Switch");
			add("-QS1.1", "SE", "ACC1", "Accessory");
			add("-QS1.1.2", "SE", "ACC2", "Sub accessory");
			const auto rows = tree();
			QCOMPARE(rows.size(), 3);
			QCOMPARE(rows[2].mid(0, 2), (QStringList{"2", "-QS1.1.2"}));
		}

		void accessoryWithoutParentIsAnOrphan()
		{
			add("-QS2.1", "SE", "ACC1", "Accessory");
			add("-K1", "ABB", "A9", "Contactor");
			const auto rows = tree();
			QCOMPARE(rows.size(), 2);
			QCOMPARE(rows[0].mid(0, 2), (QStringList{"0", "-QS2.1"}));
			QCOMPARE(rows[0][5], QStringLiteral("1"));
			QCOMPARE(rows[1][5], QStringLiteral("0"));
		}

		void moreThanFourArticlesOnOneDevice()
		{
			addRow({{"label", "-QS1"}, {"manufacturer_reference", "SW220"},
					{"manufacturer_reference_auxiliary1", "A1"},
					{"manufacturer_reference_auxiliary2", "A2"},
					{"manufacturer_reference_auxiliary3", "A3"},
					{"manufacturer_reference_auxiliary4", "A4"}});
			for (int i = 1; i <= 6; ++i)
				add(QStringLiteral("-QS1.%1").arg(i), "SE", QStringLiteral("B%1").arg(i), "Accessory");
			const auto rows = tree();
				// the device, its four auxiliary blocks and six accessories
			QCOMPARE(rows.size(), 11);
			QCOMPARE(rows[0][4], QStringLiteral("SW220"));
			for (int i = 1; i <= 4; ++i) {
				QCOMPARE(rows[i][0], QStringLiteral("1"));
				QCOMPARE(rows[i][4], QStringLiteral("A%1").arg(i));
			}
			for (int i = 5; i <= 10; ++i)
				QCOMPARE(rows[i][1], QStringLiteral("-QS1.%1").arg(i - 4));
		}

		void totalQuantityMultipliesTheParents()
		{
			add("-QS1", "SE", "SW220", "Switch", "2");
			add("-QS1.1", "SE", "ACC1", "Accessory", "3");
			add("-QS1.2", "SE", "ACC2", "Accessory", "0");
			addRow({{"label", "-QS1"}, {"manufacturer_reference_auxiliary1", "AUX1"},
					{"quantity_auxiliary1", "4"}});
			auto rows = tree();
			QCOMPARE(rows.size(), 4);
			QCOMPARE(rows[0][3], QStringLiteral("2"));
				// an auxiliary block of a device sits right under the device
			QCOMPARE(rows[1][4], QStringLiteral("AUX1"));
			QCOMPARE(rows[1][3], QStringLiteral("4"));
			QCOMPARE(rows[2][3], QStringLiteral("6"));
				// 0 stays on the list, and orders nothing
			QCOMPARE(rows[3][3], QStringLiteral("0"));
			BomExport::HierarchicalOptions o;
			o.cumulative_quantity = false;
			QStringList columns;
			rowsOf(BomExport::hierarchicalQuery(o), &columns);
			QVERIFY(!columns.contains("total_quantity"));
			o.auxiliaries = false;
			QCOMPARE(tree(o).size(), 3);
		}

		void hierarchicalQueryIsRecognised()
		{
			QVERIFY(BomExport::isHierarchicalQuery(BomExport::hierarchicalQuery()));
			QVERIFY(!BomExport::isHierarchicalQuery(BomExport::groupedQuery({})));
			QVERIFY(!BomExport::isHierarchicalQuery(BomExport::defaultQuery()));
		}

		void flatModeIsUnchanged()
		{
			add("-QS1", "SE", "SW220", "Switch");
			add("-QS1.1", "SE", "ACC1", "Accessory");
			BomExport::GroupedOptions o;
			QStringList columns;
			const auto rows = run(o, &columns);
			QCOMPARE(rows.size(), 2);
			QCOMPARE(columns, (QStringList{"number", "quantity", "name", "part_number",
										   "references", "manufacturer", "unit"}));
		}
};

QTEST_GUILESS_MAIN(tst_groupedbom)
#include "tst_groupedbom.moc"
