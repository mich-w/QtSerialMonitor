#include "portscanner.h"

#include <QSignalSpy>
#include <QtTest>

class TestPortScanner : public QObject
{
    Q_OBJECT

private slots:
    void scansInBackground()
    {
        PortScanner scanner;
        QSignalSpy spy(&scanner, &PortScanner::scanned);

        scanner.scan();
        QVERIFY(scanner.isScanning());
        scanner.scan(); // ignored while busy

        QVERIFY(spy.wait(5000));
        QTest::qWait(100);
        QCOMPARE(spy.count(), 1);
        QVERIFY(!scanner.isScanning());

        const auto ports = spy.at(0).at(0).value<QList<PortIdentity>>();
        QCOMPARE(ports.count(), PortIdentity::available().count());
    }

    void destroyWhileScanning()
    {
        auto *scanner = new PortScanner;
        scanner->scan();
        delete scanner; // waits for the worker, must not crash
    }
};

QTEST_GUILESS_MAIN(TestPortScanner)
#include "tst_portscanner.moc"
