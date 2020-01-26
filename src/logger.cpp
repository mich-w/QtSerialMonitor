#include "logger.h"

Logger::Logger(QObject *parent) : QObject(parent)
{
}

void Logger::openFile(QString fileName, bool truncateFile)
{
    if (!fileName.isEmpty())
    {
        logFile = new QFile;
        logFile->setFileName(fileName);

        if (truncateFile)
            logFile->open(QIODevice::OpenModeFlag::Truncate | QFile::OpenModeFlag::ReadWrite | QIODevice::Text);
        else
            logFile->open(QIODevice::OpenModeFlag::ReadWrite | QIODevice::Text);
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

        clearWriteBuffer();
    }
}

bool Logger::beginLog(QString path, bool autoLogging, QString fileName, bool truncateFile)
{
    if (logFile == nullptr && QDir(path).isReadable())
    {
        qDebug() << "File open";

        if (autoLogging)
            openFile(path + "/" + QDateTime::currentDateTime().toString("dd.MM.yyyy_hh.mm.ss.zzz_") + "Log" + fileName.right(fileName.length() - fileName.lastIndexOf('.')),truncateFile);
        else
            openFile(path + "/" + fileName, truncateFile);

        return true;
    }
    else
    {
        qDebug() << "Invalid Dir";
        return false;
    }
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

    for (auto i = 0; i < labelList.count(); ++i)
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

void Logger::writeLogCSV(QStringList labelList, QList<double> dataList, bool appendDate)
{
    QTextStream out(logFile);

    if (labelList.count() == 0)
        return;

    bool canAdd = false;
    for (auto i = 0; i <= labelList.count() - 1; ++i)
    {
        if(csvLabelsBuffer.count() == 0 || !csvLabelsBuffer.contains(labelList[i]))
            canAdd = true;

        if (canAdd)
        {
            csvLabelsBuffer.append(labelList[i].toLatin1());

            QStringList origFile = out.readAll().split(QRegExp("\r\n"), Qt::SplitBehaviorFlags::SkipEmptyParts);
            logFile->resize(0);

            for (auto i = 0; i < csvLabelsBuffer.count(); ++i)
                out << "\"" + csvLabelsBuffer[i] + "\",";

            out << "\n";

            for (auto i = 1; i < origFile.count(); ++i) // Start from second line (data without first line which contains labels)
                out << origFile[i] + "\r";

            break;
        }
    }

    if (canAdd)
        out << "\r";


    for (auto i = 0; i < csvLabelsBuffer.count(); ++i)
    {
        out.atEnd(); // ???

        int index = labelList.indexOf(csvLabelsBuffer[i]);
        if (index >= 0 && index < dataList.count())
            out << QString::number(dataList[index]) + ',';
    }

    out << "\r";
}

void Logger::clearWriteBuffer()
{
    csvLabelsBuffer.clear();
}

