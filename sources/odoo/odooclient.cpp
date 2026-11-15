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
#include "odooclient.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QTimer>

namespace {

const QString settings_group = QStringLiteral("odoo/");

QString redactIn(const QString &text, const QString &secret)
{
	if (secret.isEmpty()) {
		return text;
	}
	QString result = text;
	result.replace(secret, QStringLiteral("***"));
	return result;
}

/**
	@brief Synchronous http post, used by both backends.
	Redirects are not followed: a redirect would send the key to
	another address.
*/
class OdooHttp
{
	public:
		bool post(const QUrl &url, const QByteArray &body,
				  const QList<QPair<QByteArray, QByteArray>> &headers,
				  int timeout_ms, int *status, QByteArray *reply,
				  QString *error)
		{
			*status = 0;
			reply->clear();

			QNetworkRequest request(url);
			request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
								 QNetworkRequest::ManualRedirectPolicy);
			request.setHeader(QNetworkRequest::ContentTypeHeader,
							  QStringLiteral("application/json"));
			request.setRawHeader("User-Agent", "QElectroTech");
			for (const auto &header : headers) {
				request.setRawHeader(header.first, header.second);
			}

			std::unique_ptr<QNetworkReply> answer(m_manager.post(request, body));
			QEventLoop loop;
			QTimer timer;
			timer.setSingleShot(true);
			QObject::connect(answer.get(), &QNetworkReply::finished, &loop, &QEventLoop::quit);
			QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
			timer.start(timeout_ms);
			loop.exec();

			if (!answer->isFinished())
			{
				answer->abort();
				if (error) {
					*error = QCoreApplication::translate("OdooClient", "The server did not answer in time.");
				}
				return false;
			}
			*status = answer->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
			*reply = answer->readAll();
			if (*status == 0)
			{
				if (error) {
					*error = answer->errorString();
				}
				return false;
			}
			return true;
		}

	private:
		QNetworkAccessManager m_manager;
};

QString jsonErrorText(const QJsonValue &error)
{
	const QJsonObject object = error.toObject();
	const QString data_message = object.value(QStringLiteral("data")).toObject()
			.value(QStringLiteral("message")).toString();
	const QString message = object.value(QStringLiteral("message")).toString();
	if (!data_message.isEmpty()) {
		return data_message;
	}
	return message.isEmpty() ? QStringLiteral("Odoo error") : message;
}

/**
	@brief JSON-RPC of Odoo 17 and 18: /jsonrpc, services common and object.
*/
class JsonRpcBackend : public OdooBackend
{
	public:
		explicit JsonRpcBackend(const OdooConnection &connection)
			: m_connection(connection)
		{
			m_endpoint = connection.url;
			m_endpoint.setPath(trimmedPath(connection.url) + QStringLiteral("/jsonrpc"));
			m_endpoint.setQuery(QString());
		}

		OdooProtocol protocol() const override {return OdooProtocol::JsonRpc;}
		QString serverVersion() const override {return m_version;}

		bool authenticate(QString *error) override
		{
			QJsonValue version;
			if (rpc(QStringLiteral("common"), QStringLiteral("version"), QJsonArray(), &version, nullptr)) {
				m_version = version.toObject().value(QStringLiteral("server_version")).toString();
			}

			QJsonValue uid;
			if (!rpc(QStringLiteral("common"), QStringLiteral("authenticate"),
					 QJsonArray{m_connection.database, m_connection.login,
								m_connection.api_key, QJsonObject()},
					 &uid, error)) {
				return false;
			}
			if (!uid.isDouble() || uid.toInteger() <= 0)
			{
				if (error) {
					*error = QCoreApplication::translate("OdooClient", "Authentication failed: check the database, the user and the API key.");
				}
				return false;
			}
			m_uid = uid.toInteger();
			return true;
		}

