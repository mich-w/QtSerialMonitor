#include "serial.h"

Serial::Serial(QObject *parent) : QObject(parent)
{
    serialDevice = new QSerialPort(this);
}

Serial::~Serial()
{
    end();
}

void Serial::readString()
{
    switch (stringReadMode)
    {
    case canReadLine_ReadLine:
        while (this->serialDevice->canReadLine())
        {
            serialInputString.append(serialDevice->readLine());
        }
        break;
    case canReadLine_ReadAll:
        while (this->serialDevice->canReadLine())
        {
            serialInputString.append(serialDevice->readAll());
        }
        break;
    case bytesAvailable_ReadLine:
        while (this->serialDevice->bytesAvailable())
        {
            serialInputString.append(serialDevice->readLine());
        }
        break;
    case bytesAvailable_ReadAll:
        while (this->serialDevice->bytesAvailable())
        {
            serialInputString.append(serialDevice->readAll());
        }
        break;
    }
}

bool Serial::setReadMode(int mode)
{
    //    ui->comboBoxSerialReadMode->addItem("canReadLine | readLine");
    //    ui->comboBoxSerialReadMode->addItem("canReadLine | readAll");
    //    ui->comboBoxSerialReadMode->addItem("bytesAvailable | readLine");
    //    ui->comboBoxSerialReadMode->addItem("bytesAvailable | readAll");

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

QString Serial::getString(bool clearBuffer)
{
    QString output = serialInputString;

    if (clearBuffer)
        clearString();

    return output;
}

QByteArray Serial::getBytes(bool clearBuffer)
{
    QByteArray outputByteArray = serialByteArray;

    if (clearBuffer)
        clearBytes();

    return outputByteArray;
}

void Serial::clearAll(bool clearHardwareBuffers)
{
    serialInputString.clear();
    serialByteArray.clear();

    if (clearHardwareBuffers)
        serialDevice->clear();
}

void Serial::clearBytes()
{
    serialByteArray.clear();
}

void Serial::clearString()
{
    serialInputString.clear();
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
    case (QSerialPort::DataBits::UnknownDataBits):
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
    case (QSerialPort::Parity::UnknownParity):
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
    case (QSerialPort::StopBits::UnknownStopBits):
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
    case (QSerialPort::FlowControl::UnknownFlowControl):
        outputString.append("UnknownFlowControl, ");
        break;
    }

    // outputString.append("DTR: " + QSerialPort::DataTerminalReadySignal);

    return outputString;
}

bool Serial::isOpen()
{
    return serialDevice->isOpen();
}

int Serial::getAvailiblePortsCount()
{
    return QSerialPortInfo::availablePorts().count();
}

bool Serial::begin(QString parsedPortName, int parsedBaudRate, int dataBits, int parity, int stopBits, int flowControl, bool dtrOn)
{
    if (QSerialPortInfo::availablePorts().count() < 1)
    {
        // this->addLog(">>\t No devices available");
        return false;
    }

    this->serialDevice->setPortName(parsedPortName);

    if (!serialDevice->isOpen())
    {
        if (serialDevice->open(QSerialPort::ReadWrite))
        {
            this->serialDevice->clear();
            this->serialDevice->setBaudRate(parsedBaudRate);
            this->serialDevice->setDataBits((QSerialPort::DataBits)dataBits);
            this->serialDevice->setParity((QSerialPort::Parity)parity);
            this->serialDevice->setStopBits((QSerialPort::StopBits)stopBits);
            this->serialDevice->setFlowControl((QSerialPort::FlowControl)flowControl);
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
    if (this->serialDevice->isOpen() && this->serialDevice->isWritable())
    {
        this->serialDevice->write(message.toStdString().c_str());
        return true;
    }
    else
    {
        return false;
    }
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
