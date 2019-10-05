#ifndef NETWORKUDP_H
#define NETWORKUDP_H

#include <QObject>
#include <QUdpSocket>

class NetworkUDP : public QObject
{
    Q_OBJECT
public:
    explicit NetworkUDP(QObject *parent = nullptr);

    bool end();
    bool bind(QHostAddress ip, qint16 port);
    bool begin(QHostAddress ip, qint16 port);
    void write(QString message, QHostAddress ip, qint16 port);
    QString readString(bool clearBuffer = true);
    void clearBuffer();
    bool isOpen();
signals:

public slots:
private slots:
    void readDatagram();
private:
    bool isBinded = false;
    QUdpSocket *udpSocket;
    QString udpReceiveStringBuffer;
};

#endif // NETWORKUDP_H
