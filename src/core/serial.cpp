#include "serial.h"

Serial::Serial(QObject *parent) : Transport(parent)
{
    serialDevice = new QSerialPort(this);
    connect(serialDevice, &QSerialPort::errorOccurred, this, &Serial::handleError);
}

void Serial::handleError(QSerialPort::SerialPortError error)
{
    // ResourceError is raised e.g. when a USB device is unplugged while open.
    if (error == QSerialPort::ResourceError || error == QSerialPort::PermissionError)
    {
        if (serialDevice->isOpen())
            emit connectionLost(serialDevice->errorString());
    }
}

Serial::~Serial()
{
    end();
}

void Serial::readString()
{
    QByteArray chunk;

    switch (stringReadMode)
    {
    case canReadLine_ReadLine:
        while (serialDevice->canReadLine())
            chunk.append(serialDevice->readLine());
        break;
    case canReadLine_ReadAll:
        if (serialDevice->canReadLine())
            chunk.append(serialDevice->readAll());
        break;
    case bytesAvailable_ReadLine:
        while (serialDevice->bytesAvailable())
            chunk.append(serialDevice->readLine());
        break;
    case bytesAvailable_ReadAll:
        chunk.append(serialDevice->readAll());
        break;
    }

    if (!chunk.isEmpty())
        emit dataReceived(chunk, false);
}

bool Serial::setReadMode(int mode)
{
    // Order matches the "Read mode" combo box in the UI.
    switch (mode)
    {
    case 0:
        stringReadMode = canReadLine_ReadLine;
        break;
    case 1:
        stringReadMode = canReadLine_ReadAll;
        break;
    case 2:
        stringReadMode = bytesAvailable_ReadLine;
        break;
    case 3:
        stringReadMode = bytesAvailable_ReadAll;
        break;
    default:
        return false;
    }
    return true;
}

QString Serial::getSerialInfo()
{
    QString outputString;
    outputString.append(serialDevice->portName() + ", " + QString::number(serialDevice->baudRate()) + ", ");

    switch (serialDevice->dataBits())
    {
    case (QSerialPort::DataBits::Data5):
        outputString.append("Data5, ");
        break;
    case (QSerialPort::DataBits::Data6):
        outputString.append("Data6, ");
        break;
    case (QSerialPort::DataBits::Data7):
        outputString.append("Data7, ");
        break;
    case (QSerialPort::DataBits::Data8):
        outputString.append("Data8, ");
        break;
    default: // Unknown* values were removed in Qt 6
        outputString.append("UnknownDataBits, ");
        break;
    }

    switch (serialDevice->parity())
    {
    case (QSerialPort::Parity::NoParity):
        outputString.append("NoParity, ");
        break;
    case (QSerialPort::Parity::OddParity):
        outputString.append("OddParity, ");
        break;
    case (QSerialPort::Parity::EvenParity):
        outputString.append("EvenParity, ");
        break;
    case (QSerialPort::Parity::MarkParity):
        outputString.append("MarkParity, ");
        break;
    case (QSerialPort::Parity::SpaceParity):
        outputString.append("SpaceParity, ");
        break;
    default: // Unknown* values were removed in Qt 6
        outputString.append("UnknownParity, ");
        break;
    }

    switch (serialDevice->stopBits())
    {
    case (QSerialPort::StopBits::OneStop):
        outputString.append("OneStop, ");
        break;
    case (QSerialPort::StopBits::TwoStop):
        outputString.append("TwoStop, ");
        break;
    case (QSerialPort::StopBits::OneAndHalfStop):
        outputString.append("OneAndHalfStop, ");
        break;
    default: // Unknown* values were removed in Qt 6
        outputString.append("UnknownStopBits, ");
        break;
    }

    switch (serialDevice->flowControl())
    {
    case (QSerialPort::FlowControl::NoFlowControl):
        outputString.append("NoFlowControl, ");
        break;
    case (QSerialPort::FlowControl::HardwareControl):
        outputString.append("HardwareControl, ");
        break;
    case (QSerialPort::FlowControl::SoftwareControl):
        outputString.append("SoftwareControl, ");
        break;
    default: // Unknown* values were removed in Qt 6
        outputString.append("UnknownFlowControl, ");
        break;
    }


    return outputString;
}

