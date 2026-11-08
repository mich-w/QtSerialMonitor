#include "channelstats.h"

#include <QtTest>

class TestChannelStats : public QObject
{
    Q_OBJECT

private slots:
    void minMaxMeanCount()
    {
        ChannelStats stats;
        for (double value : {2.0, -1.0, 5.0, 2.0})
            stats.add(value, 0);

        QCOMPARE(stats.count, qint64(4));
        QCOMPARE(stats.min, -1.0);
        QCOMPARE(stats.max, 5.0);
        QCOMPARE(stats.mean, 2.0);
        QCOMPARE(stats.last, 2.0);
    }

    void rateFromTimeStamps()
    {
        ChannelStats stats;
        for (auto i = 0; i <= 50; ++i)
            stats.add(i, 10000 + i * 20); // every 20 ms
        QCOMPARE(stats.rateHz(), 50.0);
    }

    void rateUnknownWithoutTimeSpan()
    {
        ChannelStats stats;
        stats.add(1, 100);
        QCOMPARE(stats.rateHz(), 0.0);
        stats.add(2, 100);
        QCOMPARE(stats.rateHz(), 0.0);
    }

    void meanIsStableForLongRuns()
    {
        ChannelStats stats;
        for (auto i = 0; i < 1000000; ++i)
            stats.add(1e9 + (i % 2), i);
        QVERIFY(qAbs(stats.mean - (1e9 + 0.5)) < 1e-3);
    }
};

QTEST_GUILESS_MAIN(TestChannelStats)
#include "tst_channelstats.moc"
