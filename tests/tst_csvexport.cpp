#include "csvexport.h"
#include "parser.h"

#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QtTest>

class TestCsvExport : public QObject
{
    Q_OBJECT

private slots:
    void writesColumns()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("out.csv");
        QVERIFY(CsvExport::writeColumns(path, {"a", "b \"x\""}, {{1, 2, 3}, {0.5}}));

        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(QString::fromUtf8(file.readAll()), QString("\"a\",\"b \"\"x\"\"\"\n1,0.5\n2,\n3,\n"));
    }

    void bigExportIsFast() // issue #7: 200 000 rows used to take hours
    {
        const int rows = 200000;
        Parser parser;
        QStringList labels;
        QList<double> values;
        QList<long> times;
        for (int row = 0; row < rows; ++row)
            for (const char *label : {"t", "a", "b", "c", "d"})
            {
                labels.append(label);
                values.append(row * 0.25);
                times.append(row);
            }
        parser.appendSetToMemory(labels, values, times);

        QElapsedTimer timer;
        timer.start();
        QStringList names;
        QList<QList<double>> columns;
        parser.getCSVReadyData(&names, &columns);
        QTemporaryDir dir;
        QVERIFY(CsvExport::writeColumns(dir.filePath("big.csv"), names, columns));
        const qint64 elapsed = timer.elapsed();

        QCOMPARE(names.count(), 5);
        QCOMPARE(columns.first().count(), rows);
        QVERIFY2(elapsed < 5000, qPrintable(QString("%1 ms").arg(elapsed)));
        qInfo("exported %d rows x 5 columns in %lld ms", rows, elapsed);
    }

    void reportsErrors()
    {
        QString error;
        QVERIFY(!CsvExport::writeColumns("/nonexistent/dir/out.csv", {"a"}, {{1}}, ',', &error));
        QVERIFY(!error.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestCsvExport)
#include "tst_csvexport.moc"
