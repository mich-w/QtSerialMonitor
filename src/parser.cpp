#include "parser.h"

Parser::Parser(QObject *parent) : QObject(parent)
{
    parserClock = new QTime;
    parserClock->start();
    latestTimeStamp.setHMS(0, 0, 0, 0);
}

void Parser::parse(QString inputString, bool syncToSystemClock, bool useExternalClock, QString externalClockLabel)
{
    //    \s Matches a whitespace character (QChar::isSpace()).
    //    QStringList list = str.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    //    QStringList list = str.split(QRegExp("[\r\n\t ]+"), QString::SkipEmptyParts);

    listNumericData.clear();
    stringListLabels.clear();
    listTimeStamp.clear();
    lineCount = 0;

    QStringList inputStringSplitArrayLines = inputString.split(QRegExp("[\\n+\\r+]"), QString::SplitBehavior::SkipEmptyParts);
    lineCount = inputStringSplitArrayLines.count();

    for (auto l = 0; l < inputStringSplitArrayLines.count(); ++l)
    {
        parsingProgressPercent = (float)l / inputStringSplitArrayLines.count() * 100.0F;
        if (l % 50 == 0 && canReportProgress)
        {
            emit updateProgress(&parsingProgressPercent);
            QApplication::processEvents(); // Prevents app for freeezeng during processing large files and allows to udpate progress percent. A very cheap trick...
        }

        if (abortFlag)
        {
            abortFlag = false;
            break;
        }

        QRegExp mainSymbols("[+-]?\\d*\\.?\\d+"); // float only   //  QRegExp mainSymbols("[-+]?[0-9]*\.?[0-9]+");
        QRegExp alphanumericSymbols("\\w+");
        QRegExp sepSymbols("[=,]");

        inputStringSplitArrayLines[l].replace(sepSymbols, " ");
        QStringList inputStringSplitArray = inputStringSplitArrayLines[l].simplified().split(QRegExp("\\s+"), QString::SplitBehavior::SkipEmptyParts); // rozdzielamy traktująac spacje jako separator

        for (auto i = 0; i < inputStringSplitArray.count(); ++i)
        {
            // Find external time...
            if (useExternalClock)
            {
                if (externalClockLabel.isEmpty() == false && inputStringSplitArray[i] == externalClockLabel)
                {
                    latestTimeStamp = QTime::fromMSecsSinceStartOfDay(inputStringSplitArray[i + 1].toInt());
                }
                else if (externalClockLabel.isEmpty() == true)
                {
                    foreach (auto timeFormat, searchTimeFormatList)
                    {
                        if (QTime::fromString(inputStringSplitArray[i], timeFormat).isValid())
                        {
                            latestTimeStamp = QTime::fromString(inputStringSplitArray[i], timeFormat);
                            break;
                        }
                    }

                    if (minimumTime != QTime(0, 0, 0) && maximumTime != QTime(0, 0, 0))
                    {
                        if (latestTimeStamp < minimumTime || latestTimeStamp > maximumTime)
                        {
                            continue;
                        }
                    }
                }
            }

            // Labels +
            if (i == 0 && mainSymbols.exactMatch(inputStringSplitArray[0]))
            {
                listNumericData.append(inputStringSplitArray[i].toDouble());
                stringListLabels.append("Graph 0");
            }
            else if (i > 0 && mainSymbols.exactMatch(inputStringSplitArray[i]) && mainSymbols.exactMatch(inputStringSplitArray[i - 1]))
            {
                listNumericData.append(inputStringSplitArray[i].toDouble());
                stringListLabels.append("Graph " + QString::number(i));
            }
            else if (i > 0 && mainSymbols.exactMatch(inputStringSplitArray[i]) && !mainSymbols.exactMatch(inputStringSplitArray[i - 1]))
            {
                listNumericData.append(inputStringSplitArray[i].toDouble());
                stringListLabels.append(inputStringSplitArray[i - 1]);
            }
            else
            {
                continue; // We didnt find or add any new data points so lets not log time and skip to the next element on the list...
            }

            if (useExternalClock)
            {
                listTimeStamp.append(latestTimeStamp.msecsSinceStartOfDay());
            }
            else
            {
                if (syncToSystemClock)
                    listTimeStamp.append(parserClock->currentTime().msecsSinceStartOfDay());
                else
                    listTimeStamp.append(parserClock->elapsed());
            }
        }
    }
}

