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
#ifndef ODOOSECRETSTORE_H
#define ODOOSECRETSTORE_H

#include <QMap>
#include <QString>

class QUrl;

/**
	@brief The OdooSecretStore class keeps the Odoo API key outside of
	the project and of the settings files.

	The key is only ever handed to a store: it is never written in a .qet
	file, in QSettings, in a log. The system store is the Windows
	Credential Manager; where there is none (other systems here), the
	store keeps the key in memory for the session only, and the user types
	it again next time.
*/
class OdooSecretStore
{
	public:
		virtual ~OdooSecretStore() = default;

		virtual QString name() const = 0;
		virtual bool isPersistent() const = 0;
		virtual bool store(const QString &account, const QString &secret,
						   QString *error = nullptr) = 0;
		virtual QString read(const QString &account, QString *error = nullptr) = 0;
		virtual bool remove(const QString &account, QString *error = nullptr) = 0;

		static OdooSecretStore *system();
		static QString accountFor(const QUrl &url, const QString &login);
};

/**
	@brief A store that lives only as long as the program. Used where the
	system has no vault, and by the tests.
*/
class OdooMemorySecretStore : public OdooSecretStore
{
	public:
		QString name() const override;
		bool isPersistent() const override {return false;}
		bool store(const QString &account, const QString &secret,
				   QString *error = nullptr) override;
		QString read(const QString &account, QString *error = nullptr) override;
		bool remove(const QString &account, QString *error = nullptr) override;

	private:
		QMap<QString, QString> m_secrets;
};

#endif // ODOOSECRETSTORE_H
