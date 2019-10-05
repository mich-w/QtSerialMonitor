#ifndef SERIAL_H
#define SERIAL_H

#include <QObject>
#include <QSerialPortInfo>
#include <QSerialPort>

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
    QString getSerialInfo();
    void processSerial();
    QString getString(bool clearBuffer = true);
    bool send(const QByteArray &message);
    bool send(QString message);
    bool end();
    bool begin(QString parsedPortName, qint32 parsedBaudRate, QString dataBits, QString parity, QString stopBits, QString flowControl, bool dtrOn);
    bool begin(QString parsedPortName, int parsedBaudRate, int dataBits, int parity, int stopBits, int flowControl, bool dtrOn);
    bool setReadMode(int mode);
    bool isOpen();
    void clear(bool clearHardwareBuffers = false);
    int getAvailiblePortsCount();
    ~Serial();
signals:

public slots:
    void readString();
private:
    QSerialPort *serialDevice = nullptr;
    QString serialInputString;
    SERIAL_READ_MODE stringReadMode = canReadLine_ReadLine;
};



#endif // SERIAL_H
