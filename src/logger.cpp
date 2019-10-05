#include "logger.h"

Logger::Logger(QObject *parent) : QObject(parent)
{

}

void Logger::openFile(QString fileName)
{
    if (!fileName.isEmpty())
    {
        logFile = new QFile;
        logFile->setFileName(fileName);
        logFile->open(QIODevice::OpenModeFlag::Append | QIODevice::Text);
    }
}

void Logger::closeFile()
{
    if (logFile != nullptr)
    {
        logFile->close();
        delete logFile;
        logFile = nullptr;

        qDebug() << "Close File";
    }
}

bool Logger::beginLog(QString path, bool autoLogging, QString fileName)
{
    if (logFile == nullptr && QDir(path).isReadable())
    {
        qDebug() << "File open";

        if (autoLogging)
            openFile(path + "/" + QDateTime::currentDateTime().toString("dd.MM.yyyy_hh.mm.ss.zzz_") + "Log.txt");
        else
            openFile(path +"/" + fileName);

        return true;
    }
    qDebug() << "Invalid Dir";
    return false;
}

bool Logger::isOpen()
{
    return logFile != nullptr;
}

void Logger::writeLogLine(QString lineToAppend, bool simplifyText, bool appendDate)
{
    QString text = lineToAppend; // + "";

    if (simplifyText)
        text = text.simplified();

    if (appendDate)
        text = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz ") + text;
    else
        text = QDateTime::currentDateTime().toString("hh:mm:ss:zzz ") + text;

    if (logFile != nullptr)
    {
        QTextStream out(logFile);
        out.setCodec("UTF-8");

        out << text + "\r";
    }
}

void Logger::writeLogParsedData(QStringList labelList, QList<double> dataList, bool appendDate)
{
    QString text;

    for (auto i = 0; i< labelList.count(); ++i)
    {
        text.append(labelList[i] + " " + QString::number(dataList[i], 'f') + " :: ");
    }

    text = text.simplified();

    if (appendDate)
        text = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz ") + text;
    else
        text = QDateTime::currentDateTime().toString("hh:mm:ss:zzz ") + text;

    if (logFile != nullptr)
    {
        QTextStream out(logFile);
        out.setCodec("UTF-8");

        out << text + "\r";
    }
}

