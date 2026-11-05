#include "networkudp.h"

NetworkUDP::NetworkUDP(QObject *parent) : Transport(parent)
{
    udpSocket = new QUdpSocket(this);
}

bool NetworkUDP::begin(QHostAddress ip, quint16 port)
{
    if (!bind(ip, port))
        return false;

    connect(this->udpSocket, &QUdpSocket::readyRead, this, &NetworkUDP::readDatagram, Qt::UniqueConnection);

    return true;
}

bool NetworkUDP::end()
{
    disconnect(this->udpSocket, &QUdpSocket::readyRead, this, &NetworkUDP::readDatagram);

    udpSocket->flush();
    udpSocket->close();
    isBinded = false;

    return !udpSocket->isOpen();
}

bool NetworkUDP::bind(QHostAddress ip, quint16 port)
{
    udpSocket->flush();
    udpSocket->close();

    if (ip == QHostAddress::Any)
        isBinded = udpSocket->bind(ip, port, QUdpSocket::BindFlag::ShareAddress);
    else
        isBinded = udpSocket->bind(ip, port);

    return isBinded;
}

void NetworkUDP::write(QString message, QHostAddress ip, quint16 port)
{
    auto datagram = message.toUtf8();
    udpSocket->writeDatagram(datagram, ip, port);
}

void NetworkUDP::readDatagram()
{
    QByteArray datagram;

    while (udpSocket->hasPendingDatagrams())
    {
        datagram.resize(int(udpSocket->pendingDatagramSize()));
        udpSocket->readDatagram(datagram.data(), datagram.size());
        emit dataReceived(datagram, true); // one datagram = one message; NUL bytes are valid data
    }
}

bool NetworkUDP::isOpen() { return isBinded; }
