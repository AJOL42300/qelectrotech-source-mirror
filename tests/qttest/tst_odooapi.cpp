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
#include "odoo/odooclient.h"
#include "odoo/odoosecretstore.h"
#include "odoo/odoosync.h"

#include <QJsonDocument>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>

namespace {

const QString test_key = QStringLiteral("SECRET-KEY-0123456789");
const QString test_login = QStringLiteral("user@example.com");

bool isFalse(const QJsonValue &value)
{
	return value.isNull() || value.isUndefined()
			|| (value.isBool() && !value.toBool())
			|| (value.isString() && value.toString().isEmpty());
}

bool same(const QJsonValue &a, const QJsonValue &b)
{
	if (b.isBool() && !b.toBool()) {
		return isFalse(a);
	}
	if (a.isDouble() || b.isDouble()) {
		return a.toDouble() == b.toDouble();
	}
	return a.toString() == b.toString();
}

QJsonValue many2one(int id, const QString &name)
{
	QJsonArray pair;
	pair.append(id);
	pair.append(name);
	return pair;
}

}

/**
	A small Odoo that lives on the local machine: it answers the JSON-RPC
	and JSON-2 requests the client makes, from a few records in memory,
	and notes every call. No real Odoo is ever contacted by the tests.
*/
class FakeOdoo : public QTcpServer
{
	public:
		explicit FakeOdoo(QObject *parent = nullptr) : QTcpServer(parent)
		{
			listen(QHostAddress::LocalHost, 0);
		}

		QString url() const
		{
			return QStringLiteral("http://127.0.0.1:%1").arg(serverPort());
		}

		void addProduct(const QString &code, const QString &name,
						double cost = 0.0, int supplier_info = 0)
		{
			const int id = m_next_product++;
			QJsonObject p;
			p.insert("id", id);
			p.insert("default_code", code);
			p.insert("name", name);
			p.insert("standard_price", cost);
			p.insert("description_purchase", false);
			p.insert("qty_available", 4.0);
			p.insert("product_tmpl_id", many2one(id + 100, name));
			QJsonArray sellers;
			if (supplier_info) {
				sellers.append(supplier_info);
			}
			p.insert("seller_ids", sellers);
			products.insert(id, p);
		}

		int addBom(const QString &code, const QString &product_code)
		{
			const int id = m_next_bom++;
			QJsonObject b;
			b.insert("id", id);
			b.insert("code", code);
			b.insert("product_tmpl_id", many2one(productId(product_code) + 100, product_code));
			boms.insert(id, b);
			return id;
		}

		void addLine(int bom, const QString &product_code, double qty)
		{
			QJsonObject l;
			const int id = m_next_line++;
			l.insert("id", id);
			l.insert("bom_id", bom);
			l.insert("product_id", productId(product_code));
			l.insert("product_qty", qty);
			lines.insert(id, l);
		}

		int productId(const QString &code) const
		{
			for (const QJsonObject &p : products) {
				if (p.value("default_code").toString() == code) {
					return p.value("id").toInt();
				}
			}
			return 0;
		}

		QJsonObject product(const QString &code) const
		{
			return products.value(productId(code));
		}

		int writes() const
		{
			int total = 0;
			for (const QString &call : calls) {
				total += call.endsWith(".create") || call.endsWith(".write");
			}
			return total;
		}

		bool mrp = true;
		bool jsonrpc = true;
		bool json2 = true;
		QMap<int, QJsonObject> products;
		QMap<int, QJsonObject> boms;
		QMap<int, QJsonObject> lines;
		QMap<int, QJsonObject> suppliers;
		QStringList calls;
		QStringList requests;