		bool call(const QString &model, const QString &method,
				  const QJsonArray &ids, const QJsonObject &params,
				  QJsonValue *result, QString *error) override
		{
			if (method == QLatin1String("unlink"))
			{
				if (error) {
					*error = QCoreApplication::translate("OdooClient", "QElectroTech never deletes anything in Odoo.");
				}
				return false;
			}

			QJsonArray args;
			QJsonObject kwargs = params;
			if (method == QLatin1String("create")) {
				args.append(kwargs.take(QStringLiteral("vals_list")));
			} else if (method == QLatin1String("write")) {
				args.append(ids);
				args.append(kwargs.take(QStringLiteral("vals")));
			} else if (!ids.isEmpty()) {
				args.append(ids);
			}
			return rpc(QStringLiteral("object"), QStringLiteral("execute_kw"),
					   QJsonArray{m_connection.database, m_uid, m_connection.api_key,
								  model, method, args, kwargs},
					   result, error);
		}

	private:
		static QString trimmedPath(const QUrl &url)
		{
			QString path = url.path();
			while (path.endsWith(QLatin1Char('/'))) {
				path.chop(1);
			}
			return path;
		}

		bool rpc(const QString &service, const QString &method, const QJsonArray &args,
				 QJsonValue *result, QString *error)
		{
			QJsonObject params;
			params.insert(QStringLiteral("service"), service);
			params.insert(QStringLiteral("method"), method);
			params.insert(QStringLiteral("args"), args);
			QJsonObject request;
			request.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
			request.insert(QStringLiteral("method"), QStringLiteral("call"));
			request.insert(QStringLiteral("params"), params);
			request.insert(QStringLiteral("id"), ++m_id);

			QByteArray reply;
			if (!m_http.post(m_endpoint, QJsonDocument(request).toJson(QJsonDocument::Compact),
							 {}, m_connection.timeout_ms, &m_last_status, &reply, error)) {
				return false;
			}
			const QJsonDocument doc = QJsonDocument::fromJson(reply);
			if (!doc.isObject())
			{
				if (error) {
					*error = QCoreApplication::translate("OdooClient", "Unexpected answer from the server (HTTP %1).").arg(m_last_status);
				}
				return false;
			}
			const QJsonObject object = doc.object();
			if (object.contains(QStringLiteral("error")))
			{
				if (error) {
					*error = jsonErrorText(object.value(QStringLiteral("error")));
				}
				return false;
			}
			if (result) {
				*result = object.value(QStringLiteral("result"));
			}
			return true;
		}

		OdooConnection m_connection;
		QUrl m_endpoint;
		OdooHttp m_http;
		qint64 m_uid = 0;
		int m_id = 0;
		QString m_version;
};

/**
	@brief JSON-2 of Odoo 19: /json/2/<model>/<method>, bearer API key.
*/
class Json2Backend : public OdooBackend
{
	public:
		explicit Json2Backend(const OdooConnection &connection)
			: m_connection(connection) {}

		OdooProtocol protocol() const override {return OdooProtocol::Json2;}

		bool authenticate(QString *error) override
		{
				//JSON-2 has no login call: the key is the login. A read of
				//the user's own record proves that the key is accepted.
			QJsonValue rows;
			const QJsonObject params{
				{QStringLiteral("domain"), OdooClient::domain(QStringLiteral("login"), QStringLiteral("="), m_connection.login)},
				{QStringLiteral("fields"), QJsonArray{QStringLiteral("id")}},
				{QStringLiteral("limit"), 1}};
			if (!call(QStringLiteral("res.users"), QStringLiteral("search_read"),
					  QJsonArray(), params, &rows, error)) {
				return false;
			}
			if (!rows.isArray() || rows.toArray().isEmpty())
			{
				if (error) {
					*error = QCoreApplication::translate("OdooClient", "The API key does not belong to the user \"%1\".").arg(m_connection.login);
				}
				return false;
			}
			return true;
		}

