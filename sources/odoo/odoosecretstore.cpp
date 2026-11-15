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
#include "odoosecretstore.h"

#include <QCoreApplication>
#include <QUrl>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincred.h>
#endif

/**
	@param url the address of the Odoo server
	@param login the user name or e-mail
	@return the name under which the key of that user on that server is
	kept: it never contains the key
*/
QString OdooSecretStore::accountFor(const QUrl &url, const QString &login)
{
	return QStringLiteral("QElectroTech/Odoo/%1|%2")
			.arg(url.host().toLower() + (url.port() > 0 ? QStringLiteral(":%1").arg(url.port()) : QString()),
				 login.trimmed().toLower());
}

QString OdooMemorySecretStore::name() const
{
	return QCoreApplication::translate("OdooSecretStore", "this session only");
}

bool OdooMemorySecretStore::store(const QString &account, const QString &secret, QString *error)
{
	if (error) {
		error->clear();
	}
	m_secrets.insert(account, secret);
	return true;
}

QString OdooMemorySecretStore::read(const QString &account, QString *error)
{
	if (error) {
		error->clear();
	}
	return m_secrets.value(account);
}

bool OdooMemorySecretStore::remove(const QString &account, QString *error)
{
	if (error) {
		error->clear();
	}
	m_secrets.remove(account);
	return true;
}

#ifdef Q_OS_WIN
namespace {

/**
	@brief The Windows Credential Manager, as a generic credential per
	account. The key is stored encrypted for the current Windows user.
*/
class OdooWindowsSecretStore : public OdooSecretStore
{
	public:
		QString name() const override
		{
			return QCoreApplication::translate("OdooSecretStore", "Windows Credential Manager");
		}
		bool isPersistent() const override {return true;}

		bool store(const QString &account, const QString &secret, QString *error) override
		{
			if (error) {
				error->clear();
			}
			QByteArray blob = secret.toUtf8();
			const std::wstring target = account.toStdWString();
			const std::wstring user = QStringLiteral("QElectroTech").toStdWString();

			CREDENTIALW credential;
			ZeroMemory(&credential, sizeof(credential));
			credential.Type = CRED_TYPE_GENERIC;
			credential.TargetName = const_cast<LPWSTR>(target.c_str());
			credential.CredentialBlobSize = static_cast<DWORD>(blob.size());
			credential.CredentialBlob = reinterpret_cast<LPBYTE>(blob.data());
			credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
			credential.UserName = const_cast<LPWSTR>(user.c_str());
			const bool ok = CredWriteW(&credential, 0) == TRUE;
			if (!ok && error) {
				*error = errorText(GetLastError());
			}
			SecureZeroMemory(blob.data(), static_cast<SIZE_T>(blob.size()));
			return ok;
		}

		QString read(const QString &account, QString *error) override
		{
			if (error) {
				error->clear();
			}
			PCREDENTIALW credential = nullptr;
			if (!CredReadW(account.toStdWString().c_str(), CRED_TYPE_GENERIC, 0, &credential))
			{
				const DWORD code = GetLastError();
				if (error && code != ERROR_NOT_FOUND) {
					*error = errorText(code);
				}
				return QString();
			}
			const QString secret = QString::fromUtf8(
						reinterpret_cast<const char *>(credential->CredentialBlob),
						static_cast<qsizetype>(credential->CredentialBlobSize));
			CredFree(credential);
			return secret;
		}

		bool remove(const QString &account, QString *error) override
		{
			if (error) {
				error->clear();
			}
			if (CredDeleteW(account.toStdWString().c_str(), CRED_TYPE_GENERIC, 0)) {
				return true;
			}
			const DWORD code = GetLastError();
			if (code == ERROR_NOT_FOUND) {
				return true;
			}
			if (error) {
				*error = errorText(code);
			}
			return false;
		}

	private:
		static QString errorText(DWORD code)
		{
			return QCoreApplication::translate("OdooSecretStore", "Windows error %1").arg(code);
		}
};

}
#endif

/**
	@return the store of the system: the Windows Credential Manager, or a
	store kept in memory only
*/
OdooSecretStore *OdooSecretStore::system()
{
#ifdef Q_OS_WIN
	static OdooWindowsSecretStore store;
#else
	static OdooMemorySecretStore store;
#endif
	return &store;
}