	protected:
		void incomingConnection(qintptr handle) override
		{
			QTcpSocket *socket = new QTcpSocket(this);
			socket->setSocketDescriptor(handle);
			connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {read(socket);});
			connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
		}

	private:
		int m_next_product = 1;
		int m_next_bom = 1;
		int m_next_line = 1;
		QHash<QTcpSocket *, QByteArray> m_buffers;

		void read(QTcpSocket *socket)
		{
			QByteArray &buffer = m_buffers[socket];
			buffer += socket->readAll();
			const int end = buffer.indexOf("\r\n\r\n");
			if (end < 0) {
				return;
			}
			const QList<QByteArray> head = buffer.left(end).split('\n');
			QMap<QString, QString> headers;
			int length = 0;
			for (int i = 1; i < head.size(); ++i)
			{
				const int colon = head.at(i).indexOf(':');
				if (colon < 0) {
					continue;
				}
				const QString name = QString::fromLatin1(head.at(i).left(colon)).trimmed().toLower();
				headers.insert(name, QString::fromLatin1(head.at(i).mid(colon + 1)).trimmed());
				if (name == "content-length") {
					length = headers.value(name).toInt();
				}
			}
			if (buffer.size() < end + 4 + length) {
				return;
			}
			const QByteArray body = buffer.mid(end + 4, length);
			const QString path = QString::fromLatin1(head.first()).split(' ').value(1);
			m_buffers.remove(socket);
			requests << path;

			int status = 200;
			const QByteArray answer = path == "/jsonrpc"
					? handleJsonRpc(body, &status) : handleJson2(path, headers, body, &status);
			const QByteArray reply = "HTTP/1.1 " + QByteArray::number(status)
					+ " X\r\nContent-Type: application/json\r\nContent-Length: "
					+ QByteArray::number(answer.size()) + "\r\nConnection: close\r\n\r\n" + answer;
			socket->write(reply);
			socket->disconnectFromHost();
		}

		static QByteArray bare(const QJsonValue &value)
		{
			QJsonArray array;
			array.append(value);
			const QByteArray json = QJsonDocument(array).toJson(QJsonDocument::Compact);
			return json.mid(1, json.size() - 2);
		}

		QByteArray handleJson2(const QString &path, const QMap<QString, QString> &headers,
							   const QByteArray &body, int *status)
		{
			const QString prefix = QStringLiteral("/json/2/");
			if (!json2 || !path.startsWith(prefix))
			{
				*status = 404;
				return "{}";
			}
			if (headers.value("authorization") != "bearer " + test_key)
			{
				*status = 401;
				return R"({"message":"Invalid apikey"})";
			}
			const QStringList parts = path.mid(prefix.size()).split('/');
			QJsonObject params = QJsonDocument::fromJson(body).object();
			const QJsonArray ids = params.take("ids").toArray();
			QJsonValue out;
			QString error;
			if (!dispatch(parts.value(0), parts.value(1), ids, params, &out, &error))
			{
				*status = 422;
				return QJsonDocument(QJsonObject{{"message", error}}).toJson(QJsonDocument::Compact);
			}
			return bare(out);
		}

		QByteArray handleJsonRpc(const QByteArray &body, int *status)
		{
			if (!jsonrpc)
			{
				*status = 404;
				return "{}";
			}
			const QJsonObject request = QJsonDocument::fromJson(body).object();
			const QJsonObject params = request.value("params").toObject();
			const QString service = params.value("service").toString();
			const QString method = params.value("method").toString();
			const QJsonArray args = params.value("args").toArray();

			QJsonValue out;
			QString error;
			bool ok = true;
			if (service == "common" && method == "version") {
				out = QJsonObject{{"server_version", "18.0"}};
			} else if (service == "common" && method == "authenticate") {
				out = args.at(2).toString() == test_key && args.at(1).toString() == test_login
						? QJsonValue(2) : QJsonValue(false);
			} else if (service == "object" && method == "execute_kw") {
				if (args.at(2).toString() != test_key) {
					ok = false;
					error = "Access Denied";
				} else {
					const QString model = args.at(3).toString();
					const QString call = args.at(4).toString();
					const QJsonArray positional = args.at(5).toArray();
					QJsonObject kwargs = args.at(6).toObject();
					QJsonArray ids;
					if (call == "create") {
						kwargs.insert("vals_list", positional.at(0));
					} else if (call == "write") {
						ids = positional.at(0).toArray();
						kwargs.insert("vals", positional.at(1));
					} else if (!positional.isEmpty()) {
						ids = positional.at(0).toArray();
					}
					ok = dispatch(model, call, ids, kwargs, &out, &error);
				}
			} else {
				ok = false;
				error = "Unknown service";
			}
			QJsonObject response{{"jsonrpc", "2.0"}, {"id", request.value("id")}};
			if (ok) {
				response.insert("result", out);
			} else {
				response.insert("error", QJsonObject{{"code", 200}, {"message", "Odoo Server Error"},
									 {"data", QJsonObject{{"message", error}}}});
			}
			return QJsonDocument(response).toJson(QJsonDocument::Compact);
		}

		QJsonObject fieldsOf(const QStringList &names) const
		{
			QJsonObject fields;
			for (const QString &n : names) {
				fields.insert(n, QJsonObject{{"type", n.endsWith("_id") ? "many2one" : "char"}});
			}
			return fields;
		}

		bool matches(const QJsonObject &record, const QJsonArray &domain) const
		{
			for (const QJsonValue &term : domain)
			{
				const QJsonArray t = term.toArray();
				const QString op = t.at(1).toString();
				const QJsonValue have = record.value(t.at(0).toString());
				const QJsonValue want = t.at(2);
				if (op == "=" && !same(have, want)) {
					return false;
				}
				if (op == "!=" && same(have, want)) {
					return false;
				}
				if (op == "in")
				{
					bool found = false;
					for (const QJsonValue &v : want.toArray()) {
						found |= same(have, v);
					}
					if (!found) {
						return false;
					}
				}
			}
			return true;
		}

		QJsonValue readable(QJsonObject record, const QString &model) const
		{
			if (model == "mrp.bom.line") {
				const int id = record.value("product_id").toInt();
				record.insert("product_id", many2one(id, products.value(id).value("name").toString()));
			}
			return record;
		}

		bool dispatch(const QString &model, const QString &method,
					  const QJsonArray &ids, const QJsonObject &params,
					  QJsonValue *out, QString *error)
		{
			calls << model + "." + method;
			if (method == "unlink") {
				*error = "unlink must never be called";
				return false;
			}
			const bool bom_model = model == "mrp.bom" || model == "mrp.bom.line";
			if (bom_model && !mrp && method != "search_read")
			{
				*error = QStringLiteral("Object %1 doesn't exist").arg(model);
				return false;
			}

			if (method == "fields_get")
			{
				if (model == "product.product") {
					*out = fieldsOf({"default_code", "name", "standard_price", "description_purchase",
									 "seller_ids", "qty_available", "product_tmpl_id"});
				} else if (model == "mrp.bom") {
					*out = fieldsOf({"code", "product_tmpl_id", "product_qty", "type", "bom_line_ids"});
				} else {
					*out = fieldsOf({});
				}
				return true;
			}

			if (method == "search_read")
			{
				const QJsonArray domain = params.value("domain").toArray();
				const int limit = params.value("limit").toInt();
				QList<QJsonObject> pool;
				if (model == "ir.model")
				{
					for (const QString &m : {QStringLiteral("product.product"), QStringLiteral("mrp.bom")})
					{
						if (m == "product.product" || mrp) {
							pool << QJsonObject{{"model", m}};
						}
					}
				}
				else if (model == "res.users") {
					pool << QJsonObject{{"id", 2}, {"login", test_login}};
				} else if (model == "product.product") {
					pool = products.values();
				} else if (model == "mrp.bom") {
					pool = boms.values();
				} else if (model == "mrp.bom.line") {
					pool = lines.values();
				} else if (model == "product.supplierinfo") {
					pool = suppliers.values();
				}
				QJsonArray rows;
				for (const QJsonObject &record : std::as_const(pool))
				{
					if (!matches(record, domain)) {
						continue;
					}
					rows.append(readable(record, model));
					if (limit > 0 && rows.size() >= limit) {
						break;
					}
				}
				*out = rows;
				return true;
			}

			if (method == "create")
			{
				QJsonArray created;
				for (const QJsonValue &vals : params.value("vals_list").toArray()) {
					created.append(create(model, vals.toObject()));
				}
				*out = created;
				return true;
			}

			if (method == "write")
			{
				const QJsonObject vals = params.value("vals").toObject();
				for (const QJsonValue &id : ids)
				{
					QMap<int, QJsonObject> &table = model == "product.product" ? products
							: model == "mrp.bom" ? boms : lines;
					QJsonObject record = table.value(id.toInt());
					for (auto it = vals.constBegin(); it != vals.constEnd(); ++it) {
						record.insert(it.key(), it.value());
					}
					table.insert(id.toInt(), record);
				}
				*out = true;
				return true;
			}
			*error = "Unknown method " + method;
			return false;
		}

		int create(const QString &model, QJsonObject vals)
		{
			if (model == "product.product")
			{
				const int id = m_next_product++;
				vals.insert("id", id);
				vals.insert("product_tmpl_id", many2one(id + 100, vals.value("name").toString()));
				vals.insert("seller_ids", QJsonArray());
				products.insert(id, vals);
				return id;
			}
			if (model == "mrp.bom")
			{
				const int id = m_next_bom++;
				const QJsonArray bom_lines = vals.take("bom_line_ids").toArray();
				vals.insert("id", id);
				boms.insert(id, vals);
				for (const QJsonValue &command : bom_lines) {
					QJsonObject line = command.toArray().at(2).toObject();
					line.insert("bom_id", id);
					create("mrp.bom.line", line);
				}
				return id;
			}
			const int id = m_next_line++;
			vals.insert("id", id);
			lines.insert(id, vals);
			return id;
		}
};

