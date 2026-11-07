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

        QVERIFY2(spy.count() >= 15 && spy.count() <= 40, qPrintable(QString::number(spy.count())));
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