bool Serial::isOpen()
{
    return serialDevice->isOpen();
}

QList<int> Serial::getAvailibleBaudRates()
{
    return QSerialPortInfo::standardBaudRates();
}

bool Serial::begin(QString parsedPortName, qint32 parsedBaudRate, QString dataBits, QString parity, QString stopBits, QString flowControl, bool dtrOn)
{
    if (QSerialPortInfo::availablePorts().count() < 1)
        return false;

    this->serialDevice->setPortName(parsedPortName);

    if (!serialDevice->isOpen())
    {
        if (serialDevice->open(QSerialPort::ReadWrite))
        {
            this->serialDevice->clear();
            this->serialDevice->setBaudRate(parsedBaudRate);

            if (dataBits.contains("Data5"))
                this->serialDevice->setDataBits(QSerialPort::DataBits::Data5);
            else if (dataBits.contains("Data6"))
                this->serialDevice->setDataBits(QSerialPort::DataBits::Data6);
            else if (dataBits.contains("Data7"))
                this->serialDevice->setDataBits(QSerialPort::DataBits::Data7);
            else if (dataBits.contains("Data8"))
                this->serialDevice->setDataBits(QSerialPort::DataBits::Data8);

            if (parity.contains("NoParity"))
                this->serialDevice->setParity(QSerialPort::Parity::NoParity);
            else if (parity.contains("EvenParity"))
                this->serialDevice->setParity(QSerialPort::Parity::EvenParity);
            else if (parity.contains("OddParity"))
                this->serialDevice->setParity(QSerialPort::Parity::OddParity);
            else if (parity.contains("SpaceParity"))
                this->serialDevice->setParity(QSerialPort::Parity::SpaceParity);
            else if (parity.contains("MarkParity"))
                this->serialDevice->setParity(QSerialPort::Parity::MarkParity);

            if (stopBits.contains("OneStop"))
                this->serialDevice->setStopBits(QSerialPort::StopBits::OneStop);
            else if (stopBits.contains("OneAndHalfStop"))
                this->serialDevice->setStopBits(QSerialPort::StopBits::OneAndHalfStop);
            else if (stopBits.contains("TwoStop"))
                this->serialDevice->setStopBits(QSerialPort::StopBits::TwoStop);

            if (flowControl.contains("NoFlowControl"))
                this->serialDevice->setFlowControl(QSerialPort::FlowControl::NoFlowControl);
            else if (flowControl.contains("HardwareControl"))
                this->serialDevice->setFlowControl(QSerialPort::FlowControl::HardwareControl);
            else if (flowControl.contains("SoftwareControl"))
                this->serialDevice->setFlowControl(QSerialPort::FlowControl::SoftwareControl);

            this->serialDevice->setDataTerminalReady(dtrOn);

            //  I have identified that the problem is due to the EDBG chip (usb-serial bridge) which requires DTR signal to enable the serial port RXD and TXD pins.
            //  Including the line of code "serial->setDataTerminalReady(true);" after opening the serial port, the serial port application is now working fine. I guess this is automatically carried out on hyper-terminal software and tera-term software.
            //  Whether the communication uses the flow control or not,some of the hardware bridge requires the flow control signals to be set for the first time after power ON.

            connect(this->serialDevice, SIGNAL(readyRead()), this, SLOT(readString()));

            return true;
        }
        return false;
    }
    else
    {
        return false;
    }
}

bool Serial::end()
{
    disconnect(serialDevice, SIGNAL(readyRead()), this, SLOT(readString()));

    serialDevice->clear();
    serialDevice->close();
    if (!this->serialDevice->isOpen())
        return true;
    else
        return false;
}

bool Serial::send(QString message)
{
    return send(message.toUtf8());
}

bool Serial::send(const QByteArray &message)
{
    if (this->serialDevice->isOpen() && this->serialDevice->isWritable())
    {
        this->serialDevice->write(message);
        return true;
    }
    else
    {
        return false;
    }
}
