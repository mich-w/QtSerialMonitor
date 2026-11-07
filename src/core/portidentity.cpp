#include "portidentity.h"

PortIdentity PortIdentity::fromInfo(const QSerialPortInfo &info)
{
    PortIdentity identity;
    identity.portName = info.portName();
    identity.serialNumber = info.serialNumber();
    identity.description = info.description();
    identity.hasUsbIds = info.hasVendorIdentifier() && info.hasProductIdentifier();
    identity.vendorId = info.vendorIdentifier();
    identity.productId = info.productIdentifier();
    return identity;
}

QList<PortIdentity> PortIdentity::available()
{
    QList<PortIdentity> result;
    for (const auto &info : QSerialPortInfo::availablePorts())
        result.append(fromInfo(info));
    return result;
}

QString PortIdentity::findMatch(const PortIdentity &wanted, const QList<PortIdentity> &candidates)
{
    // 1. Unique USB serial number - survives a change of port name (ttyUSB0 -> ttyUSB1, COM3 -> COM5).
    if (!wanted.serialNumber.isEmpty())
    {
        for (const auto &candidate : candidates)
            if (candidate.serialNumber == wanted.serialNumber)
                return candidate.portName;
    }

    // 2. Same USB model; prefer the old port name, then the same description.
    if (wanted.hasUsbIds)
    {
        QString sameModel;
        for (const auto &candidate : candidates)
        {
            if (!candidate.hasUsbIds || candidate.vendorId != wanted.vendorId || candidate.productId != wanted.productId)
                continue;
            if (!wanted.serialNumber.isEmpty() && !candidate.serialNumber.isEmpty())
                continue; // both have serial numbers and they differ - a different board of the same model
            if (candidate.portName == wanted.portName)
                return candidate.portName;
            if (sameModel.isEmpty() && candidate.description == wanted.description)
                sameModel = candidate.portName;
        }
        if (!sameModel.isEmpty())
            return sameModel;
        return QString();
    }

    // 3. No USB information (e.g. a built-in UART) - only the port name is meaningful.
    for (const auto &candidate : candidates)
        if (candidate.portName == wanted.portName)
            return candidate.portName;

    return QString();
}
