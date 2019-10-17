#ifndef NETWORKUDP_H
#define NETWORKUDP_H

#include <QObject>
#include <QUdpSocket>

class NetworkUDP : public QObject
{
    Q_OBJECT
public:
    explicit NetworkUDP(QObject *parent = nullptr);
    bool begin(QHostAddress ip, qint16 port);
    bool bind(QHostAddress ip, qint16 port);
    bool end();
    bool isOpen();
    QString readString(bool clearBuffer = true);
    void clearAll();
    void clearStringBuffer();
    void write(QString message, QHostAddress ip, qint16 port);
signals:

public slots:
private slots:
    void readDatagram();

private:
    bool isBinded = false;
    QString udpReceiveStringBuffer;
    QUdpSocket *udpSocket;
};

#endif // NETWORKUDP_H
