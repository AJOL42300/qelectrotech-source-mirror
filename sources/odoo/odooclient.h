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
#ifndef ODOOCLIENT_H
#define ODOOCLIENT_H

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <memory>

/**
	@brief The two ways Odoo is spoken to.

	JsonRpc is the /jsonrpc endpoint of Odoo 17 and 18 (still there in
	19, announced for removal in Odoo 22). Json2 is the /json/2 API
	introduced by Odoo 19. Auto tries Json2 first and falls back to JsonRpc
	when the server does not know it.
*/
enum class OdooProtocol {
	Auto,
	Json2,
	JsonRpc
};

/**
	@brief How to reach an Odoo server. The API key is held here only in
	memory: saveSettings() never writes it, and nothing here logs it.
*/
struct OdooConnection
{
	QUrl url = QUrl(QStringLiteral("https://mcatech.odoo.com"));
	QString database = QStringLiteral("mcatech");
	QString login;
	QString api_key;
	OdooProtocol protocol = OdooProtocol::Auto;
	int timeout_ms = 30000;

	void loadSettings();
	void saveSettings() const;
	static QString protocolName(OdooProtocol protocol);
};

/**
	@brief One way of calling a method of a model of Odoo. A backend
	speaks one protocol; OdooClient chooses one.

	ids are the records the method acts on, params its named arguments.
	A backend refuses unlink: QElectroTech never deletes anything in Odoo.
*/
class OdooBackend
{
	public:
		virtual ~OdooBackend() = default;

		virtual OdooProtocol protocol() const = 0;
		virtual bool authenticate(QString *error) = 0;
		virtual bool call(const QString &model, const QString &method,
						  const QJsonArray &ids, const QJsonObject &params,
						  QJsonValue *result, QString *error) = 0;
		virtual QString serverVersion() const {return QString();}

			//Status of the last http answer, 0 when there was none
		int lastStatus() const {return m_last_status;}

	protected:
		int m_last_status = 0;
};

/**
	@brief The OdooClient class is the connection to an Odoo server:
	authentication, a few reads and the two writes the exchange needs
	(create and write). It is synchronous, and meant to be used from the
	user interface thread on explicit request.

	The API key is only handed to the HTTP request. Every message going out
	of this class (errors, log) goes through redact().
*/
class OdooClient : public QObject
{
	Q_OBJECT

	public:
		explicit OdooClient(const OdooConnection &connection, QObject *parent = nullptr);
		~OdooClient() override;

		bool connectToServer(QString *error);
		bool isConnected() const {return m_backend != nullptr;}
		OdooProtocol protocol() const;
		QString serverVersion() const;

		bool searchRead(const QString &model, const QJsonArray &domain,
						const QStringList &fields, int limit,
						QJsonArray *rows, QString *error);
		bool create(const QString &model, const QJsonObject &values,
					qint64 *id, QString *error);
		bool write(const QString &model, const QList<qint64> &ids,
				   const QJsonObject &values, QString *error);
		QMap<QString, QString> fieldTypes(const QString &model, QString *error);
		bool modelExists(const QString &model, bool *exists, QString *error);

		QString redact(const QString &text) const;
		QStringList log() const {return m_log;}
		int writeCount() const {return m_write_count;}

		static bool isUrlAllowed(const QUrl &url, QString *error = nullptr);
		static QJsonArray domain(const QString &field, const QString &op,
								 const QJsonValue &value);

	signals:
		void logged(const QString &line);

	private:
		bool call(const QString &model, const QString &method,
				  const QJsonArray &ids, const QJsonObject &params,
				  QJsonValue *result, QString *error);
		void addLog(const QString &line);

		OdooConnection m_connection;
		std::unique_ptr<OdooBackend> m_backend;
		QMap<QString, QMap<QString, QString>> m_fields_cache;
		QStringList m_log;
		int m_write_count = 0;
};

#endif // ODOOCLIENT_H
