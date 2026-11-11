#ifndef PORTIDENTITY_H
#define PORTIDENTITY_H

#include <QList>
#include <QMetaType>
#include <QSerialPortInfo>
#include <QString>

// What we remember about a serial device to find it again after it was unplugged.
struct PortIdentity
{
    QString portName;
    QString serialNumber;
    QString description;
    quint16 vendorId = 0;
    quint16 productId = 0;
    bool hasUsbIds = false;

    static PortIdentity fromInfo(const QSerialPortInfo &info);
    static QList<PortIdentity> available();

    // Name of the port in `candidates` that is most likely the same device, or empty.
    // Preference: same USB serial number > same VID:PID (+description) > same port name.
    static QString findMatch(const PortIdentity &wanted, const QList<PortIdentity> &candidates);
};

Q_DECLARE_METATYPE(PortIdentity)

#endif // PORTIDENTITY_H
