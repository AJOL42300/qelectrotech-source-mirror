// SPDX-License-Identifier: GPL-2.0-or-later
/*
	--export-html writes the interactive technical dossier of a project, and
	--pdf-bookmarks adds the folios' table of contents to --export-pdf. Runs
	the real binary on examples/industrial.qet.
*/
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>
#include "qettesttimeout.h"

class tst_htmldossier : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	int run(const QStringList &arguments, QString *output = nullptr)
	{
		const QString home = m_dir.filePath(QStringLiteral("home"));
		const QString tmp = m_dir.filePath(QStringLiteral("tmp"));
		QDir().mkpath(home);
		QDir().mkpath(tmp);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/.config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/.local/share"));
		env.insert(QStringLiteral("TMPDIR"), tmp);

		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.setProcessChannelMode(QProcess::MergedChannels);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), arguments);
		if (!proc.waitForFinished(QetTest::timeout(240000))) {
			proc.kill();
			return -1;
		}
		if (output)
			*output = QString::fromLocal8Bit(proc.readAll());
		return proc.exitCode();
	}

	static QString project()
	{
		return QStringLiteral(QET_EXAMPLES_DIR) + QStringLiteral("/industrial.qet");
	}

	QString read(const QString &path) const
	{
		QFile file(m_dir.filePath(path));
		return file.open(QIODevice::ReadOnly)
			? QString::fromUtf8(file.readAll()) : QString();
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
		QVERIFY2(QFile::exists(project()), "examples/industrial.qet not found");
	}

	void exportsPages()
	{
		const QString out = m_dir.filePath(QStringLiteral("dossier"));
		QString log;
		QVERIFY2(run({QStringLiteral("--export-html"), project(), out,
					  QStringLiteral("--html-no-pdf")}, &log) == 0,
				 qPrintable(log));

		for (const char *name : {"index.html", "search.html",
								 "nomenclature.html", "dossier.css",
								 "dossier.js", "data/index.js",
								 "data/search-index.js", "data/common.js",
								 "folio-01.html", "folio-01.svg",
								 "data/folio-01.js"})
			QVERIFY2(QFile::exists(out + QLatin1Char('/') + QLatin1String(name)),
					 name);
		QVERIFY(!QFile::exists(out + QStringLiteral("/dossier.pdf")));

		// Offline: nothing is fetched from the network
		for (const char *name : {"index.html", "folio-01.html", "search.html"}) {
			const QString html = read(QStringLiteral("dossier/")
									  + QLatin1String(name));
			QVERIFY2(!html.contains(QStringLiteral("http://"))
					 && !html.contains(QStringLiteral("https://")), name);
		}
		QVERIFY(read(QStringLiteral("dossier/data/index.js"))
				.contains(QStringLiteral("\"folios\"")));
	}

	void onlyTheFoliosAsked()
	{
		const QString out = m_dir.filePath(QStringLiteral("two"));
		QString log;
		QVERIFY2(run({QStringLiteral("--export-html"), project(), out,
					  QStringLiteral("--html-no-pdf"),
					  QStringLiteral("--html-folios"), QStringLiteral("2")},
					 &log) == 0, qPrintable(log));
		QVERIFY(QFile::exists(out + QStringLiteral("/folio-02.html")));
		QVERIFY(!QFile::exists(out + QStringLiteral("/folio-01.html")));
	}

	void pdfHasBookmarks()
	{
		const QString out = m_dir.filePath(QStringLiteral("marks.pdf"));
		QString log;
		QVERIFY2(run({QStringLiteral("--export-pdf"), project(), out,
					  QStringLiteral("--pdf-bookmarks")}, &log) == 0,
				 qPrintable(log));
		QFile file(out);
		QVERIFY(file.open(QIODevice::ReadOnly));
		const QByteArray pdf = file.readAll();
		QVERIFY(pdf.startsWith("%PDF"));
		QVERIFY(pdf.contains("/Outlines"));
		QVERIFY(pdf.contains("/UseOutlines"));
		QVERIFY(pdf.trimmed().endsWith("%%EOF"));
	}
};

QTEST_MAIN(tst_htmldossier)
#include "tst_htmldossier.moc"
