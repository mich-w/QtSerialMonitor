#include "logger.h"

namespace
{
    const char *const timeColumnLabel = "time";

    QString quoted(const QString &text) { return "\"" + QString(text).replace("\"", "\"\"") + "\""; }
}

Logger::Logger(QObject *parent) : QObject(parent)
{
}

Logger::~Logger()
{
    closeFile();
}

QString Logger::timeStamp()
{
    return QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH:mm:ss.zzzZ");
}

bool Logger::openFile(QString fileName, bool trunc)
{
    closeFile();

    if (fileName.isEmpty())
        return false;

    logFile = new QFile(fileName);
    if (!logFile->open(QFile::ReadWrite))
    {
        qDebug() << "Unable to open log file" << fileName << logFile->errorString();
        delete logFile;
        logFile = nullptr;
        return false;
    }

    if (trunc)
        logFile->resize(0);

    return true;
}

void Logger::closeFile()
{
    if (logFile != nullptr)
    {
        logFile->close();
        delete logFile;
        logFile = nullptr;
    }

    clearWriteBuffer();
}

bool Logger::beginLog(QString path, bool autoLogging, QString fileName, bool truncateFile)
{
    if (logFile != nullptr)
        return true; // already logging

    if (!QDir().mkpath(path))
    {
        qDebug() << "Invalid log dir" << path;
        return false;
    }

    QString fullPath;
    if (autoLogging)
    {
        const int dot = fileName.lastIndexOf('.');
        const QString extension = dot >= 0 ? fileName.mid(dot) : QString(".txt");
        fullPath = path + "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd_hh.mm.ss.zzz_") + "Log" + extension;
    }
    else
    {
        fullPath = path + "/" + fileName;
    }

    if (!openFile(fullPath, truncateFile))
        return false;

    // Appending to an existing CSV - reuse its header so columns stay aligned.
    if (fullPath.endsWith(".csv", Qt::CaseInsensitive) && logFile->size() > 0)
    {
        logFile->seek(0);
        const QString header = QString::fromUtf8(logFile->readLine()).trimmed();
        for (QString label : header.split(',', Qt::SkipEmptyParts))
        {
            label = label.trimmed();
            if (label.startsWith('"') && label.endsWith('"') && label.length() >= 2)
                label = label.mid(1, label.length() - 2).replace("\"\"", "\"");
            csvLabelsBuffer.append(label);
        }
    }

    logFile->seek(logFile->size()); // append, never overwrite existing content
    return true;
}

bool Logger::isOpen()
{
    return logFile != nullptr;
}

void Logger::writeLine(const QString &line)
{
    if (logFile == nullptr)
        return;

    logFile->seek(logFile->size());
    logFile->write((line + "\n").toUtf8());
    logFile->flush();
}

void Logger::writeLogTXTLine(QString lineToAppend, bool simplifyText)
{
    QString text = simplifyText ? lineToAppend.simplified() : lineToAppend;

    // Keep one timestamp per received line, even for multi-line chunks.
    const QString stamp = timeStamp() + " ";
    const QStringList lines = text.split(QRegularExpression("[\\r\\n]+"), Qt::SkipEmptyParts);
    for (const auto &line : lines)
        writeLine(stamp + line);
}

void Logger::writeLogTXTParsedData(QStringList labelList, QList<double> dataList)
{
    QStringList parts;
    const auto count = qMin(labelList.count(), dataList.count());
    for (auto i = 0; i < count; ++i)
        parts.append(labelList[i] + " " + QString::number(dataList[i], 'g', 10));

    if (!parts.isEmpty())
        writeLine(timeStamp() + " " + parts.join(" :: "));
}

void Logger::rewriteCSVHeader()
{
    QStringList header;
    for (const auto &label : csvLabelsBuffer)
        header.append(quoted(label));

    logFile->seek(0);
    QByteArray content = logFile->readAll();

    // Drop the old header (first line), if the file has one.
    if (content.startsWith('"'))
    {
        const int newLine = content.indexOf('\n');
        content = newLine >= 0 ? content.mid(newLine + 1) : QByteArray();
    }

    logFile->resize(0);
    logFile->seek(0);
    logFile->write((header.join(',') + "\n").toUtf8());
    logFile->write(content);
    logFile->flush();
}

void Logger::writeLogCSV(QStringList labelList, QList<double> dataList, bool addTime)
{
    if (logFile == nullptr || labelList.isEmpty())
        return;

    bool headerChanged = false;

    if (addTime && !csvLabelsBuffer.contains(timeColumnLabel))
    {
        csvLabelsBuffer.insert(0, timeColumnLabel);
        headerChanged = true;
    }

    for (const auto &label : labelList)
    {
        if (!csvLabelsBuffer.contains(label))
        {
            csvLabelsBuffer.append(label);
            headerChanged = true;
        }
    }

    if (headerChanged)
        rewriteCSVHeader();

    // One field per column; missing values are left empty so columns stay aligned.
    QStringList row;
    for (const auto &column : csvLabelsBuffer)
    {
        const int index = labelList.lastIndexOf(column);
        if (index >= 0 && index < dataList.count())
            row.append(QString::number(dataList[index], 'g', 10));
        else if (column == timeColumnLabel && addTime)
            row.append(timeStamp());
        else
            row.append(QString());
    }

    writeLine(row.join(','));
}

void Logger::clearWriteBuffer()
{
    csvLabelsBuffer.clear();
}
