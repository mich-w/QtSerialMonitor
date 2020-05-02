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
    void appendSetToMemory(QStringList newlabelList, QList<double> newDataList, QList<long> newTimeList, QString text = "");
    void clear();
    void clearExternalClock();
    void clearStorage();
    void parse(QString inputString, bool syncToSystemClock = true, bool useExternalClock = false, QString externalClockLabel = QString());
    void parseCSV(QString inputString, bool useExternalLabel = false, QString externalClockLabel = "");
    void parserClockAddMSecs(int millis);
    void resetTimeRange();
    void restartChartTimer();
    void setParsingTimeRange(QTime minTime, QTime maxTime);
    void setReportProgress(bool isEnabled);
    void getCSVReadyData(QStringList *columnNames, QList<QList<double> > *dataColumns);
signals:
    void updateProgress(float *percent);
public slots:
private:
    bool abortFlag = false;
    bool canReportProgress = false;
    float parsingProgressPercent = 0.0f;
    int lineCount = 0;
    QList<double> dataStorage;
    QList<double> listNumericData;
    QList<long> listTimeStamp;
    QList<long> timeStampStorage;
    QStringList labelStorage;
    QStringList stringListNumericData, stringListLabels;
    QStringList textStorage;
    QTime *parserClock;
    QTime latestTimeStamp;
    QTime minimumTime, maximumTime;
};

#endif // PARSER_H
