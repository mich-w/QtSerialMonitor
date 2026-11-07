#include "filereader.h"

#include <QTemporaryDir>
#include <QSignalSpy>
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

    void loadCsvAsynchronously()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("data.csv");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("\"time\",\"a\"\n2024-01-01T10:00:00.000Z,1\n2024-01-01T10:00:01.000Z,2\n");
        file.close();

        FileReader reader;
        QSignalSpy loadedSpy(&reader, &FileReader::loaded);

        FileReader::Options options;
        options.csv = true;
        QVERIFY(reader.load(path, options));
        QVERIFY(reader.isBusy());
        QVERIFY(!reader.load(path, options)); // only one load at a time

        QVERIFY(loadedSpy.wait(5000));
        const auto result = loadedSpy.at(0).at(0).value<FileReader::Result>();
        QVERIFY(result.error.isEmpty());
        QVERIFY(!result.cancelled);
        QCOMPARE(result.values, (QList<double>{1, 2}));
        QCOMPARE(result.timeStamps.last(), long(QTime(10, 0, 1).msecsSinceStartOfDay()));
        QVERIFY(!reader.isBusy());
    }

    void cancelLargeLoad()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("big.txt");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        for (auto i = 0; i < 200000; ++i)
            file.write(QByteArray("10:00:00.000 a ") + QByteArray::number(i) + " b 2 c 3\n");
        file.close();

        FileReader reader;
        QSignalSpy progressSpy(&reader, &FileReader::progress);
        QSignalSpy loadedSpy(&reader, &FileReader::loaded);

        QVERIFY(reader.load(path, {}));
        QTRY_VERIFY_WITH_TIMEOUT(!progressSpy.isEmpty(), 5000); // parsing has started
        reader.cancel();

        QVERIFY(loadedSpy.wait(5000));
        const auto result = loadedSpy.at(0).at(0).value<FileReader::Result>();
        QVERIFY(result.cancelled);
    }

    void unreadableFileReportsError()
    {
        FileReader reader;
        QSignalSpy loadedSpy(&reader, &FileReader::loaded);
        QVERIFY(reader.load("/nonexistent/file.txt", {}));
        QVERIFY(loadedSpy.wait(5000));
        QVERIFY(!loadedSpy.at(0).at(0).value<FileReader::Result>().error.isEmpty());
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
