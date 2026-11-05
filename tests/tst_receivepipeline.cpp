#include "receivepipeline.h"

#include <QSignalSpy>
#include <QtTest>

class TestReceivePipeline : public QObject
{
    Q_OBJECT

    static QStringList lines(const QSignalSpy &spy)
    {
        QStringList result;
        for (const auto &args : spy)
            result.append(args.at(0).toString());
        return result;
    }

private slots:
    void partialLinesAreHeldBack()
    {
        ReceivePipeline pipeline;
        pipeline.setPartialLineTimeout(10000);
        QSignalSpy rawSpy(&pipeline, &ReceivePipeline::received);
        QSignalSpy lineSpy(&pipeline, &ReceivePipeline::linesReceived);

        pipeline.append("Temp 23");
        pipeline.flush();
        QCOMPARE(rawSpy.count(), 1); // display gets data immediately
        QCOMPARE(rawSpy.at(0).at(1).toString(), QString("Temp 23"));
        QCOMPARE(lineSpy.count(), 0); // parser does not see half a number

        pipeline.append(".5\nHum 4");
        pipeline.flush();
        QCOMPARE(lines(lineSpy), QStringList{"Temp 23.5\n"});
    }

    void partialLineReleasedAfterIdle()
    {
        ReceivePipeline pipeline;
        pipeline.setPartialLineTimeout(20);
        QSignalSpy lineSpy(&pipeline, &ReceivePipeline::linesReceived);

        pipeline.append("no newline 1");
        pipeline.flush();
        QVERIFY(lineSpy.wait(1000));
        QCOMPARE(lines(lineSpy), QStringList{"no newline 1"});
    }

    void completeMessagesAreSeparated()
    {
        ReceivePipeline pipeline;
        QSignalSpy lineSpy(&pipeline, &ReceivePipeline::linesReceived);

        pipeline.append("a 1", true); // e.g. UDP datagrams without newline
        pipeline.append("a 2", true);
        pipeline.flush();
        QCOMPARE(lines(lineSpy), QStringList{"a 1\na 2\n"});
    }

    void batchesUntilTimer()
    {
        ReceivePipeline pipeline;
        pipeline.setBatchInterval(20);
        QSignalSpy rawSpy(&pipeline, &ReceivePipeline::received);

        pipeline.append("x 1\n");
        pipeline.append("x 2\n");
        QCOMPARE(rawSpy.count(), 0);
        QVERIFY(rawSpy.wait(1000));
        QCOMPARE(rawSpy.count(), 1);
        QCOMPARE(rawSpy.at(0).at(0).toByteArray(), QByteArray("x 1\nx 2\n"));
    }

    void rawBytesPreserved()
    {
        ReceivePipeline pipeline;
        QSignalSpy rawSpy(&pipeline, &ReceivePipeline::received);

        pipeline.append(QByteArray("A\0\xff", 3));
        pipeline.flush();
        QCOMPARE(rawSpy.at(0).at(0).toByteArray(), QByteArray("A\0\xff", 3));
    }

    void resetDropsPendingData()
    {
        ReceivePipeline pipeline;
        QSignalSpy lineSpy(&pipeline, &ReceivePipeline::linesReceived);

        pipeline.append("stale");
        pipeline.reset();
        pipeline.append("fresh 1\n");
        pipeline.flush();
        QCOMPARE(lines(lineSpy), QStringList{"fresh 1\n"});
    }
};

QTEST_GUILESS_MAIN(TestReceivePipeline)
#include "tst_receivepipeline.moc"