class tst_OdooApi : public QObject
{
	Q_OBJECT

	private:
		static OdooConnection connection(const FakeOdoo &server, OdooProtocol protocol,
										 const QString &key = test_key)
		{
			OdooConnection c;
			c.url = QUrl(server.url());
			c.database = QStringLiteral("testdb");
			c.login = test_login;
			c.api_key = key;
			c.protocol = protocol;
			c.timeout_ms = 5000;
			return c;
		}

		static OdooPart part(const QString &reference, const QString &name,
							 double quantity, double price = 0.0)
		{
			OdooPart p;
			p.reference = reference;
			p.name = name;
			p.manufacturer = QStringLiteral("ACME");
			p.quantity = quantity;
			p.price = price;
			return p;
		}

		static OdooBom bom(const QList<OdooPart> &lines)
		{
			OdooBom b;
			b.code = QStringLiteral("QET-P");
			b.product_reference = QStringLiteral("QET-P");
			b.product_name = QStringLiteral("Plant");
			b.lines = lines;
			return b;
		}

		static void noKeyIn(const QStringList &texts)
		{
			for (const QString &text : texts) {
				QVERIFY2(!text.contains(test_key), qPrintable(text));
			}
		}

	private slots:
		void urlsMustBeHttpsExceptLocally()
		{
			QVERIFY(OdooClient::isUrlAllowed(QUrl("https://mcatech.odoo.com")));
			QVERIFY(OdooClient::isUrlAllowed(QUrl("http://127.0.0.1:8069")));
			QVERIFY(OdooClient::isUrlAllowed(QUrl("http://localhost:8069")));
			QString error;
			QVERIFY(!OdooClient::isUrlAllowed(QUrl("http://mcatech.odoo.com"), &error));
			QVERIFY(!error.isEmpty());
			QVERIFY(!OdooClient::isUrlAllowed(QUrl("ftp://mcatech.odoo.com")));
			QVERIFY(!OdooClient::isUrlAllowed(QUrl("mcatech.odoo.com")));
		}

