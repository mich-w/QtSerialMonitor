#include "networkudp.h"

NetworkUDP::NetworkUDP(QObject *parent) : QObject(parent)
{
    udpSocket = new QUdpSocket(this);
}

bool NetworkUDP::begin(QHostAddress ip, qint16 port)
{
    bind(ip, port);

    connect(this->udpSocket, SIGNAL(readyRead()), this, SLOT(readDatagram()));

    return true;
}

bool NetworkUDP::end()
{
    disconnect(this->udpSocket, SIGNAL(readyRead()), this, SLOT(readDatagram()));

    udpSocket->flush();
    udpSocket->close();
    isBinded = false;

    if (!udpSocket->isOpen()) // TODO ZAWSZE FALSE
        return true;
    else
        return false;
}

bool NetworkUDP::bind(QHostAddress ip, qint16 port)
{
    udpSocket->flush();
    udpSocket->close();

    if (ip == QHostAddress::Any)
        udpSocket->bind(ip, port, QUdpSocket::BindFlag::ShareAddress);
    else
        udpSocket->bind(ip, port);

    isBinded = true;
    return true;
}

void NetworkUDP::write(QString message, QHostAddress ip, qint16 port)
{
    auto datagram = message.toLatin1();
    udpSocket->writeDatagram(datagram, ip, port);
}

void NetworkUDP::readDatagram()
{
    QByteArray datagram;

    while (udpSocket->hasPendingDatagrams())
    {
        datagram.resize(int(udpSocket->pendingDatagramSize()));
        udpSocket->readDatagram(datagram.data(), datagram.size());
        udpReceiveStringBuffer.append(datagram.toStdString().c_str());
    }
}

QString NetworkUDP::readString(bool clearBuffer)
{
    QString output = udpReceiveStringBuffer;

    if (clearBuffer)
        udpReceiveStringBuffer.clear();

    return output;
}

void NetworkUDP::clearStringBuffer()
{
    udpReceiveStringBuffer.clear();
}

void NetworkUDP::clearAll() { udpReceiveStringBuffer.clear(); }

bool NetworkUDP::isOpen() { return isBinded; }
