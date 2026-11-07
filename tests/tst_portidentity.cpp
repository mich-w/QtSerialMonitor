#include "portidentity.h"

#include <QtTest>

class TestPortIdentity : public QObject
{
    Q_OBJECT

    static PortIdentity port(const QString &name, const QString &serial = {}, quint16 vid = 0, quint16 pid = 0, const QString &description = {})
    {
        PortIdentity identity;
        identity.portName = name;
        identity.serialNumber = serial;
        identity.vendorId = vid;
        identity.productId = pid;
        identity.hasUsbIds = vid != 0;
        identity.description = description;
        return identity;
    }

private slots:
    void serialNumberSurvivesRename()
    {
        const auto wanted = port("ttyUSB0", "A1B2C3", 0x0403, 0x6001);
        const QList<PortIdentity> now = {port("ttyS0"), port("ttyUSB1", "A1B2C3", 0x0403, 0x6001)};
        QCOMPARE(PortIdentity::findMatch(wanted, now), QString("ttyUSB1"));
    }

    void differentBoardOfSameModelIsNotMatched()
    {
        const auto wanted = port("ttyUSB0", "A1B2C3", 0x0403, 0x6001);
        const QList<PortIdentity> now = {port("ttyUSB0", "ZZZZZZ", 0x0403, 0x6001)};
        QCOMPARE(PortIdentity::findMatch(wanted, now), QString());
    }

    void sameModelWithoutSerialPrefersOldName()
    {
        const auto wanted = port("ttyACM1", {}, 0x2341, 0x0043, "Arduino Uno");
        const QList<PortIdentity> now = {port("ttyACM0", {}, 0x2341, 0x0043, "Arduino Uno"), port("ttyACM1", {}, 0x2341, 0x0043, "Arduino Uno")};
        QCOMPARE(PortIdentity::findMatch(wanted, now), QString("ttyACM1"));
    }

    void sameModelWithoutSerialUnderNewName()
    {
        const auto wanted = port("COM3", {}, 0x2341, 0x0043, "Arduino Uno");
        const QList<PortIdentity> now = {port("COM1"), port("COM7", {}, 0x2341, 0x0043, "Arduino Uno")};
        QCOMPARE(PortIdentity::findMatch(wanted, now), QString("COM7"));
    }

    void otherUsbDeviceOnSameNameIsNotMatched()
    {
        const auto wanted = port("ttyUSB0", {}, 0x0403, 0x6001);
        const QList<PortIdentity> now = {port("ttyUSB0", {}, 0x10c4, 0xea60)};
        QCOMPARE(PortIdentity::findMatch(wanted, now), QString());
    }

    void plainUartMatchedByName()
    {
        QCOMPARE(PortIdentity::findMatch(port("ttyS1"), {port("ttyS0"), port("ttyS1")}), QString("ttyS1"));
        QCOMPARE(PortIdentity::findMatch(port("ttyS2"), {port("ttyS0")}), QString());
    }
};

QTEST_GUILESS_MAIN(TestPortIdentity)
#include "tst_portidentity.moc"
