#include "chartdatamodel.h"
#include "qcustomplot.h"

#include <QAbstractItemModelTester>
#include <QElapsedTimer>
#include <QtTest>

class TestChartDataModel : public QObject
{
    Q_OBJECT

    QCustomPlot *plot = nullptr;
    ChartDataModel *model = nullptr;

    QCPGraph *addGraph(const QString &name)
    {
        QCPGraph *graph = plot->addGraph();
        graph->setName(name);
        return graph;
    }
    QString cell(int row, int column) { return model->index(row, column).data().toString(); }

private slots:
    void init()
    {
        plot = new QCustomPlot;
        model = new ChartDataModel;
        new QAbstractItemModelTester(model, QAbstractItemModelTester::FailureReportingMode::QtTest, model);
        model->setPlot(plot);
    }

    void cleanup()
    {
        delete model;
        delete plot;
    }

    void rowsMergeGraphsByTime()
    {
        QCPGraph *a = addGraph("a"), *b = addGraph("b");
        a->addData({1.0, 2.0, 3.0}, {10, 20, 30});
        b->addData({1.0, 2.5, 3.0}, {-1, -2.5, -3});
        model->refresh();

        QCOMPARE(model->columnCount(), 3);
        QCOMPARE(model->headerData(1, Qt::Horizontal).toString(), QString("a"));
        QCOMPARE(model->headerData(2, Qt::Horizontal).toString(), QString("b"));
        QCOMPARE(model->rowCount(), 4); // 1, 2, 2.5, 3
        QCOMPARE(cell(0, 1), QString("10"));
        QCOMPARE(cell(0, 2), QString("-1"));
        QCOMPARE(cell(1, 1), QString("20"));
        QCOMPARE(cell(1, 2), QString());     // b has nothing at 2 s
        QCOMPARE(cell(2, 1), QString());
        QCOMPARE(cell(2, 2), QString("-2.5"));
        QCOMPARE(cell(3, 2), QString("-3"));
        QCOMPARE(cell(0, 0), ChartDataModel::timeText(1.0));
        QCOMPARE(ChartDataModel::timeText(3600 * 10 + 0.25), QString("10:00:00:250"));
    }

    void repeatedKeyGetsOwnRows()
    {
        QCPGraph *a = addGraph("a"), *b = addGraph("b");
        a->addData({1.0, 1.0, 2.0}, {1, 2, 3});
        b->addData({1.0}, {7});
        model->refresh();

        QCOMPARE(model->rowCount(), 3);
        QCOMPARE(cell(0, 1), QString("1"));
        QCOMPARE(cell(0, 2), QString("7"));
        QCOMPARE(cell(1, 1), QString("2"));
        QCOMPARE(cell(1, 2), QString());
        QCOMPARE(cell(2, 1), QString("3"));
    }

    void appendsAndTrimsIncrementally()
    {
        QCPGraph *a = addGraph("a");
        a->addData({1.0, 2.0}, {1, 2});
        model->refresh();
        QCOMPARE(model->rowCount(), 2);

        QSignalSpy resets(model, &QAbstractItemModel::modelReset);
        QSignalSpy inserted(model, &QAbstractItemModel::rowsInserted);
        QSignalSpy removed(model, &QAbstractItemModel::rowsRemoved);

        a->addData(2.0, 2.5); // same key as the last row: one more row
        a->addData(3.0, 3);
        model->refresh();
        QCOMPARE(model->rowCount(), 4);
        QCOMPARE(cell(2, 1), QString("2.5"));
        QCOMPARE(cell(3, 1), QString("3"));

        a->data()->removeBefore(2.0); // "Keep data for"
        a->addData(4.0, 4);
        model->refresh();
        QCOMPARE(model->rowCount(), 4); // 2, 2, 3, 4
        QCOMPARE(cell(0, 1), QString("2"));
        QCOMPARE(cell(3, 1), QString("4"));

        QCOMPARE(resets.count(), 0);
        QCOMPARE(inserted.count(), 2);
        QCOMPARE(removed.count(), 1);

        model->refresh(); // nothing new
        QCOMPARE(inserted.count(), 2);
    }

    void rebuildsWhenGraphsChange()
    {
        QCPGraph *a = addGraph("a");
        a->addData(1.0, 1);
        model->refresh();

        QSignalSpy columns(model, &ChartDataModel::columnsChanged);
        QCPGraph *b = addGraph("b");
        b->addData(1.0, 5);
        model->refresh();
        QCOMPARE(columns.count(), 1);
        QCOMPARE(model->columnCount(), 3);
        QCOMPARE(cell(0, 2), QString("5"));

        plot->removeGraph(a);
        model->refresh();
        QCOMPARE(model->columnCount(), 2);
        QCOMPARE(cell(0, 1), QString("5"));

        b->data()->clear(); // Clear plot
        model->refresh();
        QCOMPARE(model->rowCount(), 0);

        plot->clearGraphs();
        model->refresh();
        QCOMPARE(model->columnCount(), 1);
    }

    void olderDataNeedsInvalidate()
    {
        QCPGraph *a = addGraph("a");
        a->addData({5.0, 6.0}, {5, 6});
        model->refresh();

        a->addData(1.0, 1); // inserted before the newest key, e.g. loaded from parser memory
        model->invalidate();
        model->refresh();
        QCOMPARE(model->rowCount(), 3);
        QCOMPARE(cell(0, 1), QString("1"));
    }

    void lookups()
    {
        QCPGraph *a = addGraph("a");
        a->addData({1.0, 2.0, 3.0}, {1, 2, 3});
        model->refresh();
        QCOMPARE(model->keyAt(1), 2.0);
        QCOMPARE(model->rowForKey(1.5), 1);
        QCOMPARE(model->rowForKey(10), 3);
        QCOMPARE(model->lastKey(), 3.0);
        QVERIFY(std::isnan(model->keyAt(5)));
    }

    void bigDataIsFast()
    {
        const int points = 1000000;
        QVector<double> keys(points), values(points);
        for (int i = 0; i < points; ++i)
        {
            keys[i] = i * 0.001;
            values[i] = i;
        }
        for (const char *name : {"a", "b", "c"})
            addGraph(name)->setData(keys, values, true);
        plot->graph(1)->data()->remove(0.5, 0.6); // gaps -> empty cells

        delete model; // without QAbstractItemModelTester, which visits every cell
        model = new ChartDataModel;
        model->setPlot(plot);

        QElapsedTimer timer;
        timer.start();
        model->refresh();
        QVERIFY2(timer.elapsed() < 1000, qPrintable(QString("full rebuild took %1 ms").arg(timer.elapsed())));
        QCOMPARE(model->rowCount(), points);
        QCOMPARE(cell(points - 1, 3), QString::number(points - 1));

        for (int g = 0; g < 3; ++g)
            plot->graph(g)->addData(points * 0.001, 1);
        timer.restart();
        model->refresh();
        QVERIFY2(timer.elapsed() < 50, qPrintable(QString("incremental refresh took %1 ms").arg(timer.elapsed())));
        QCOMPARE(model->rowCount(), points + 1);
    }
};

QTEST_MAIN(TestChartDataModel)
#include "tst_chartdatamodel.moc"