		bool call(const QString &model, const QString &method,
				  const QJsonArray &ids, const QJsonObject &params,
				  QJsonValue *result, QString *error) override
		{
			if (method == QLatin1String("unlink"))
			{
				if (error) {
					*error = QCoreApplication::translate("OdooClient", "QElectroTech never deletes anything in Odoo.");
				}
				return false;
			}

			QUrl url = m_connection.url;
			QString base = url.path();
			while (base.endsWith(QLatin1Char('/'))) {
				base.chop(1);
			}
			url.setPath(base + QStringLiteral("/json/2/") + model + QLatin1Char('/') + method);
			url.setQuery(QString());

			QJsonObject body = params;
			if (!ids.isEmpty()) {
				body.insert(QStringLiteral("ids"), ids);
			}
			QList<QPair<QByteArray, QByteArray>> headers;
			headers.append({"Authorization", "bearer " + m_connection.api_key.toUtf8()});
			if (!m_connection.database.isEmpty()) {
				headers.append({"X-Odoo-Database", m_connection.database.toUtf8()});
			}

			QByteArray reply;
			if (!m_http.post(url, QJsonDocument(body).toJson(QJsonDocument::Compact),
							 headers, m_connection.timeout_ms, &m_last_status, &reply, error)) {
				return false;
			}
			if (m_last_status < 200 || m_last_status >= 300)
			{
				if (error)
				{
					const QJsonObject object = QJsonDocument::fromJson(reply).object();
					const QString message = object.value(QStringLiteral("message")).toString();
					*error = message.isEmpty()
							? QCoreApplication::translate("OdooClient", "HTTP error %1").arg(m_last_status)
							: message;
				}
				return false;
			}
			if (result)
			{
					//A result may be a bare value: wrap it to parse it.
				const QJsonDocument doc = QJsonDocument::fromJson(QByteArray("[") + reply + "]");
				*result = doc.array().isEmpty() ? QJsonValue() : doc.array().at(0);
			}
			return true;
		}

	private:
		OdooConnection m_connection;
		OdooHttp m_http;
};

}

void OdooConnection::loadSettings()
{
	QSettings settings;
	const QString saved_url = settings.value(settings_group + QStringLiteral("url")).toString();
	if (!saved_url.isEmpty()) {
		url = QUrl(saved_url);
	}
	database = settings.value(settings_group + QStringLiteral("database"), database).toString();
	login = settings.value(settings_group + QStringLiteral("login")).toString();
	const QString saved_protocol = settings.value(settings_group + QStringLiteral("protocol")).toString();
	if (saved_protocol == QLatin1String("json2")) {
		protocol = OdooProtocol::Json2;
	} else if (saved_protocol == QLatin1String("jsonrpc")) {
		protocol = OdooProtocol::JsonRpc;
	} else {
		protocol = OdooProtocol::Auto;
	}
}

/**
	@brief OdooConnection::saveSettings
	Remember the address, the database, the user and the protocol. The API
	key is not among them.
*/
void OdooConnection::saveSettings() const
{
	QSettings settings;
	settings.setValue(settings_group + QStringLiteral("url"), url.toString());
	settings.setValue(settings_group + QStringLiteral("database"), database);
	settings.setValue(settings_group + QStringLiteral("login"), login);
	settings.setValue(settings_group + QStringLiteral("protocol"),
					  protocol == OdooProtocol::Json2 ? QStringLiteral("json2")
					  : protocol == OdooProtocol::JsonRpc ? QStringLiteral("jsonrpc")
					  : QStringLiteral("auto"));
}

QString OdooConnection::protocolName(OdooProtocol protocol)
{
	switch (protocol)
	{
		case OdooProtocol::Json2: return QStringLiteral("JSON-2");
		case OdooProtocol::JsonRpc: return QStringLiteral("JSON-RPC");
		default: return QStringLiteral("Auto");
	}
}

OdooClient::OdooClient(const OdooConnection &connection, QObject *parent) :
	QObject(parent),
	m_connection(connection)
{}

OdooClient::~OdooClient() = default;

