#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <QByteArray>
#include <QObject>
#include <QString>

// Common interface of a byte source/sink (serial port, UDP socket, ...).
// Data is pushed through dataReceived() as soon as it arrives.
class Transport : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    ~Transport() override = default;

    virtual bool isOpen() = 0;

signals:
    // isCompleteMessage: the chunk is a whole message (e.g. a UDP datagram),
    // so a line ends here even without a trailing newline.
    void dataReceived(const QByteArray &bytes, bool isCompleteMessage);

    // The connection was lost unexpectedly (device unplugged, socket error, ...).
    void connectionLost(const QString &reason);
};

#endif // TRANSPORT_H
