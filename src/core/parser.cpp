#include "parser.h"

#include <QHash>
#include <QDateTime>
#include <QRegularExpression>

namespace
{
    // Signed decimal number with optional fraction and exponent: 1, -2.5, .5, 3., 1e-3, +4.2E+6
    const QRegularExpression numberRegex("^[+-]?(\\d+\\.?\\d*|\\.\\d+)([eE][+-]?\\d+)?$");
    const QRegularExpression lineSeparators("[\\r\\n]+");
    const QRegularExpression whitespace("\\s+");
    const QRegularExpression valueSeparators("[=,]");

    // Arduino IDE 2 Serial Plotter style "label:value". The label must start with a
    // letter so time stamps such as 10:30:00.000 are left alone.
    const QRegularExpression labelColonValue("^([A-Za-z_][^:]*):([+-]?(\\d+\\.?\\d*|\\.\\d+)([eE][+-]?\\d+)?)$");

    bool isNumber(const QString &token) { return numberRegex.match(token).hasMatch(); }

    QStringList expandLabelColonValue(const QStringList &tokens)
    {
        QStringList expanded;
        for (const auto &token : tokens)
        {
            const auto match = labelColonValue.match(token);
            if (match.hasMatch())
                expanded << match.captured(1) << match.captured(2);
            else
                expanded << token;
        }
        return expanded;
    }
}

void Parser::appendSample(const QString &label, double value, bool syncToSystemClock, bool useExternalClock)
{
    stringListLabels.append(label);
    listNumericData.append(value);

    if (useExternalClock)
        listTimeStamp.append(latestTimeStamp.msecsSinceStartOfDay());
    else if (syncToSystemClock)
        listTimeStamp.append(QTime::currentTime().msecsSinceStartOfDay());
    else
        listTimeStamp.append(parserClock.elapsed());
}

// Teleplot line: ">name:value", ">name:timestamp_ms:value", optionally followed by "§unit" and/or "|flags".
// Returns false if the line is not a numeric Teleplot sample.
bool Parser::parseTeleplotLine(const QString &line, bool syncToSystemClock, bool useExternalClock)
{
    QString body = line.trimmed().mid(1);

    const int flagsAt = body.indexOf('|');
    const QString flags = flagsAt >= 0 ? body.mid(flagsAt + 1) : QString();
    if (flagsAt >= 0)
        body.truncate(flagsAt);
    if (flags.contains('t') || flags.contains("xy"))
        return false; // text or XY variables are not plotted over time

    const int unitAt = body.indexOf(QChar(0x00A7)); // '§'
    if (unitAt >= 0)
        body.truncate(unitAt);

    const QStringList parts = body.split(':');
    if (parts.count() < 2 || parts.count() > 3 || parts[0].trimmed().isEmpty())
        return false;

    const QString value = parts.last().trimmed();
    if (!isNumber(value))
        return false;

    if (parts.count() == 3 && useExternalClock)
    {
        bool ok = false;
        const qint64 epochMs = qint64(parts[1].trimmed().toDouble(&ok));
        if (ok)
            latestTimeStamp = QDateTime::fromMSecsSinceEpoch(epochMs).time();
    }

    appendSample(parts[0].trimmed(), value.toDouble(), syncToSystemClock, useExternalClock);
    return true;
}

Parser::Parser(QObject *parent) : QObject(parent)
{
    parserClock.start();
    latestTimeStamp.setHMS(0, 0, 0, 0);
}

bool Parser::isInParsingTimeRange(const QTime &time) const
{
    // Range filtering is active only when a valid, non-empty range was set.
    if (!minimumTime.isValid() || !maximumTime.isValid() || minimumTime >= maximumTime)
        return true;

    return time >= minimumTime && time <= maximumTime;
}

QTime Parser::findTimeStamp(const QStringList &tokens) const
{
    for (const auto &token : tokens)
    {
        if (token.contains('T')) // ISO 8601 date-time, as written by Logger
        {
            QDateTime dateTime = QDateTime::fromString(token, Qt::ISODateWithMs);
            if (dateTime.isValid())
                return dateTime.time();
        }

        for (const auto &timeFormat : searchTimeFormatList)
        {
            QTime time = QTime::fromString(token, timeFormat);
            if (time.isValid())
                return time;
        }
    }
    return QTime();
}

void Parser::parse(QString inputString, bool syncToSystemClock, bool useExternalClock, QString externalClockLabel)
{
    lastParseAborted = false;
    listNumericData.clear();
    stringListLabels.clear();
    listTimeStamp.clear();

    QStringList inputStringSplitArrayLines = inputString.split(lineSeparators, Qt::SkipEmptyParts);
    lineCount = inputStringSplitArrayLines.count();

    for (auto l = 0; l < lineCount; ++l)
    {
        if (shouldStop(l))
            break;

        if (inputStringSplitArrayLines[l].startsWith('>'))
        {
            parseTeleplotLine(inputStringSplitArrayLines[l], syncToSystemClock, useExternalClock);
            continue;
        }

        inputStringSplitArrayLines[l].replace(valueSeparators, " ");
        const QStringList tokens = expandLabelColonValue(inputStringSplitArrayLines[l].simplified().split(whitespace, Qt::SkipEmptyParts));

        // Find external time for this line
        if (useExternalClock)
        {
            if (!externalClockLabel.isEmpty())
            {
                const int labelIndex = tokens.indexOf(externalClockLabel);
                if (labelIndex >= 0 && labelIndex + 1 < tokens.count() && isNumber(tokens[labelIndex + 1]))
                    latestTimeStamp = QTime::fromMSecsSinceStartOfDay(qRound(tokens[labelIndex + 1].toDouble()));
            }
            else
            {
                QTime time = findTimeStamp(tokens);
                if (time.isValid())
                {
                    latestTimeStamp = time;
                    if (!isInParsingTimeRange(time))
                        continue; // skip whole line
                }
            }
        }

        for (auto i = 0; i < tokens.count(); ++i)
        {
            if (!isNumber(tokens[i]))
                continue;

            QString label;
            if (i == 0 || isNumber(tokens[i - 1]))
                label = "Graph " + QString::number(i);
            else
            {
                label = tokens[i - 1];
                if (label.length() > 1 && label.endsWith(':')) // "Voltage: 1.23"
                    label.chop(1);
            }

            appendSample(label, tokens[i].toDouble(), syncToSystemClock, useExternalClock);
        }
    }
}

