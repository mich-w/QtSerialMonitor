#ifndef LOGGER_H
#define LOGGER_H

#include <QDateTime>
#include <QRegularExpression>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QtDebug>
#include <QTextStream>
#include <QTime>

class Logger : public QObject
{
    Q_OBJECT
public:
    explicit Logger(QObject *parent = nullptr);
    ~Logger();
    bool beginLog(QString path, bool autoLogging, QString fileName, bool truncateFile = false);
    bool isOpen();
    void clearWriteBuffer();
    void closeFile();
    bool openFile(QString fileName, bool trunc = false);
    // Several records (lines) may come at once: a label that repeats starts the next row.
    // timesMsOfDay (optional, parallel to dataList): when each value arrived, used for the time column.
    void writeLogCSV(QStringList labelList, QList<double> dataList, const QList<long> &timesMsOfDay = {}, bool addTime = true);
    void writeLogTXTLine(QString lineToAppend, bool simplifyText);
    void writeLogTXTParsedData(QStringList labelList, QList<double> dataList, const QList<long> &timesMsOfDay = {});
signals:

public slots:

private:
    static QString timeStamp(long msOfDay = -1);
    static QList<QPair<int, int>> records(const QStringList &labels); // [begin, end) per record
    void rewriteCSVHeader();
    void writeLine(const QString &line);

    QFile *logFile = nullptr;
    QStringList csvLabelsBuffer;
};

#endif // LOGGER_H
