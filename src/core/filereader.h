#ifndef FILEREADER_H
#define FILEREADER_H

#include <QFile>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QThread>
#include <QTime>

#include <memory>

class Parser;

// Loads and parses a log file on a worker thread.
class FileReader : public QObject
{
    Q_OBJECT
public:
    struct Options
    {
        bool csv = false;
        bool useExternalClockLabel = false; // CSV only: take time from the column named clockLabel
        QString clockLabel;
        QTime minTime; // optional time-range filter
        QTime maxTime;
    };

    struct Result
    {
        QStringList labels;
        QList<double> values;
        QList<long> timeStamps;
        QString text;
        QString error;
        bool cancelled = false;
    };

    explicit FileReader(QObject *parent = nullptr);
    ~FileReader() override;

    bool load(const QString &path, const Options &options); // false if a load is already running
    void cancel();
    bool isBusy() const;

    static QList<QTime> getFileTimeRange(QFile *file);

signals:
    void progress(int percent);
    void loaded(const FileReader::Result &result);

private:
    QPointer<QThread> worker;
    std::shared_ptr<Parser> activeParser;
};

Q_DECLARE_METATYPE(FileReader::Result)

#endif // FILEREADER_H
