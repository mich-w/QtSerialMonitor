#ifndef PARSER_H
#define PARSER_H

#include <QObject>
#include <QTime>
#include <QDebug>
#include <QTimer>
#include <QApplication>

class Parser : public QObject
{
    Q_OBJECT
public:
    explicit Parser(QObject *parent = nullptr);
    void parse(QString inputString, bool syncToSystemClock = true, bool useExternalClock = false, QString externalClockLabel = QString());
    QStringList getStringListNumericData();
    QStringList getStringListLabels();
    QList<double> getListNumericValues();
    QList<long> getListTimeStamp();
    void clearExternalClock();
    void restartChartTimer();
    void parserClockAddMSecs(int millis);
    void appendSetToMemory(QStringList newlabelList, QList<double> newDataList, QList<long> newTimeList, QString text = "");
    QStringList getLabelStorage();
    QList<double> getDataStorage();
    QList<long> getTimeStorage();
    QStringList getTextList();
    void clear();
    void setParsingTimeRange(QTime minTime, QTime maxTime);
    void resetTimeRange();
    void abort();
    void setReportProgress(bool isEnabled);
signals:
    void updateProgress(float *percent);
public slots:
private:
    QTime *parserClock;
    QTime latestTimeStamp;
    QStringList stringListNumericData, stringListLabels;
    QList<double> listNumericData;
    QList<long> listTimeStamp;
    QStringList searchTimeFormatList = {"hh:mm:ss:zzz", "hh:mm:ss.zzz", "hh:mm:ss.z"};

    QStringList labelStorage;
    QList<double> dataStorage;
    QList<long> timeStorage;
    QStringList textStorage;
    int lineCount = 0;
    float parsingProgressPercent = 0.0f;
    bool abortFlag = false;
    bool canReportProgress = false;
    QTime minimumTime, maximumTime;
};

#endif // PARSER_H