namespace
{
    // Splits one CSV record, keeping empty fields and honouring "quoted, fields" with "" escapes.
    QStringList splitCsvLine(const QString &line, QChar delimiter)
    {
        QStringList fields;
        QString field;
        bool inQuotes = false;

        for (auto i = 0; i < line.size(); ++i)
        {
            const QChar ch = line[i];
            if (inQuotes)
            {
                if (ch == '"' && i + 1 < line.size() && line[i + 1] == '"')
                {
                    field += '"';
                    ++i;
                }
                else if (ch == '"')
                    inQuotes = false;
                else
                    field += ch;
            }
            else if (ch == '"')
                inQuotes = true;
            else if (ch == delimiter)
            {
                fields.append(field.trimmed());
                field.clear();
            }
            else
                field += ch;
        }
        fields.append(field.trimmed());

        // A trailing delimiter (as written by older versions) does not start a new column.
        if (fields.count() > 1 && fields.last().isEmpty() && line.trimmed().endsWith(delimiter))
            fields.removeLast();

        return fields;
    }

    QChar detectCsvDelimiter(const QString &firstLine)
    {
        const QList<QChar> candidates = {',', ';', '\t'};
        QChar best = ',';
        int bestCount = 0;
        for (const QChar candidate : candidates)
        {
            const int count = firstLine.count(candidate);
            if (count > bestCount)
            {
                best = candidate;
                bestCount = count;
            }
        }
        return best;
    }
}

void Parser::parseCSV(QString inputString, bool useExternalLabel, QString externalClockLabel)
{
    lastParseAborted = false;
    listNumericData.clear();
    stringListLabels.clear();
    listTimeStamp.clear();

    const QStringList lines = inputString.split(lineSeparators, Qt::SkipEmptyParts);
    lineCount = lines.count();
    if (lineCount == 0)
        return;

    const QChar delimiter = detectCsvDelimiter(lines.first());
    QStringList csvLabels;
    int firstDataLine = 0;

    // Header: used unless every field of the first line is numeric.
    const QStringList firstFields = splitCsvLine(lines.first(), delimiter);
    bool hasHeader = false;
    for (const auto &field : firstFields)
        hasHeader |= !field.isEmpty() && !isNumber(field);

    for (auto i = 0; i < firstFields.count(); ++i)
    {
        const QString &name = firstFields[i];
        csvLabels.append(hasHeader && !name.isEmpty() ? name : "Graph " + QString::number(i));
    }
    if (hasHeader)
        firstDataLine = 1;

    const int clockColumn = useExternalLabel && !externalClockLabel.isEmpty() ? csvLabels.indexOf(externalClockLabel) : -1;

    for (auto l = firstDataLine; l < lineCount; ++l)
    {
        if (shouldStop(l))
            break;

        const QStringList fields = splitCsvLine(lines[l], delimiter);

        // Columns that appear beyond the header get generic names.
        while (csvLabels.count() < fields.count())
            csvLabels.append("Graph " + QString::number(csvLabels.count()));

        // Look for time reference
        if (clockColumn >= 0)
        {
            if (clockColumn < fields.count() && isNumber(fields[clockColumn]))
                latestTimeStamp = QTime::fromMSecsSinceStartOfDay(qRound(fields[clockColumn].toDouble()));
        }
        else if (!useExternalLabel)
        {
            QTime time = findTimeStamp(fields);
            if (time.isValid())
            {
                latestTimeStamp = time;
                if (!isInParsingTimeRange(time))
                    continue; // skip whole line
            }
        }

        // Look for data
        for (auto i = 0; i < fields.count(); ++i)
        {
            if (isNumber(fields[i]))
            {
                stringListLabels.append(csvLabels[i]);
                listNumericData.append(fields[i].toDouble());
                listTimeStamp.append(latestTimeStamp.msecsSinceStartOfDay());
            }
        }
    }
}

void Parser::getCSVReadyData(QStringList *columnNames, QList<QList<double>> *dataColumns)
{
    QStringList names;
    QList<QList<double>> columns;
    QHash<QString, int> columnIndex;

    const auto count = qMin(labelStorage.count(), dataStorage.count());
    for (auto i = 0; i < count; ++i)
    {
        auto it = columnIndex.find(labelStorage[i]);
        if (it == columnIndex.end())
        {
            it = columnIndex.insert(labelStorage[i], names.count());
            names.append(labelStorage[i]);
            columns.append(QList<double>());
        }
        columns[it.value()].append(dataStorage[i]);
    }

    *columnNames = names;
    *dataColumns = columns;
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
void Parser::restartChartTimer() { parserClock.restart(); }

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
    minimumTime = QTime();
    maximumTime = QTime();
}

void Parser::abort() { abortFlag = true; }
void Parser::setProgressCallback(std::function<void(int)> callback) { progressCallback = std::move(callback); }

bool Parser::shouldStop(int line)
{
    if (progressCallback && line % 1000 == 0)
        progressCallback(lineCount > 0 ? line * 100 / lineCount : 0);

    if (abortFlag.exchange(false))
        lastParseAborted = true;
    return lastParseAborted;
}