/**
	@brief OdooClient::isUrlAllowed
	The key travels in every request: only https is accepted, except on
	the local machine (the test server).
	@param url
	@param error
	@return true when the key may be sent to url
*/
bool OdooClient::isUrlAllowed(const QUrl &url, QString *error)
{
	const QString host = url.host().toLower();
	const bool local = host == QLatin1String("localhost") || host == QLatin1String("127.0.0.1")
			|| host == QLatin1String("::1");
	if (url.isValid() && !host.isEmpty()
			&& (url.scheme() == QLatin1String("https")
				|| (url.scheme() == QLatin1String("http") && local))) {
		return true;
	}
	if (error) {
		*error = QCoreApplication::translate("OdooClient", "The address must be an https:// address.");
	}
	return false;
}

/**
	@return the search domain [[field, op, value]]. Built with append(): a
	braced list holding one QJsonArray would be read as a copy of it.
*/
QJsonArray OdooClient::domain(const QString &field, const QString &op,
							  const QJsonValue &value)
{
	QJsonArray term;
	term.append(field);
	term.append(op);
	term.append(value);
	QJsonArray domain;
	domain.append(term);
	return domain;
}

/**
	@brief OdooClient::redact
	@param text
	@return text with the API key hidden
*/
QString OdooClient::redact(const QString &text) const
{
	return redactIn(text, m_connection.api_key);
}

void OdooClient::addLog(const QString &line)
{
	const QString clean = redact(line);
	m_log.append(clean);
	emit logged(clean);
}

/**
	@brief OdooClient::connectToServer
	Choose the protocol and check the credentials.
	@param error
	@return true when Odoo accepts the user and the key
*/
bool OdooClient::connectToServer(QString *error)
{
	if (error) {
		error->clear();
	}
	m_backend.reset();
	m_fields_cache.clear();

	QString problem;
	if (!isUrlAllowed(m_connection.url, &problem)) {
		if (error) {
			*error = problem;
		}
		return false;
	}
	if (m_connection.login.trimmed().isEmpty() || m_connection.api_key.isEmpty())
	{
		if (error) {
			*error = QCoreApplication::translate("OdooClient", "The user and the API key are needed.");
		}
		return false;
	}

	std::unique_ptr<OdooBackend> backend;
	if (m_connection.protocol != OdooProtocol::JsonRpc)
	{
		backend = std::make_unique<Json2Backend>(m_connection);
		addLog(QStringLiteral("Trying JSON-2 on %1").arg(m_connection.url.host()));
		if (backend->authenticate(&problem))
		{
			m_backend = std::move(backend);
			addLog(QStringLiteral("Connected with JSON-2"));
			return true;
		}
		const bool unknown = backend->lastStatus() == 404;
		addLog(QStringLiteral("JSON-2: %1").arg(problem));
		if (m_connection.protocol == OdooProtocol::Json2 || !unknown)
		{
			if (error) {
				*error = redact(problem);
			}
			return false;
		}
	}

	backend = std::make_unique<JsonRpcBackend>(m_connection);
	addLog(QStringLiteral("Trying JSON-RPC on %1").arg(m_connection.url.host()));
	if (!backend->authenticate(&problem))
	{
		addLog(QStringLiteral("JSON-RPC: %1").arg(problem));
		if (error) {
			*error = redact(problem);
		}
		return false;
	}
	m_backend = std::move(backend);
	addLog(QStringLiteral("Connected with JSON-RPC"));
	return true;
}

OdooProtocol OdooClient::protocol() const
{
	return m_backend ? m_backend->protocol() : m_connection.protocol;
}

QString OdooClient::serverVersion() const
{
	return m_backend ? m_backend->serverVersion() : QString();
}

bool OdooClient::call(const QString &model, const QString &method,
					  const QJsonArray &ids, const QJsonObject &params,
					  QJsonValue *result, QString *error)
{
	if (error) {
		error->clear();
	}
	if (!m_backend)
	{
		if (error) {
			*error = tr("Not connected to Odoo.");
		}
		return false;
	}
	const bool writes = method == QLatin1String("create") || method == QLatin1String("write");
	if (writes) {
		++m_write_count;
	}
	QString problem;
	const bool ok = m_backend->call(model, method, ids, params, result, &problem);
	addLog(QStringLiteral("%1.%2 -> %3").arg(model, method, ok ? QStringLiteral("ok") : redact(problem)));
	if (!ok && error) {
		*error = redact(problem);
	}
	return ok;
}

