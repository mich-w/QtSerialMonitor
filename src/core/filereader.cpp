#include "filereader.h"

#include "parser.h"

#include <QDateTime>
#include <QDebug>
#include <QTextStream>
#include <QRegularExpression>

FileReader::FileReader(QObject *parent) : QObject(parent)
{
}

FileReader::~FileReader()
{
    cancel();
    if (worker)
    {
        worker->wait();
        delete worker; // its finished handler is bound to this object and will not run
    }
}

bool FileReader::isBusy() const
{
    return !worker.isNull();
}

void FileReader::cancel()
{
    if (activeParser)
        activeParser->abort();
}

bool FileReader::load(const QString &path, const Options &options)
{
    if (isBusy())
        return false;

    auto parser = std::make_shared<Parser>();
    auto result = std::make_shared<Result>();
    activeParser = parser;

    parser->setParsingTimeRange(options.minTime, options.maxTime);
    parser->setProgressCallback([this](int percent)
                                { emit progress(percent); }); // emitted from the worker thread, queued to receivers

    worker = QThread::create([path, options, parser, result]()
                             {
                                 QFile file(path);
                                 if (!file.open(QIODevice::ReadOnly))
                                 {
                                     result->error = file.errorString();
                                     return;
                                 }

                                 result->text = QString::fromUtf8(file.readAll());
                                 file.close();

                                 if (options.csv)
                                     parser->parseCSV(result->text, options.useExternalClockLabel, options.clockLabel);
                                 else
                                     parser->parse(result->text, false, true, "");

                                 result->labels = parser->getStringListLabels();
                                 result->values = parser->getListNumericValues();
                                 result->timeStamps = parser->getListTimeStamp();
                             });

    connect(worker, &QThread::finished, this, [this, parser, result]()
            {
                result->cancelled = parser->wasAborted();
                worker->deleteLater();
                worker = nullptr;
                activeParser.reset();
                emit loaded(*result);
            });

    worker->start();
    return true;
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
