#include "logger.h"

#include <QTemporaryDir>
#include <QtTest>

class TestLogger : public QObject
{
    Q_OBJECT

    static QStringList readLines(const QString &path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return {};
        return QString::fromUtf8(file.readAll()).split('\n', Qt::SkipEmptyParts);
    }

private slots:
    void csvColumnsStayAligned()
    {
        QTemporaryDir dir;
        {
            Logger logger;
            QVERIFY(logger.beginLog(dir.path(), false, "x.csv", true));
            logger.writeLogCSV({"a"}, {1});
            logger.writeLogCSV({"a", "b"}, {2, 3});
            logger.writeLogCSV({"b"}, {4});
        }

        const auto lines = readLines(dir.filePath("x.csv"));
        QCOMPARE(lines.count(), 4);
        QCOMPARE(lines[0], QString("\"time\",\"a\",\"b\""));
        QVERIFY(lines[1].endsWith(",1"));
        QVERIFY(lines[2].endsWith(",2,3"));
        QVERIFY(lines[3].endsWith(",,4"));
    }

    void csvAppendReusesHeader()
    {
        QTemporaryDir dir;
        {
            Logger logger;
            QVERIFY(logger.beginLog(dir.path(), false, "x.csv", true));
            logger.writeLogCSV({"a", "b"}, {1, 2});
        }
        {
            Logger logger;
            QVERIFY(logger.beginLog(dir.path(), false, "x.csv", false));
            logger.writeLogCSV({"c", "a"}, {5, 6});
        }

        const auto lines = readLines(dir.filePath("x.csv"));
        QCOMPARE(lines.count(), 3);
        QCOMPARE(lines[0], QString("\"time\",\"a\",\"b\",\"c\""));
        QVERIFY(lines[1].endsWith(",1,2"));
        QVERIFY(lines[2].endsWith(",6,,5"));
    }

    void txtAppendsWithIsoTimestamps()
    {
        QTemporaryDir dir;
        {
            Logger logger;
            QVERIFY(logger.beginLog(dir.path(), false, "x.txt", false));
            logger.writeLogTXTLine("hello\r\nworld", false);
            logger.writeLogTXTParsedData({"a", "b"}, {1.5, 2});
        }
        {
            Logger logger;
            QVERIFY(logger.beginLog(dir.path(), false, "x.txt", false));
            logger.writeLogTXTLine("appended", false);
        }

        const auto lines = readLines(dir.filePath("x.txt"));
        QCOMPARE(lines.count(), 4);
        QVERIFY(lines[0].endsWith(" hello"));
        QVERIFY(lines[1].endsWith(" world"));
        QVERIFY(lines[2].endsWith(" a 1.5 :: b 2"));
        QVERIFY(lines[3].endsWith(" appended"));

        // Timestamp must parse as ISO 8601 with milliseconds (minutes, not month).
        const QString stamp = lines[0].section(' ', 0, 0);
        QVERIFY2(QDateTime::fromString(stamp, Qt::ISODateWithMs).isValid(), qPrintable(stamp));
    }

    void createsMissingDirectory()
    {
        QTemporaryDir dir;
        Logger logger;
        QVERIFY(logger.beginLog(dir.filePath("nested/logs"), false, "x.txt"));
        QVERIFY(QFile::exists(dir.filePath("nested/logs/x.txt")));
    }

    void autoLoggingUsesTimestampedName()
    {
        QTemporaryDir dir;
        Logger logger;
        QVERIFY(logger.beginLog(dir.path(), true, "%DateTime%_Log.csv"));
        const auto files = QDir(dir.path()).entryList(QDir::Files);
        QCOMPARE(files.count(), 1);
        QVERIFY(files[0].endsWith("_Log.csv"));
        QVERIFY(!files[0].contains(':'));
    }
};

QTEST_GUILESS_MAIN(TestLogger)
#include "tst_logger.moc"
