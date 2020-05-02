#ifndef SERIAL_H
#define SERIAL_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>

enum SERIAL_READ_MODE
{
    canReadLine_ReadLine = 0,
    canReadLine_ReadAll,
    bytesAvailable_ReadLine,
    bytesAvailable_ReadAll
};

class Serial : public QObject
{
    Q_OBJECT
public:
    explicit Serial(QObject *parent = nullptr);
    ~Serial();
    bool begin(QString parsedPortName, int parsedBaudRate, int dataBits, int parity, int stopBits, int flowControl, bool dtrOn);
    bool begin(QString parsedPortName, qint32 parsedBaudRate, QString dataBits, QString parity, QString stopBits, QString flowControl, bool dtrOn);
    bool end();
    bool isOpen();
    bool send(const QByteArray &message);
    bool send(QString message);
    bool setReadMode(int mode);
    int getAvailiblePortsCount();
    QList<QSerialPortInfo> getAvailiblePorts();
    QString getSerialInfo();
    QString getString(bool clearBuffer = true);
    void clearAll(bool clearHardwareBuffers = false);
    void clearString();
    QList<int> getAvailibleBaudRates();
signals:

public slots:
    void readString();

private:
    QSerialPort *serialDevice = nullptr;
    QString serialInputString;
    SERIAL_READ_MODE stringReadMode = canReadLine_ReadLine;
};

#endif // SERIAL_H
