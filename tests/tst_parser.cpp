#include "parser.h"
#include "receivepipeline.h"

#include <cmath>

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
        QTest::newRow("arduino plotter colon") << "Sensor1:12,Sensor2:34" << QStringList{"Sensor1", "Sensor2"} << QList<double>{12, 34};
        QTest::newRow("arduino plotter tabs") << "a:1\tb:-2.5\tc:3e2" << QStringList{"a", "b", "c"} << QList<double>{1, -2.5, 300};
        QTest::newRow("teleplot") << ">temp:21.5\n>hum:40\n" << QStringList{"temp", "hum"} << QList<double>{21.5, 40};
        QTest::newRow("teleplot unit and flags") << QString::fromUtf8(">volt:3.3§V|g\n") << QStringList{"volt"} << QList<double>{3.3};
        QTest::newRow("teleplot text ignored") << ">state:running|t\n>xy:1:2|xy\n" << QStringList{} << QList<double>{};
        QTest::newRow("mixed teleplot and plain") << ">a:1\nb 2\n" << QStringList{"a", "b"} << QList<double>{1, 2};
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

    void teleplotTimestamp()
    {
        const QDateTime when(QDate(2024, 1, 1), QTime(12, 34, 56, 789));
        Parser parser;
        parser.parse(QString(">temp:%1:21.5\n").arg(when.toMSecsSinceEpoch()), false, true, "");

        const auto result = samples(parser);
        QCOMPARE(result.count(), 1);
        QCOMPARE(result[0].time, long(QTime(12, 34, 56, 789).msecsSinceStartOfDay()));
    }

    void timeTokensAreNotLabelColonValue()
    {
        Parser parser;
        parser.parse("10:30:00.000 x 1", false, true, "");
        QCOMPARE(labels(parser), QStringList{"x"});
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

        // Empty cell must not shift later values into the wrong column.
        QCOMPARE(result[2].label, QString("b"));
        QCOMPARE(result[2].value, 3.0);
        QCOMPARE(result[2].time, long(QTime(10, 30, 1).msecsSinceStartOfDay()));
    }

    void csvLoggerOutputRoundTrip()
    {
        Parser parser;
        parser.parseCSV("\"time\",\"a\",\"b\"\n"
                        "2024-01-01T10:00:00.000Z,1\n"
                        "2024-01-01T10:00:01.000Z,,4\n");

        QCOMPARE(parser.getStringListLabels(), (QStringList{"a", "b"}));
        QCOMPARE(parser.getListNumericValues(), (QList<double>{1, 4}));
    }

    void csvDelimitersAndQuoting_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QStringList>("expectedLabels");
        QTest::addColumn<QList<double>>("expectedValues");

        QTest::newRow("semicolon") << "x;y\n1;2\n" << QStringList{"x", "y"} << QList<double>{1, 2};
        QTest::newRow("tab") << "x\ty\n1\t2\n" << QStringList{"x", "y"} << QList<double>{1, 2};
        QTest::newRow("quoted comma in header") << "\"temp, C\",rpm\n20,100\n" << QStringList{"temp, C", "rpm"} << QList<double>{20, 100};
        QTest::newRow("no header") << "1,2\n3,4\n" << QStringList{"Graph 0", "Graph 1", "Graph 0", "Graph 1"} << QList<double>{1, 2, 3, 4};
        QTest::newRow("label with spaces") << "Motor speed,Load\n5,6\n" << QStringList{"Motor speed", "Load"} << QList<double>{5, 6};
    }

    void csvDelimitersAndQuoting()
    {
        QFETCH(QString, input);
        QFETCH(QStringList, expectedLabels);
        QFETCH(QList<double>, expectedValues);

        Parser parser;
        parser.parseCSV(input);
        QCOMPARE(parser.getStringListLabels(), expectedLabels);
        QCOMPARE(parser.getListNumericValues(), expectedValues);
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

    void highSpeedArduinoOutputSplitAnywhere() // issue #9
    {
        // The reporter's sketch: five comma-separated values per line, no labels.
        QByteArray stream;
        QList<QList<double>> expected;
        for (float x = 0.0f; x < 3.0f; x += 0.1f)
        {
            const QList<double> values = {std::sin(x) * 100.0, std::cos(x) * 100.0, std::sin(x) * 50.0, std::cos(x) * 50.0, std::sin(x)};
            QStringList fields;
            for (double v : values)
                fields << QString::number(v, 'f', 2); // Serial.print(float) prints 2 decimals
            stream += fields.join(',').toLatin1() + "\r\n";
            QList<double> rounded;
            for (const auto &f : fields)
                rounded << f.toDouble();
            expected << rounded;
        }

        // Serial data arrives in arbitrary chunks: try every split point.
        for (int split = 1; split < stream.size(); split += 7)
        {
            ReceivePipeline pipeline;
            QList<double> values;
            QStringList labels;
            connect(&pipeline, &ReceivePipeline::linesReceived, [&](const QString &lines, const QList<long> &times)
                    {
                        Parser parser;
                        parser.parse(lines, true, false, QString(), times);
                        values << parser.getListNumericValues();
                        labels << parser.getStringListLabels();
                    });
            pipeline.append(stream.left(split));
            pipeline.flush();
            pipeline.append(stream.mid(split));
            pipeline.flush();

            QCOMPARE(values.count(), expected.count() * 5);
            for (int line = 0; line < expected.count(); ++line)
                for (int column = 0; column < 5; ++column)
                {
                    QCOMPARE(values[line * 5 + column], expected[line][column]);
                    QCOMPARE(labels[line * 5 + column], "Graph " + QString::number(column));
                }
        }
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
