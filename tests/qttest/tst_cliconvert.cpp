// SPDX-License-Identifier: GPL-2.0-or-later
// --convert-to-qetz and --convert-to-qet: a project converted to a .qetz and
// back has the same content, the input is left alone, and an output with
// the wrong extension, or the input itself, is refused. Runs the real binary.

#include <QtTest>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include "qettesttimeout.h"

class tst_cliconvert : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	struct Result {
		int code = -1;
		QString out, err;
	};

	Result run(const QStringList &args)
	{
		const QString home = m_dir.filePath(QStringLiteral("home"));
		QDir().mkpath(home);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   QStringList{QStringLiteral("--common-elements-dir=" QET_ELEMENTS_DIR "/")} + args);
		Result result;
		if (!proc.waitForFinished(QetTest::timeout(300000))) return result;
		result.code = proc.exitCode();
		result.out = QString::fromUtf8(proc.readAllStandardOutput());
		result.err = QString::fromUtf8(proc.readAllStandardError());
		return result;
	}

	static QByteArray digest(const QString &path)
	{
		QFile f(path);
		return f.open(QIODevice::ReadOnly)
				? QCryptographicHash::hash(f.readAll(), QCryptographicHash::Sha256) : QByteArray();
	}

	// The project's counts and lists, without the path it was read from
	QString info(const QString &path)
	{
		const Result r = run({QStringLiteral("--info"), path});
		QString text = r.out;
		text.remove(QRegularExpression(QStringLiteral("\"(path|file)\"\\s*:\\s*\"[^\"]*\",?")));
		return r.code == 0 ? text : QString();
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	// .qet -> .qetz -> .qet: same content, the input untouched
	void roundTrip()
	{
		const QString source = QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet");
		const QString qet = m_dir.filePath(QStringLiteral("a.qet"));
		const QString qetz = m_dir.filePath(QStringLiteral("a.qetz"));
		const QString back = m_dir.filePath(QStringLiteral("b.qet"));
		QVERIFY(QFile::copy(source, qet));
		const QByteArray before = digest(qet);

		Result r = run({QStringLiteral("--convert-to-qetz"), qet, qetz});
		QVERIFY2(r.code == 0, qPrintable(r.err.right(600)));
		QVERIFY(QFileInfo(qetz).size() > 0);
		QCOMPARE(digest(qet), before);

		r = run({QStringLiteral("--convert-to-qet"), qetz, back});
		QVERIFY2(r.code == 0, qPrintable(r.err.right(600)));

		const QString expected = info(qet);
		QVERIFY(!expected.isEmpty());
		QCOMPARE(info(qetz), expected);
		QCOMPARE(info(back), expected);
	}

	// The extension of the output says what is written
	void refusals()
	{
		const QString qet = QStringLiteral(QET_EXAMPLES_DIR "/grafcet.qet");
		const QString wrong = m_dir.filePath(QStringLiteral("wrong.qet"));
		QCOMPARE(run({QStringLiteral("--convert-to-qetz"), qet, wrong}).code, 2);
		QVERIFY(!QFile::exists(wrong));

		const QString copy = m_dir.filePath(QStringLiteral("copy.qet"));
		QVERIFY(QFile::copy(qet, copy));
		const QByteArray before = digest(copy);
		QCOMPARE(run({QStringLiteral("--convert-to-qet"), copy, copy}).code, 2);
		QCOMPARE(digest(copy), before);
	}
};

QTEST_MAIN(tst_cliconvert)
#include "tst_cliconvert.moc"
