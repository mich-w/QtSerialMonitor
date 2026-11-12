#include "logtablemodel.h"

#include <QSignalSpy>
#include <QtTest>

class TestLogTableModel : public QObject
{
    Q_OBJECT

    static QString cell(const LogTableModel &model, int row, int column) { return model.index(row, column).data().toString(); }

private slots:
    void rowsAndColumns()
    {
        LogTableModel model;
        model.append({1000, 1000}, {"a", "b"}, {1, 2});
        model.append({2000}, {"c"}, {3});

        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.columnCount(), 4);
        QCOMPARE(model.headerData(0, Qt::Horizontal).toString(), QString("Time"));
        QCOMPARE(model.headerData(3, Qt::Horizontal).toString(), QString("c"));
        QCOMPARE(cell(model, 0, 0), QString("00:00:01:000"));
        QCOMPARE(cell(model, 0, 2), QString("2"));
        QCOMPARE(cell(model, 0, 3), QString()); // no value for c in the first row
        QCOMPARE(cell(model, 1, 3), QString("3"));
    }

    void repeatedLabelStartsNewRow()
    {
        LogTableModel model;
        model.append({1, 1, 2, 3}, {"x", "y", "x", "x"}, {1, 2, 3, 4});
        QCOMPARE(model.rowCount(), 3);
        QCOMPARE(cell(model, 1, 1), QString("3"));
        QCOMPARE(cell(model, 1, 2), QString());
    }

    void maxRowsDropsOldest()
    {
        LogTableModel model;
        model.setMaxRows(3);
        for (auto i = 0; i < 10; ++i)
            model.append({i}, {"v"}, {double(i)});
        QCOMPARE(model.rowCount(), 3);
        QCOMPARE(cell(model, 0, 1), QString("7"));
        QCOMPARE(cell(model, 2, 1), QString("9"));
    }

    void batchInsertsOnce()
    {
        LogTableModel model;
        QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
        model.append({1, 1, 1, 1}, {"a", "a", "a", "a"}, {1, 2, 3, 4});
        QCOMPARE(model.rowCount(), 4);
        QCOMPARE(inserted.count(), 1);
    }

    void sortByValueColumn()
    {
        LogTableModel model;
        model.append({1, 2, 3}, {"v", "v", "v"}, {5, -1, 3});
        model.sort(1, Qt::AscendingOrder);
        QCOMPARE(cell(model, 0, 1), QString("-1"));
        QCOMPARE(cell(model, 2, 1), QString("5"));
        model.sort(0, Qt::DescendingOrder);
        QCOMPARE(cell(model, 0, 1), QString("3"));
    }

    void clearResets()
    {
        LogTableModel model;
        model.append({1}, {"a"}, {1});
        model.clear();
        QCOMPARE(model.rowCount(), 0);
        QCOMPARE(model.columnCount(), 1);
    }
};

QTEST_GUILESS_MAIN(TestLogTableModel)
#include "tst_logtablemodel.moc"
