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
#include "pictureurl.h"

#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>

namespace {

constexpr qint64 max_size = 10 * 1024 * 1024;
constexpr int timeout_ms = 30000;

/// Get @p url in memory. Only https, a size limit, a time limit.
bool get(const QUrl &url, QByteArray *data, QString *type, QString *error)
{
	if (url.scheme() != QLatin1String("https")) {
		*error = QStringLiteral("only https addresses are fetched");
		return false;
	}
	QNetworkAccessManager manager;
	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
						 QNetworkRequest::NoLessSafeRedirectPolicy);
	request.setHeader(QNetworkRequest::UserAgentHeader,
					  QStringLiteral("QElectroTech picture import"));
	QNetworkReply *reply = manager.get(request);
	QEventLoop loop;
	QTimer timer;
	timer.setSingleShot(true);
	bool too_big = false;
	QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
	QObject::connect(reply, &QNetworkReply::downloadProgress, &loop,
					 [&](qint64 received, qint64) {
						 if (received > max_size) {
							 too_big = true;
							 reply->abort();
						 }
					 });
	timer.start(timeout_ms);
	loop.exec();
	const bool timed_out = !timer.isActive();
	timer.stop();
	if (too_big) {
		*error = QStringLiteral("the file is larger than 10 MB");
	} else if (timed_out && !reply->isFinished()) {
		reply->abort();
		*error = QStringLiteral("no answer after 30 seconds");
	} else if (reply->error() != QNetworkReply::NoError) {
		*error = reply->errorString();
	}
	const bool ok = error->isEmpty();
	if (ok) {
		*data = reply->readAll();
		*type = reply->header(QNetworkRequest::ContentTypeHeader).toString();
	}
	reply->deleteLater();
	return ok;
}

} // namespace

namespace PictureUrl {

bool isUrl(const QString &text)
{
	const QString t = text.trimmed().toLower();
	return t.startsWith(QLatin1String("http://"))
		|| t.startsWith(QLatin1String("https://"));
}

QUrl pictureLinkIn(const QByteArray &html, const QUrl &base)
{
	const QString text = QString::fromUtf8(html);
	// Both orders of the attributes of the meta tag
	static const QRegularExpression og1(
		QStringLiteral("<meta[^>]+property=[\"']og:image[\"'][^>]+content=[\"']([^\"']+)[\"']"),
		QRegularExpression::CaseInsensitiveOption);
	static const QRegularExpression og2(
		QStringLiteral("<meta[^>]+content=[\"']([^\"']+)[\"'][^>]+property=[\"']og:image[\"']"),
		QRegularExpression::CaseInsensitiveOption);
	static const QRegularExpression beckhoff(
		QStringLiteral("https://multimedia\\.beckhoff\\.com/media/[^\"'\\s\\\\]*_web_preview\\.png"));
	for (const QRegularExpression *re : {&og1, &og2}) {
		const QRegularExpressionMatch m = re->match(text);
		if (m.hasMatch()) {
			return base.resolved(QUrl(m.captured(1)));
		}
	}
	const QRegularExpressionMatch m = beckhoff.match(text);
	return m.hasMatch() ? QUrl(m.captured(0)) : QUrl();
}

QString cacheFolder()
{
	return QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
		+ QStringLiteral("/downloaded-pictures");
}

bool download(const QString &url, QString *file, QUrl *pictureUrl,
			  QString *error)
{
	QString problem;
	QByteArray data;
	QString type;
	QUrl picture(url.trimmed());
	if (!get(picture, &data, &type, &problem)) {
		*error = problem;
		return false;
	}
	if (type.startsWith(QLatin1String("text/html"))) {
		const QUrl link = pictureLinkIn(data, picture);
		if (link.isEmpty()) {
			*error = QStringLiteral("no picture found on that page: give the "
									"address of the picture itself");
			return false;
		}
		picture = link;
		if (!get(picture, &data, &type, &problem)) {
			*error = problem;
			return false;
		}
	}
	if (!type.startsWith(QLatin1String("image/"))) {
		*error = QStringLiteral("that is not a picture (%1)").arg(type);
		return false;
	}
	QString name = QFileInfo(picture.path()).fileName();
	if (name.isEmpty() || !name.contains(QLatin1Char('.'))) {
		name = QStringLiteral("picture.png");
	}
	QDir().mkpath(cacheFolder());
	const QString path = cacheFolder() + QLatin1Char('/') + name;
	QSaveFile out(path);
	if (!out.open(QIODevice::WriteOnly) || out.write(data) < 0
		|| !out.commit()) {
		*error = QStringLiteral("cannot write %1").arg(path);
		return false;
	}
	*file = path;
	if (pictureUrl) {
		*pictureUrl = picture;
	}
	return true;
}

bool isInGitRepository(const QString &path)
{
	QDir dir(QFileInfo(path).absolutePath());
	do {
		if (QFileInfo::exists(dir.filePath(QStringLiteral(".git")))) {
			return true;
		}
	} while (dir.cdUp());
	return false;
}

QString notice()
{
	return QStringLiteral(
		"The picture belongs to its author (for a product, the "
		"manufacturer). It was kept in %1 and the element made from it is "
		"for your own use: do not add either to a repository or share them "
		"without the permission of the owner.")
		.arg(QDir::toNativeSeparators(cacheFolder()));
}

} // namespace PictureUrl