/**
	@brief OdooClient::searchRead
	@param model for instance product.product
	@param domain Odoo's search domain
	@param fields the fields to read
	@param limit 0 for no limit
	@param rows receives the records
	@param error
	@return true on success
*/
bool OdooClient::searchRead(const QString &model, const QJsonArray &domain,
							const QStringList &fields, int limit,
							QJsonArray *rows, QString *error)
{
	QJsonObject params;
	params.insert(QStringLiteral("domain"), domain);
	params.insert(QStringLiteral("fields"), QJsonArray::fromStringList(fields));
	if (limit > 0) {
		params.insert(QStringLiteral("limit"), limit);
	}
	QJsonValue result;
	if (!call(model, QStringLiteral("search_read"), QJsonArray(), params, &result, error)) {
		return false;
	}
	*rows = result.toArray();
	return true;
}

/**
	@brief OdooClient::create
	Create a record. The ids of every kind of answer (one id or a list)
	are understood.
*/
bool OdooClient::create(const QString &model, const QJsonObject &values,
						qint64 *id, QString *error)
{
	QJsonObject params;
	params.insert(QStringLiteral("vals_list"), QJsonArray{values});
	QJsonValue result;
	if (!call(model, QStringLiteral("create"), QJsonArray(), params, &result, error)) {
		return false;
	}
	const QJsonValue first = result.isArray() ? result.toArray().at(0) : result;
	if (!first.isDouble())
	{
		if (error) {
			*error = tr("Odoo did not return the id of the new record.");
		}
		return false;
	}
	if (id) {
		*id = first.toInteger();
	}
	return true;
}

bool OdooClient::write(const QString &model, const QList<qint64> &ids,
					   const QJsonObject &values, QString *error)
{
	QJsonArray id_array;
	for (qint64 id : ids) {
		id_array.append(id);
	}
	QJsonObject params;
	params.insert(QStringLiteral("vals"), values);
	return call(model, QStringLiteral("write"), id_array, params, nullptr, error);
}

/**
	@param model
	@param error
	@return the fields of a model and their type; empty when it can't be read.
	The answer is kept: Odoo's versions do not have the same fields.
*/
QMap<QString, QString> OdooClient::fieldTypes(const QString &model, QString *error)
{
	const auto cached = m_fields_cache.constFind(model);
	if (cached != m_fields_cache.constEnd()) {
		return cached.value();
	}
	QJsonObject params;
	params.insert(QStringLiteral("attributes"), QJsonArray{QStringLiteral("type")});
	QJsonValue result;
	QMap<QString, QString> types;
	if (!call(model, QStringLiteral("fields_get"), QJsonArray(), params, &result, error)) {
		return types;
	}
	const QJsonObject object = result.toObject();
	for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
		types.insert(it.key(), it.value().toObject().value(QStringLiteral("type")).toString());
	}
	m_fields_cache.insert(model, types);
	return types;
}

/**
	@brief OdooClient::modelExists
	Tell whether an application is installed, e.g. mrp.bom for
	Manufacturing.
	@param model
	@param exists
	@param error
	@return false when the answer could not be obtained
*/
bool OdooClient::modelExists(const QString &model, bool *exists, QString *error)
{
	QJsonArray rows;
	QString problem;
	if (searchRead(QStringLiteral("ir.model"),
				   domain(QStringLiteral("model"), QStringLiteral("="), model),
				   {QStringLiteral("model")}, 1, &rows, &problem))
	{
		*exists = !rows.isEmpty();
		return true;
	}
		//A user may not read ir.model: ask for the fields of the model.
	QString fields_error;
	*exists = !fieldTypes(model, &fields_error).isEmpty();
	if (error) {
		error->clear();
	}
	return true;
}