void Parser::parseCSV(QString inputString, bool useExternalLabel, QString externalClockLabel)
{
    listNumericData.clear();
    stringListLabels.clear();
    listTimeStamp.clear();
    lineCount = 0;

    QStringList inputStringSplitArrayLines = inputString.split(QRegExp("[\\n+\\r+]"), QString::SplitBehavior::SkipEmptyParts);
    lineCount = inputStringSplitArrayLines.count();

    QStringList csvLabels; // !

    for (auto l = 0; l < lineCount; ++l)
    {
        parsingProgressPercent = (float)l / lineCount * 100.0F;
        if (l % 50 == 0 && canReportProgress)
        {
            emit updateProgress(&parsingProgressPercent);
            QApplication::processEvents(); // Prevents app for freeezeng during processing large files and allows to udpate progress percent. A very cheap trick...
        }

        if (abortFlag)
        {
            abortFlag = false;
            break;
        }

        QRegExp mainSymbols("[+-]?\\d*\\.?\\d+"); // float only   //  QRegExp mainSymbols("[-+]?[0-9]*\.?[0-9]+");
        QRegExp alphanumericSymbols("\\w+");
        QRegExp sepSymbols("[=,]");

        inputStringSplitArrayLines[l].replace(sepSymbols, " ");
        inputStringSplitArrayLines[l].remove("\"");

        QStringList inputStringSplitArray = inputStringSplitArrayLines[l].simplified().split(QRegExp("\\s+"), QString::SplitBehavior::SkipEmptyParts); // rozdzielamy traktująac spacje jako separator

        // Look for labels
        if (l == 0)
        {
            for (auto i = 0; i < inputStringSplitArray.count(); ++i)
            {
                if (!mainSymbols.exactMatch(inputStringSplitArray[i]))
                {
                    if (!csvLabels.contains(inputStringSplitArray[i]))
                        csvLabels.append(inputStringSplitArray[i]);
                }
            }
        }

        // Look for time reference
        for (auto i = 0; i < inputStringSplitArray.count(); ++i)
        {
            if (useExternalLabel == true && externalClockLabel.isEmpty() == false && i == csvLabels.indexOf(externalClockLabel))
            {
                qDebug() << "inputStringSplitArray: " + QString::number(inputStringSplitArray[i].toFloat());

                latestTimeStamp = QTime::fromMSecsSinceStartOfDay((int)inputStringSplitArray[i].toFloat());
                qDebug() << "TIME: " + QString::number(inputStringSplitArray[i].toFloat());
                qDebug() << "latestTimeStamp: " + latestTimeStamp.toString();
                break;
            }
            else if (useExternalLabel == false)
            {
                foreach (auto timeFormat, searchTimeFormatList)
                {
                    if (QTime::fromString(inputStringSplitArray[i], timeFormat).isValid())
                    {
                        latestTimeStamp = QTime::fromString(inputStringSplitArray[i], timeFormat);

                        if (minimumTime != QTime(0, 0, 0) && maximumTime != QTime(0, 0, 0))
                        {
                            if (latestTimeStamp < minimumTime || latestTimeStamp > maximumTime)
                            {
                                continue;
                            }
                        }

                        break;
                    }
                }
            }
        }

        // Look for data
        for (auto i = 0; i < inputStringSplitArray.count(); ++i)
        {
            if (mainSymbols.exactMatch(inputStringSplitArray[i]))
            {
                if (i >= csvLabels.count())
                    continue; // TODO ERROR REPORTING
                stringListLabels.append(csvLabels[i]);
                listNumericData.append(inputStringSplitArray[i].toDouble());
                listTimeStamp.append(latestTimeStamp.msecsSinceStartOfDay());
            }
        }
    }
}

void Parser::getCSVReadyData(QStringList *columnNames, QList<QList<double>> *dataColumns)
{
    QStringList labelStorage = this->getLabelStorage();
    QStringList tempColumnNames = labelStorage;
    tempColumnNames.removeDuplicates();
    QList<QList<double>> tempColumnsData;
    QList<double> numericDataList = this->getDataStorage();

    for (auto i = 0; i < tempColumnNames.count(); ++i)
    {
        tempColumnsData.append(*new QList<double>);

        while (labelStorage.contains(tempColumnNames[i]))
        {
            tempColumnsData[tempColumnsData.count() - 1].append(numericDataList.takeAt(labelStorage.indexOf(tempColumnNames[i])));
            labelStorage.removeAt(labelStorage.indexOf(tempColumnNames[i]));
        }
    }

    *columnNames = tempColumnNames;
    *dataColumns = tempColumnsData;
}

QList<double> Parser::getDataStorage() { return dataStorage; }
QList<double> Parser::getListNumericValues() { return listNumericData; }
QList<long> Parser::getListTimeStamp() { return listTimeStamp; }
QList<long> Parser::getTimeStorage() { return timeStampStorage; }
QStringList Parser::getLabelStorage() { return labelStorage; }
QStringList Parser::getStringListLabels() { return stringListLabels; }
QStringList Parser::getStringListNumericData() { return stringListNumericData; }
QStringList Parser::getTextList() { return textStorage; }
void Parser::clearExternalClock() { latestTimeStamp.setHMS(0, 0, 0, 0); }
void Parser::restartChartTimer() { parserClock->restart(); }
void Parser::parserClockAddMSecs(int millis)
{
    parserClock->addMSecs(millis);
    // parserClock->start();
}

void Parser::appendSetToMemory(QStringList newlabelList, QList<double> newDataList, QList<long> newTimeList, QString text)
{
    labelStorage.append(newlabelList);
    dataStorage.append(newDataList);
    timeStampStorage.append(newTimeList);

    if (!text.isEmpty())
        textStorage.append(text);
}

void Parser::clearStorage()
{
    labelStorage.clear();
    dataStorage.clear();
    timeStampStorage.clear();
    textStorage.clear();
}

void Parser::clear()
{
    stringListLabels.clear();
    stringListNumericData.clear();
    listTimeStamp.clear();
}

void Parser::setParsingTimeRange(QTime minTime, QTime maxTime)
{
    minimumTime = minTime;
    maximumTime = maxTime;
}

void Parser::resetTimeRange()
{
    minimumTime.setHMS(0, 0, 0);
    maximumTime.setHMS(0, 0, 0);
}

void Parser::abort() { abortFlag = true; }
void Parser::setReportProgress(bool isEnabled) { canReportProgress = isEnabled; }