		void remoteHttpIsNotContacted()
		{
			OdooConnection c;
			c.url = QUrl("http://example.com");
			c.login = test_login;
			c.api_key = test_key;
			OdooClient client(c);
			QString error;
			QVERIFY(!client.connectToServer(&error));
			QVERIFY(!error.isEmpty());
			QVERIFY(client.log().isEmpty());
		}

		void defaultsPointToTheUsersInstance()
		{
			OdooConnection c;
			QCOMPARE(c.url.toString(), QStringLiteral("https://mcatech.odoo.com"));
			QCOMPARE(c.database, QStringLiteral("mcatech"));
			QVERIFY(c.api_key.isEmpty());
		}

		void protocolIsChosenAndTheKeyIsAccepted_data()
		{
			QTest::addColumn<int>("requested");
			QTest::addColumn<bool>("json2");
			QTest::addColumn<bool>("jsonrpc");
			QTest::addColumn<int>("expected");
			QTest::newRow("auto, both") << int(OdooProtocol::Auto) << true << true << int(OdooProtocol::Json2);
			QTest::newRow("auto, old Odoo") << int(OdooProtocol::Auto) << false << true << int(OdooProtocol::JsonRpc);
			QTest::newRow("forced json-rpc") << int(OdooProtocol::JsonRpc) << true << true << int(OdooProtocol::JsonRpc);
			QTest::newRow("forced json-2") << int(OdooProtocol::Json2) << true << false << int(OdooProtocol::Json2);
		}

