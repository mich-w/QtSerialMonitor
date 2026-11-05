#ifndef NETWORKUDP_H
#define NETWORKUDP_H

#include "transport.h"

#include <QObject>
#include <QUdpSocket>

class NetworkUDP : public Transport
{
    Q_OBJECT
public:
    explicit NetworkUDP(QObject *parent = nullptr);
    bool begin(QHostAddress ip, quint16 port);
    bool bind(QHostAddress ip, quint16 port);
    bool end();
    bool isOpen() override;
    void write(QString message, QHostAddress ip, quint16 port);
signals:

public slots:
private slots:
    void readDatagram();

private:
    bool isBinded = false;
    QUdpSocket *udpSocket;
};

#endif // NETWORKUDP_H
