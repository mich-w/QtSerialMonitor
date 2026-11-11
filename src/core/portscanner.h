#ifndef PORTSCANNER_H
#define PORTSCANNER_H

#include "portidentity.h"

#include <QObject>
#include <QPointer>
#include <QThread>

// Enumerates serial ports on a worker thread. QSerialPortInfo::availablePorts() takes
// ~15 ms on Linux (often more on Windows) - too long for the GUI thread twice a second.
class PortScanner : public QObject
{
    Q_OBJECT
public:
    explicit PortScanner(QObject *parent = nullptr);
    ~PortScanner() override;

    void scan(); // no-op while a scan is still running
    bool isScanning() const;

signals:
    void scanned(const QList<PortIdentity> &ports);

private:
    QPointer<QThread> worker;
};

#endif // PORTSCANNER_H
