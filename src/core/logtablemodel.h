#ifndef LOGTABLEMODEL_H
#define LOGTABLEMODEL_H

#include <QAbstractTableModel>
#include <QHash>
#include <QStringList>
#include <QVector>

#include <deque>

// Table of parsed samples: one row per received record, a Time column plus one column per label.
// Rows are appended in batches and the oldest are dropped beyond maxRows, so the table stays cheap
// at high data rates (a QTableWidget item per cell was ~30% of the GUI thread at 1000 lines/s).
class LogTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit LogTableModel(QObject *parent = nullptr);

    // Same record splitting as before: a label that is already filled in the current row starts the next row.
    void append(const QList<long> &timeStamps, const QStringList &labels, const QList<double> &values);
    void clear();
    void setMaxRows(int rows); // 0 = unlimited
    QStringList labels() const { return columnLabels; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

signals:
    void columnsAdded();

private:
    struct Row
    {
        long timeMs = 0;
        QVector<double> values; // NaN = no value
    };

    void trimToMaxRows();

    std::deque<Row> rows;
    QStringList columnLabels;
    QHash<QString, int> columnOfLabel;
    int maxRows = 0;
};

#endif // LOGTABLEMODEL_H
