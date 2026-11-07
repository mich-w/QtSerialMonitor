#ifndef PARSER_H
#define PARSER_H

#include <QObject>
#include <QTime>
#include <QDebug>
#include <QCoreApplication>
#include <QElapsedTimer>

#include <atomic>
#include <functional>

class Parser : public QObject
{
    Q_OBJECT
public:
    explicit Parser(QObject *parent = nullptr);
    QList<double> getDataStorage();
    QList<double> getListNumericValues();
    QList<long> getListTimeStamp();
    QList<long> getTimeStorage();
    QStringList getLabelStorage();
    QStringList getStringListLabels();
    QStringList getStringListNumericData();
    QStringList getTextList();
    QStringList searchTimeFormatList = {"hh:mm:ss:zzz", "hh:mm:ss.zzz", "hh:mm:ss.z", "hh:mm:ss"};
    void abort();
    bool wasAborted() const { return lastParseAborted; } // true if the last parse stopped early
    void appendSetToMemory(QStringList newlabelList, QList<double> newDataList, QList<long> newTimeList, QString text = "");
    void clear();
    void clearExternalClock();
    void clearStorage();
    void parse(QString inputString, bool syncToSystemClock = true, bool useExternalClock = false, QString externalClockLabel = QString());
    void parseCSV(QString inputString, bool useExternalLabel = false, QString externalClockLabel = "");
    void resetTimeRange();
    void restartChartTimer();
    void setParsingTimeRange(QTime minTime, QTime maxTime);
    void setProgressCallback(std::function<void(int percent)> callback); // called from the parsing thread
    void getCSVReadyData(QStringList *columnNames, QList<QList<double> > *dataColumns);
private:
    std::atomic<bool> abortFlag{false};
    bool lastParseAborted = false;
    std::function<void(int)> progressCallback;
    bool shouldStop(int line); // reports progress, returns true when aborted
    int lineCount = 0;
    QList<double> dataStorage;
    QList<double> listNumericData;
    QList<long> listTimeStamp;
    QList<long> timeStampStorage;
    QStringList labelStorage;
    QStringList stringListNumericData, stringListLabels;
    QStringList textStorage;
    QElapsedTimer parserClock;
    bool isInParsingTimeRange(const QTime &time) const;
    QTime findTimeStamp(const QStringList &tokens) const;
    QTime latestTimeStamp;
    QTime minimumTime, maximumTime;
};

#endif // PARSER_H
