#ifndef DATARATECHART_H
#define DATARATECHART_H

#include <QObject>
#include <qcustomplot.h>

class DataRateChart : public QObject
{
    Q_OBJECT
public:
    explicit DataRateChart(QObject *parent = nullptr, QCustomPlot *plotWidget = nullptr);

    void disableDataRateChart();
    void createDataRateChart();
    void clearDataRateChart(bool replot);
    void processDataRateChart(QString input);
signals:

public slots:
private slots:
    void chartDataRateRunAutoTrackSlot();
private:
    QCustomPlot *dataRatePlot = nullptr;
    QTime appRunningClock;
};

#endif // DATARATECHART_H
