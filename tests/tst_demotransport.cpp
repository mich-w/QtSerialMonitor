#include "demotransport.h"
#include "parser.h"

#include <QSignalSpy>
#include <QtTest>

class TestDemoTransport : public QObject
{
    Q_OBJECT

private slots:
    void sampleLineIsParsable()
    {
        Parser parser;
        parser.parse(QString::fromLatin1(DemoTransport::sampleLine(1.25)), false);

        QCOMPARE(parser.getStringListLabels(), (QStringList{"sin1", "sin2", "noisy"}));
        const auto values = parser.getListNumericValues();
        QCOMPARE(values[0], 1.0);  // quarter period of 5 s
        QCOMPARE(values[1], 0.35); // 0.7 * sin(pi/2 - pi/3)
    }

    void orientationOutputOnRequest()
    {
        Parser parser;
        parser.parse(QString::fromLatin1(DemoTransport::sampleLine(1.5, 0, true)), false);
        QCOMPARE(parser.getStringListLabels(), (QStringList{"sin1", "sin2", "noisy", "roll", "pitch", "yaw"}));
        QCOMPARE(parser.getListNumericValues()[3], 35.0); // quarter of the 6 s roll period

        DemoTransport demo;
        QVERIFY(!demo.orientationOutput());
        demo.setOrientationOutput(true);
        QVERIFY(demo.orientationOutput());
    }

    void linesEndLikeArduinoPrintln()
    {
        QVERIFY(DemoTransport::sampleLine(0).endsWith("\r\n"));
    }

    void streamsAtRequestedRate()
    {
        DemoTransport demo;
        QSignalSpy spy(&demo, &Transport::dataReceived);

        QVERIFY(demo.start(100));
        QVERIFY(demo.isOpen());
        QTest::qWait(300);
        demo.stop();
        QVERIFY(!demo.isOpen());

        int lines = 0;
        for (const auto &args : spy)
            lines += args.at(0).toByteArray().count('\n');
        QVERIFY2(lines >= 25 && lines <= 36, qPrintable(QString::number(lines))); // 300 ms at 100 Hz
    }

    void keepsRateWhenTicksAreDelayed()
    {
        DemoTransport demo;
        QSignalSpy spy(&demo, &Transport::dataReceived);
        demo.start(1000);

        // Block the event loop like a busy GUI thread: no timer ticks for 200 ms.
        QElapsedTimer busy;
        busy.start();
        while (busy.elapsed() < 200)
        {
        }
        QTest::qWait(50);
        demo.stop();

        int lines = 0;
        for (const auto &args : spy)
            lines += args.at(0).toByteArray().count('\n');
        QVERIFY2(lines >= 200 && lines <= 300, qPrintable(QString::number(lines))); // ~250 ms at 1 kHz
    }

    void echoesSentText()
    {
        DemoTransport demo;
        QVERIFY(!demo.send("ignored while stopped"));

        demo.start(1);
        QSignalSpy spy(&demo, &Transport::dataReceived);
        QVERIFY(demo.send("ping\n"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toByteArray(), QByteArray("demo> received: ping\r\n"));
    }

    void rateIsClamped()
    {
        DemoTransport demo;
        demo.start(100000);
        QCOMPARE(demo.rate(), 1000);
        demo.start(0);
        QCOMPARE(demo.rate(), 1);
    }
};

QTEST_GUILESS_MAIN(TestDemoTransport)
#include "tst_demotransport.moc"