		void protocolIsChosenAndTheKeyIsAccepted()
		{
			QFETCH(int, requested);
			QFETCH(bool, json2);
			QFETCH(bool, jsonrpc);
			QFETCH(int, expected);
			FakeOdoo server;
			server.json2 = json2;
			server.jsonrpc = jsonrpc;
			OdooClient client(connection(server, OdooProtocol(requested)));
			QString error;
			QVERIFY2(client.connectToServer(&error), qPrintable(error));
			QCOMPARE(int(client.protocol()), expected);
			if (expected == int(OdooProtocol::JsonRpc)) {
				QCOMPARE(client.serverVersion(), QStringLiteral("18.0"));
			}
			noKeyIn(client.log());
		}

		void forcedJson2AgainstAnOldOdooFails()
		{
			FakeOdoo server;
			server.json2 = false;
			OdooClient client(connection(server, OdooProtocol::Json2));
			QString error;
			QVERIFY(!client.connectToServer(&error));
			QVERIFY(!error.isEmpty());
		}

		void wrongKeyIsRefusedAndNeverEchoed_data()
		{
			QTest::addColumn<int>("protocol");
			QTest::newRow("json-rpc") << int(OdooProtocol::JsonRpc);
			QTest::newRow("json-2") << int(OdooProtocol::Json2);
			QTest::newRow("auto") << int(OdooProtocol::Auto);
		}

		void wrongKeyIsRefusedAndNeverEchoed()
		{
			QFETCH(int, protocol);
			FakeOdoo server;
			const QString wrong = QStringLiteral("WRONG-KEY-987654");
			OdooClient client(connection(server, OdooProtocol(protocol), wrong));
			QString error;
			QVERIFY(!client.connectToServer(&error));
			QVERIFY(!error.isEmpty());
			QVERIFY(!client.isConnected());
			QVERIFY(!error.contains(wrong));
			for (const QString &line : client.log()) {
				QVERIFY(!line.contains(wrong));
			}
		}

		void redactHidesTheKey()
		{
			FakeOdoo server;
			OdooClient client(connection(server, OdooProtocol::Auto));
			const QString text = client.redact(QStringLiteral("failed with %1 twice %1").arg(test_key));
			QVERIFY(!text.contains(test_key));
			QVERIFY(text.contains("***"));
		}

		void settingsNeverHoldTheKey()
		{
			QTemporaryDir dir;
			QVERIFY(dir.isValid());
			QSettings::setDefaultFormat(QSettings::IniFormat);
			QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
			QCoreApplication::setOrganizationName("qet-odoo-test");
			QCoreApplication::setApplicationName("qet-odoo-test");

			OdooConnection c;
			c.login = test_login;
			c.api_key = test_key;
			c.saveSettings();
			QSettings().sync();

			OdooConnection read;
			read.loadSettings();
			QCOMPARE(read.login, test_login);
			QCOMPARE(read.database, QStringLiteral("mcatech"));
			QVERIFY(read.api_key.isEmpty());

			QDirIterator it(dir.path(), QDir::Files, QDirIterator::Subdirectories);
			bool any = false;
			while (it.hasNext())
			{
				QFile file(it.next());
				QVERIFY(file.open(QIODevice::ReadOnly));
				QVERIFY(!file.readAll().contains(test_key.toUtf8()));
				any = true;
			}
			QVERIFY(any);
		}

		void secretStoreKeepsTheKeyOutOfBandOnly()
		{
			OdooMemorySecretStore store;
			const QString account = OdooSecretStore::accountFor(QUrl("https://mcatech.odoo.com"), test_login);
			QVERIFY(!account.contains(test_key));
			QVERIFY(account.contains("mcatech.odoo.com"));
			QVERIFY(account.contains(test_login));
			QVERIFY(store.store(account, test_key));
			QCOMPARE(store.read(account), test_key);
			QVERIFY(store.read("other").isEmpty());
			QVERIFY(store.remove(account));
			QVERIFY(store.read(account).isEmpty());
			QVERIFY(!store.isPersistent());
			QVERIFY(OdooSecretStore::system() != nullptr);
		}

		void pushIsPlannedThenSimulatedThenApplied_data()
		{
			QTest::addColumn<int>("protocol");
			QTest::newRow("json-rpc") << int(OdooProtocol::JsonRpc);
			QTest::newRow("json-2") << int(OdooProtocol::Json2);
		}

