#include "filereader.h"

#include <QTemporaryDir>
#include <QtTest>

class TestFileReader : public QObject
{
    Q_OBJECT

    static QString writeFile(const QTemporaryDir &dir, const QByteArray &content)
    {
        const QString path = dir.filePath("log.txt");
        QFile file(path);
        if (file.open(QIODevice::WriteOnly))
            file.write(content);
        return path;
    }

private slots:
    void timeRangeFromHeadAndTail()
    {
        QTemporaryDir dir;
        QFile file(writeFile(dir, "header\n10:00:00.100 a 1\n10:00:01.000 a 2\n10:05:00.000 a 3\ntrailing\n"));

        const auto range = FileReader::getFileTimeRange(&file);
        QCOMPARE(range.first(), QTime(10, 0, 0, 100));
        QCOMPARE(range.last(), QTime(10, 5, 0, 0));
    }

    void isoTimestamps()
    {
        QTemporaryDir dir;
        QFile file(writeFile(dir, "2024-01-01T08:00:00.000Z x\n2024-01-01T09:15:00.000Z y\n"));

        const auto range = FileReader::getFileTimeRange(&file);
        QCOMPARE(range.first(), QTime(8, 0));
        QCOMPARE(range.last(), QTime(9, 15));
    }

    void noTimestampsReturnsInvalid() // previously undefined behaviour (missing return)
    {
        QTemporaryDir dir;
        QFile file(writeFile(dir, "no time here\nnone here either\n"));

        const auto range = FileReader::getFileTimeRange(&file);
        QCOMPARE(range.count(), 2);
        QVERIFY(!range.first().isValid());
        QVERIFY(!range.last().isValid());
    }

    void missingFile()
    {
        QFile file("/nonexistent/file.txt");
        const auto range = FileReader::getFileTimeRange(&file);
        QVERIFY(!range.first().isValid());
    }
};

QTEST_GUILESS_MAIN(TestFileReader)
#include "tst_filereader.moc"
