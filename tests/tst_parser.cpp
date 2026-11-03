#include "parser.h"

#include <QtTest>

class TestParser : public QObject
{
    Q_OBJECT

private:
    struct Sample
    {
        QString label;
        double value;
        long time;
    };

    static QList<Sample> samples(Parser &parser)
    {
        QList<Sample> result;
        const auto labels = parser.getStringListLabels();
        const auto values = parser.getListNumericValues();
        const auto times = parser.getListTimeStamp();
        for (auto i = 0; i < labels.count(); ++i)
            result.append({labels[i], values[i], times[i]});
        return result;
    }

    static QStringList labels(Parser &parser) { return parser.getStringListLabels(); }

private slots:
    void labelValuePairs_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QStringList>("expectedLabels");
        QTest::addColumn<QList<double>>("expectedValues");

        QTest::newRow("equals") << "Roll = 1.23 Pitch = 45.6" << QStringList{"Roll", "Pitch"} << QList<double>{1.23, 45.6};
        QTest::newRow("colon label") << "Voltage: 1.23\tOutput: 4.56" << QStringList{"Voltage", "Output"} << QList<double>{1.23, 4.56};
        QTest::newRow("comma separated") << "a=1,b=2" << QStringList{"a", "b"} << QList<double>{1, 2};
        QTest::newRow("unlabelled") << "1.23 4.56" << QStringList{"Graph 0", "Graph 1"} << QList<double>{1.23, 4.56};
        QTest::newRow("plus sign") << "Temp +1.5" << QStringList{"Temp"} << QList<double>{1.5};
        QTest::newRow("scientific") << "sci 1e-3 big 2.5E+3" << QStringList{"sci", "big"} << QList<double>{0.001, 2500};
        QTest::newRow("negative and dot") << "x -.5 y 3." << QStringList{"x", "y"} << QList<double>{-0.5, 3};
        QTest::newRow("no numbers") << "hello world" << QStringList{} << QList<double>{};
        QTest::newRow("multi line") << "a 1\r\nb 2\n\nc 3\r" << QStringList{"a", "b", "c"} << QList<double>{1, 2, 3};
    }

    void labelValuePairs()
    {
        QFETCH(QString, input);
        QFETCH(QStringList, expectedLabels);
        QFETCH(QList<double>, expectedValues);

        Parser parser;
        parser.parse(input, false);

        QCOMPARE(parser.getStringListLabels(), expectedLabels);
        QCOMPARE(parser.getListNumericValues(), expectedValues);
        QCOMPARE(parser.getListTimeStamp().count(), expectedLabels.count());
    }

    void externalClockLabel()
    {
        Parser parser;
        parser.parse("time 5000 a 1", true, true, "time");

        const auto result = samples(parser);
        QCOMPARE(result.count(), 2);
        QCOMPARE(result[1].label, QString("a"));
        QCOMPARE(result[1].time, 5000L);
    }

    void externalClockLabelAtEndOfLine()
    {
        Parser parser;
        parser.parse("a 1 time", true, true, "time"); // used to read past the end of the token list
        QCOMPARE(labels(parser), QStringList{"a"});
    }

    void externalClockTimeOfDay()
    {
        Parser parser;
        parser.parse("10:30:00.250 x 1", false, true, "");
        const auto result = samples(parser);
        QCOMPARE(result.count(), 1);
        QCOMPARE(result[0].time, long(QTime(10, 30, 0, 250).msecsSinceStartOfDay()));
    }

    void timeRangeSkipsWholeLines()
    {
        Parser parser;
        parser.setParsingTimeRange(QTime(10, 0), QTime(11, 0));
        parser.parse("09:00:00.000 x 1 y 1\n10:30:00.000 x 2 y 2\n12:00:00.000 x 3 y 3", false, true, "");

        QCOMPARE(parser.getListNumericValues(), (QList<double>{2, 2}));

        parser.resetTimeRange();
        parser.parse("09:00:00.000 x 1", false, true, "");
        QCOMPARE(parser.getListNumericValues(), QList<double>{1});
    }

    void csvWithHeaderAndIsoTime()
    {
        Parser parser;
        parser.parseCSV("\"time\",\"a\",\"b\",\n"
                        "2024-01-01T10:30:00.500Z,1.5,2,\n"
                        "2024-01-01T10:30:01.000Z,,3,\n");

        const auto result = samples(parser);
        QCOMPARE(result.count(), 3);
        QCOMPARE(result[0].label, QString("a"));
        QCOMPARE(result[0].value, 1.5);
        QCOMPARE(result[0].time, long(QTime(10, 30, 0, 500).msecsSinceStartOfDay()));
        QCOMPARE(result[1].label, QString("b"));
        // Note: empty CSV cells collapse, so values after an empty cell shift left.
    }

    void csvExternalClockColumn()
    {
        Parser parser;
        parser.parseCSV("ms,v\n1000,5\n2000,6\n", true, "ms");

        const auto result = samples(parser);
        QCOMPARE(result.count(), 4);
        QCOMPARE(result[1].label, QString("v"));
        QCOMPARE(result[1].time, 1000L);
        QCOMPARE(result[3].time, 2000L);
    }

    void storageToCsvColumns()
    {
        Parser parser;
        parser.appendSetToMemory({"a", "b", "a"}, {1, 2, 3}, {0, 0, 0});

        QStringList names;
        QList<QList<double>> columns;
        parser.getCSVReadyData(&names, &columns);

        QCOMPARE(names, (QStringList{"a", "b"}));
        QCOMPARE(columns[0], (QList<double>{1, 3}));
        QCOMPARE(columns[1], QList<double>{2});
    }
};

QTEST_GUILESS_MAIN(TestParser)
#include "tst_parser.moc"