		void pushIsPlannedThenSimulatedThenApplied()
		{
			QFETCH(int, protocol);
			FakeOdoo server;
			server.addProduct("A9", "Contactor", 0.0);
			OdooClient client(connection(server, OdooProtocol(protocol)));
			QString error;
			QVERIFY2(client.connectToServer(&error), qPrintable(error));
			OdooSync sync(&client, OdooMapping::defaults());

			const QList<OdooPart> parts{part("A9", "Contactor", 3, 12.5), part("B1", "Breaker", 2)};
			OdooPushPlan plan;
			QVERIFY2(sync.planPush(parts, {bom(parts)}, OdooPushOptions(), &plan, &error), qPrintable(error));
			QVERIFY(plan.boms_supported);
			QCOMPARE(plan.products.size(), 3);
			QCOMPARE(plan.count(OdooAction::Create), 3);
			QCOMPARE(plan.count(OdooAction::Update), 1);
			QCOMPARE(plan.writeCalls(), 4);
			QVERIFY(!plan.describe().isEmpty());
			QCOMPARE(server.writes(), 0);

				//The simulation does not even call the client
			const int calls_before = server.calls.size();
			const OdooPushResult simulated = sync.applyPush(plan, true);
			QVERIFY(simulated.ok);
			QVERIFY(simulated.simulated);
			QCOMPARE(simulated.created, 3);
			QCOMPARE(server.calls.size(), calls_before);
			QCOMPARE(client.writeCount(), 0);
			QCOMPARE(server.writes(), 0);
			QCOMPARE(server.products.size(), 1);
			QVERIFY(server.boms.isEmpty());

			const OdooPushResult applied = sync.applyPush(plan, false);
			QVERIFY2(applied.ok, qPrintable(applied.messages.join('\n')));
			QVERIFY(!applied.simulated);
			QCOMPARE(client.writeCount(), 4);
			QCOMPARE(server.writes(), 4);
			QCOMPARE(server.products.size(), 3);
			QCOMPARE(server.product("A9").value("standard_price").toDouble(), 12.5);
			QCOMPARE(server.product("A9").value("description_purchase").toString(), QStringLiteral("ACME"));
			QCOMPARE(server.product("A9").value("name").toString(), QStringLiteral("Contactor"));
			QVERIFY(server.productId("B1") > 0);
			QVERIFY(server.productId("QET-P") > 0);
			QCOMPARE(server.boms.size(), 1);
			QCOMPARE(server.boms.first().value("code").toString(), QStringLiteral("QET-P"));
			QCOMPARE(server.lines.size(), 2);
			double total = 0;
			for (const QJsonObject &line : server.lines) {
				total += line.value("product_qty").toDouble();
			}
			QCOMPARE(total, 5.0);

			QVERIFY(server.calls.filter("unlink").isEmpty());
			noKeyIn(client.log());

				//The same push again has nothing left to do
			OdooPushPlan again;
			QVERIFY2(sync.planPush(parts, {bom(parts)}, OdooPushOptions(), &again, &error), qPrintable(error));
			QCOMPARE(again.writeCalls(), 0);
		}

		void withoutManufacturingOnlyProductsAreExchanged()
		{
			FakeOdoo server;
			server.mrp = false;
			OdooClient client(connection(server, OdooProtocol::JsonRpc));
			QString error;
			QVERIFY2(client.connectToServer(&error), qPrintable(error));
			OdooSync sync(&client, OdooMapping::defaults());

			const QList<OdooPart> parts{part("A9", "Contactor", 3)};
			OdooPushPlan plan;
			QVERIFY2(sync.planPush(parts, {bom(parts)}, OdooPushOptions(), &plan, &error), qPrintable(error));
			QVERIFY(!plan.boms_supported);
			QVERIFY(plan.boms.isEmpty());
			QVERIFY(!plan.warnings.isEmpty());
			QVERIFY(sync.applyPush(plan, false).ok);
			QVERIFY(server.productId("A9") > 0);
			QVERIFY(server.boms.isEmpty());
			for (const QString &call : server.calls) {
				QVERIFY2(!call.startsWith("mrp.bom"), qPrintable(call));
			}
		}

