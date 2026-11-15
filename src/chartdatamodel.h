#ifndef CHARTDATAMODEL_H
#define CHARTDATAMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QPointer>

#include <vector>

class QCustomPlot;
class QCPGraph;

// Table view of the data held by the chart (QCustomPlot graphs - also the data model behind Qt Graphs).
// Nothing is copied: rows are only an index of time keys, cells are looked up in the graphs when shown.
// Column 0 is the time, then one column per graph. Values of different graphs with the same key share a row
// (one row per received line, since each line has one arrival time).
class ChartDataModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit ChartDataModel(QObject *parent = nullptr);

    void setPlot(QCustomPlot *plot);

    // Catches up with the chart: appends new rows and drops the ones the chart no longer holds.
    // Rebuilds everything when graphs were added/removed/cleared or after invalidate().
    void refresh();
    void invalidate() { rebuildPending = true; } // data was changed other than by appending

    double keyAt(int row) const;           // seconds, like the chart keys
    int rowForKey(double key) const;       // first row at or after key (rowCount() if none)
    double lastKey() const;                // NaN if empty

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    static QString timeText(double key);

signals:
    void columnsChanged(); // graphs added or removed

private:
    QList<QCPGraph *> currentGraphs() const;
    std::vector<double> mergedKeysFrom(double fromKey) const;
    void rebuild(const QList<QCPGraph *> &graphs);
    void rememberLastKeys();

    QPointer<QCustomPlot> plot;
    QList<QPointer<QCPGraph>> graphs; // columns 1..n
    QStringList names;
    QList<double> lastKeys;           // per graph, to notice cleared / rewritten data
    std::vector<double> rowKeys;      // sorted; a key repeats when one graph has several points at it
    bool rebuildPending = true;
};

#endif // CHARTDATAMODEL_H
