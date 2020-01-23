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
        logFile->open(QIODevice::OpenModeFlag::WriteOnly | QIODevice::Text);
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
            openFile(path + "/" + fileName);

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
    QString newCSVLine;

    if (labelList.count() == 0)
        return;

    if(csvLabelsBuffer.count() == 0)
    {
        csvLabelsBuffer.append(labelList[0]);
        out.readLine(0);
        out << "\"" + labelList[0].trimmed() + "\",";
        out << "\r";
    }
    else
    {
        bool added = false;
        for (auto i = 0; i < labelList.count(); ++i)
        {
            if (!csvLabelsBuffer.contains(labelList[i]))
            {
                csvLabelsBuffer.append(labelList[i]);

                out.readLine(0);
                out << "\"" + labelList[i].trimmed() + "\",";
                added = true;
            }
        }

        if (added)
            out << "\r";

    }

    for (auto i = 0; i < csvLabelsBuffer.count(); ++i)
    {
        out.atEnd();

        int index = labelList.indexOf(csvLabelsBuffer[i]);
        if (index >= 0 && index < dataList.count())
            out << QString::number(dataList[index]) + ',';
    }

    out << "\r";

    //    for (auto i = 0; i < labelsInFile.count(); ++i)
    //    {



    //        for (auto i = 0; i < labelsInFile.count(); ++i)
    //        {
    //            if(!labelsInFile.contains(labelList[i]))
    //            {
    //                out.readLine(0);
    //                out << labelList[i] + ',';


    //            }
    //            else
    //            {
    //                //logFile->atEnd();
    //                out.atEnd();

    //                out << dataList[i] + ',';
    //            }
    //        }

    //        out << "\r";
    //    }


    //    for (auto i = 0; i < labelList.count(); ++i)
    //    {
    //        text.append(labelList[i] + " " + QString::number(dataList[i], 'f') + " :: ");
    //    }

    //    text = text.simplified();

    //    if (appendDate)
    //        text = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz ") + text;
    //    else
    //        text = QDateTime::currentDateTime().toString("hh:mm:ss:zzz ") + text;



    //    out.setCodec("UTF-8");
    //    out << text + "\r";
}
