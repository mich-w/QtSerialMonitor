#include "filereader.h"

#include <QDateTime>
#include <QRegularExpression>

FileReader::FileReader(QObject *parent) : QObject(parent)
{
    fileReadTimer.stop();
}

void FileReader::readLineSendLine()
{
    int progressPercent = lineReadIterator * 100 / readFileSplitLines.count();
    emit lineReady(&readFileSplitLines[lineReadIterator], &progressPercent);
    lineReadIterator++;

    if (lineReadIterator >= readFileSplitLines.count())
    {
        emit fileReadFinished();

        fileReadTimer.stop();
        lineReadIterator = 0;
    }
}

bool FileReader::readAllAtOnce(QFile *fileToRead)
{
    if (fileToRead->open(QIODevice::ReadOnly))
    {
        QTextStream stream(fileToRead);
        QString allData = stream.readAll();
        fileToRead->close();

        emit textReady(&allData);
        emit fileReadFinished();

        return true;
    }
    return false;
}

static QTime findTimeInLine(const QString &line)
{
    static const QStringList searchTimeFormatList = {"hh:mm:ss:zzz", "hh:mm:ss.zzz", "hh:mm:ss.z", "hh:mm:ss"};
    static const QRegularExpression tokenSeparators("[\\s,]+");

    const QStringList tokens = line.split(tokenSeparators, Qt::SkipEmptyParts);
    for (const QString &token : tokens)
    {
        if (token.contains('T')) // ISO 8601 date-time, as written by Logger
        {
            QDateTime dateTime = QDateTime::fromString(token, Qt::ISODateWithMs);
            if (dateTime.isValid())
                return dateTime.time();
        }

        for (const QString &timeFormat : searchTimeFormatList)
        {
            QTime time = QTime::fromString(token, timeFormat);
            if (time.isValid())
                return time;
        }
    }
    return QTime();
}

// Scans only the head and tail of the file, so it stays fast for large logs.
QList<QTime> FileReader::getFileTimeRange(QFile *file)
{
    static const qint64 scanChunkSize = 64 * 1024;
    static const QRegularExpression lineSeparators("[\\r\\n]+");

    QList<QTime> timeRange = {QTime(), QTime()};

    if (!file->open(QIODevice::ReadOnly))
    {
        qDebug() << "Get file time range - invalid file ?";
        return timeRange;
    }

    const QStringList headLines = QString::fromUtf8(file->read(scanChunkSize)).split(lineSeparators, Qt::SkipEmptyParts);

    file->seek(qMax<qint64>(0, file->size() - scanChunkSize));
    const QStringList tailLines = QString::fromUtf8(file->readAll()).split(lineSeparators, Qt::SkipEmptyParts);
    file->close();

    for (const QString &line : headLines)
    {
        timeRange.first() = findTimeInLine(line);
        if (timeRange.first().isValid())
            break;
    }

    for (auto i = tailLines.count() - 1; i >= 0; --i)
    {
        timeRange.last() = findTimeInLine(tailLines[i]);
        if (timeRange.last().isValid())
            break;
    }

    return timeRange;
}

void FileReader::setReadInterval(int newVal) { readInterval = newVal; }

void FileReader::abortRead() { fileReadTimer.stop(); }
