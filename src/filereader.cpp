#include "filereader.h"

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

QList<QTime> FileReader::getFileTimeRange(QFile *file)
{
    QList<QTime> timeRange = {QTime(), QTime()};

    if (file->open(QIODevice::ReadOnly))
    {
        QString allData = file->readAll();
        file->close();

        QStringList readFileSplitLines = allData.split(QRegExp("[\\n+\\r+]"), QString::SplitBehavior::SkipEmptyParts);

        for (auto i = 0; i < readFileSplitLines.count(); ++i)
        {
            QStringList inputStringSplitArrayTopLine = readFileSplitLines[i].simplified().split(QRegExp("[\\s+,]"), QString::SplitBehavior::SkipEmptyParts);                                     // rozdzielamy traktująac spacje jako separator
            QStringList inputStringSplitArrayButtomLine = readFileSplitLines[readFileSplitLines.count() - 1 - i].simplified().split(QRegExp("[\\s+,]"), QString::SplitBehavior::SkipEmptyParts); // rozdzielamy traktująac spacje jako separator
            QStringList searchTimeFormatList = {"hh:mm:ss:zzz", "hh:mm:ss.zzz", "hh:mm:ss.z", "hh:mm:ss"};                                                                                       // TODO !!!

            bool foundTime[2] = {false};

            for (auto j = 0; j < inputStringSplitArrayTopLine.count(); ++j)
            {
                foreach (auto timeFormat, searchTimeFormatList)
                {
                    if (QTime::fromString(inputStringSplitArrayTopLine[j], timeFormat).isValid() && !foundTime[0])
                    {
                        timeRange.first() = QTime::fromString(inputStringSplitArrayTopLine[j], timeFormat);
                        foundTime[0] = true;
                        break;
                    }
                }
            }

            for (auto j = 0; j < inputStringSplitArrayButtomLine.count(); ++j)
            {
                foreach (auto timeFormat, searchTimeFormatList)
                {
                    if (QTime::fromString(inputStringSplitArrayButtomLine[j], timeFormat).isValid() && !foundTime[1])
                    {
                        timeRange.last() = (QTime::fromString(inputStringSplitArrayButtomLine[j], timeFormat));
                        foundTime[1] = true;

                        break;
                    }
                }
            }

            if (foundTime[0] && foundTime[1])
                return timeRange;
        }
    }
    else
    {
        qDebug() << "Get file time range - invalid file ?";
        return {QTime(), QTime()};
    }
}

void FileReader::setReadInterval(int newVal) { readInterval = newVal; }

void FileReader::abortRead() { fileReadTimer.stop(); }