		void anExistingBomIsCompletedAndNothingIsRemoved()
		{
			FakeOdoo server;
			server.addProduct("QET-P", "Plant");
			server.addProduct("A9", "Contactor", 5.0);
			server.addProduct("C9", "Only in Odoo", 1.0);
			const int bom_id = server.addBom("QET-P", "QET-P");
			server.addLine(bom_id, "A9", 1.0);
			server.addLine(bom_id, "C9", 7.0);
			OdooClient client(connection(server, OdooProtocol::Json2));
			QString error;
			QVERIFY2(client.connectToServer(&error), qPrintable(error));
			OdooSync sync(&client, OdooMapping::defaults());

			const QList<OdooPart> parts{part("A9", "Contactor", 3), part("B1", "Breaker", 2)};
			OdooPushOptions options;
			options.create_only = true;
			OdooPushPlan plan;
			QVERIFY2(sync.planPush(parts, {bom(parts)}, options, &plan, &error), qPrintable(error));
			QCOMPARE(plan.boms.size(), 1);
			QCOMPARE(plan.boms.first().action, OdooAction::Update);
			QCOMPARE(plan.boms.first().extra_lines, QStringList{"C9"});
				//create_only: the product A9 is left alone
			QCOMPARE(plan.count(OdooAction::Update), 2);
			QCOMPARE(plan.products.first().action, OdooAction::Unchanged);

			QVERIFY2(sync.applyPush(plan, false).ok, "apply");
			QCOMPARE(server.lines.size(), 3);
			double a9 = 0, c9 = 0, b1 = 0;
			for (const QJsonObject &line : server.lines)
			{
				const int product = line.value("product_id").toInt();
				const double qty = line.value("product_qty").toDouble();
				if (product == server.productId("A9")) {
					a9 = qty;
				} else if (product == server.productId("C9")) {
					c9 = qty;
				} else if (product == server.productId("B1")) {
					b1 = qty;
				}
			}
			QCOMPARE(a9, 3.0);
			QCOMPARE(b1, 2.0);
			QCOMPARE(c9, 7.0);
			QCOMPARE(server.product("A9").value("standard_price").toDouble(), 5.0);
			QVERIFY(server.calls.filter("unlink").isEmpty());
		}

		void aFailingWriteStopsAndIsReported()
		{
			FakeOdoo server;
			OdooClient client(connection(server, OdooProtocol::JsonRpc));
			QString error;
			QVERIFY2(client.connectToServer(&error), qPrintable(error));
			OdooSync sync(&client, OdooMapping::defaults());
			const QList<OdooPart> parts{part("A9", "Contactor", 3)};
			OdooPushPlan plan;
			QVERIFY2(sync.planPush(parts, {bom(parts)}, OdooPushOptions(), &plan, &error), qPrintable(error));
				//Manufacturing is removed between the plan and the apply
			server.mrp = false;
			const OdooPushResult result = sync.applyPush(plan, false);
			QVERIFY(!result.ok);
			QVERIFY(result.messages.join('\n').contains("mrp.bom"));
			noKeyIn(result.messages);
		}

		void productsAreReadBackForTheCatalogue()
		{
			FakeOdoo server;
			server.suppliers.insert(1, QJsonObject{{"id", 1}, {"partner_id", many2one(9, "Rexel")}});
			server.addProduct("A9", "Contactor 3P", 12.5, 1);
			server.addProduct("B1", "Breaker", 4.0);
			OdooClient client(connection(server, OdooProtocol::Auto));
			QString error;
			QVERIFY2(client.connectToServer(&error), qPrintable(error));
			OdooSync sync(&client, OdooMapping::defaults());

			OdooPullResult result;
			QVERIFY2(sync.fetchProducts({"A9", "ZZ9"}, 0, &result, &error), qPrintable(error));
			QCOMPARE(result.records.size(), 1);
			QCOMPARE(result.not_found, QStringList{"ZZ9"});
			const MaterialRecord &record = result.records.first();
			QCOMPARE(record.value("manufacturer_reference"), QStringLiteral("A9"));
			QCOMPARE(record.value("designation"), QStringLiteral("Contactor 3P"));
			QCOMPARE(record.value("unit_price"), QStringLiteral("12.5"));
			QCOMPARE(record.value("stock"), QStringLiteral("4"));
			QCOMPARE(record.value("supplier"), QStringLiteral("Rexel"));

			OdooPullResult all;
			QVERIFY2(sync.fetchProducts({}, 0, &all, &error), qPrintable(error));
			QCOMPARE(all.records.size(), 2);
			QCOMPARE(client.writeCount(), 0);
			QCOMPARE(server.writes(), 0);
		}
};

QTEST_GUILESS_MAIN(tst_OdooApi)
#include "tst_odooapi.moc"
