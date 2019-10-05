#include "dataratechart.h"

DataRateChart::DataRateChart(QObject *parent, QCustomPlot *plotWidget) : QObject(parent)
{
    dataRatePlot = plotWidget;
    createDataRateChart();

    appRunningClock.start();
}

void DataRateChart::disableDataRateChart()
{
    dataRatePlot->clearGraphs();
    dataRatePlot->xAxis->setVisible(false);
    dataRatePlot->xAxis2->setVisible(false);
    dataRatePlot->yAxis->setVisible(false);
    dataRatePlot->yAxis2->setVisible(false);
    dataRatePlot->replot();
}

void DataRateChart::createDataRateChart()
{
    QSharedPointer<QCPAxisTickerTime> xTicker(new QCPAxisTickerTime);
    QSharedPointer<QCPAxisTicker> yTicker(new QCPAxisTicker);
    xTicker->setTimeFormat("%h:%m:%s:%z");
    xTicker->setTickCount(5);
    yTicker->setTickCount(3);
    yTicker->setTickStepStrategy(QCPAxisTicker::TickStepStrategy::tssReadability);

    dataRatePlot->xAxis->setTicker(xTicker);
    dataRatePlot->yAxis->setTicker(yTicker);
    dataRatePlot->xAxis->setTickLabels(false);
    dataRatePlot->yAxis->setLabel("[ kB / s ]");
    dataRatePlot->yAxis->setLabelPadding(5);
    dataRatePlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignBottom|Qt::AlignLeft);
    dataRatePlot->axisRect()->setAutoMargins(QCP::msLeft);
    dataRatePlot->axisRect()->setMargins(QMargins(0,0,0,0));
    dataRatePlot->axisRect()->setupFullAxesBox(true);
    dataRatePlot->legend->setVisible(false);
    dataRatePlot->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

    dataRatePlot->addGraph();
    dataRatePlot->graph()->setName("dataRateBytesPerSec");
    // ui->widgetChart->graph()->selectionDecorator()-> setPen(QPen(Qt::darkBlue, 1.0, Qt::PenStyle::SolidLine));

    dataRatePlot->graph()->setLineStyle(QCPGraph::LineStyle::lsLine);
    dataRatePlot->graph()->setScatterStyle(QCPScatterStyle::ScatterShape::ssNone);
    dataRatePlot->graph()->setPen(QPen(Qt::GlobalColor::darkGreen));

    connect(dataRatePlot, SIGNAL(beforeReplot()), this, SLOT(chartDataRateRunAutoTrackSlot()));

    dataRatePlot->replot();
}

void DataRateChart::clearDataRateChart(bool replot)
{
    if (dataRatePlot->graphCount() > 0)
        dataRatePlot->graph()->data().data()->clear();

    if (replot)
        dataRatePlot->replot();
}

void DataRateChart::processDataRateChart(QString input)
{
    static unsigned long long lastTime = 0;
    unsigned long long deltaT = appRunningClock.elapsed() - lastTime;
    lastTime = appRunningClock.elapsed();

    static float bytesPerSec = 0, currentBytesPerSec = 0;
    if (input.size() > 0)
        currentBytesPerSec = input.size() / ((float)deltaT / 1000.0);
    else
        currentBytesPerSec = 0;

    auto alpha = 0.25;
    bytesPerSec = (alpha * currentBytesPerSec) + ((1.0f - alpha) * bytesPerSec);

    dataRatePlot->graph()->addData(appRunningClock.elapsed() / 1000.0, bytesPerSec / 1000.0);
    dataRatePlot->graph()->data().data()->removeBefore((appRunningClock.elapsed() / 1000.0) - 10.0);
    dataRatePlot->replot();
}

void DataRateChart::chartDataRateRunAutoTrackSlot()
{
    dataRatePlot->xAxis->setRange((appRunningClock.elapsed() / 1000.0) + (10.0f * 0.05),
                                             10.0f, Qt::AlignRight);

    dataRatePlot->yAxis->rescale();
    dataRatePlot->yAxis->scaleRange(2);
    //  ui->widgetDataRateChart->yAxis->setRangeLower(-2);
}
