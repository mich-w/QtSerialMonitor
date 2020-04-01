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

void FileReader::setReadInterval(int newVal)
{
    readInterval = newVal;
}

void FileReader::abortRead()
{
    fileReadTimer.stop(); // ...
}
