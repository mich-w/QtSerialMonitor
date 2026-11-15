#include "chartdatamodel.h"

#include "qcustomplot.h"

#include <QTime>

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    constexpr double noKey = std::numeric_limits<double>::quiet_NaN();
}

ChartDataModel::ChartDataModel(QObject *parent) : QAbstractTableModel(parent)
{
}

void ChartDataModel::setPlot(QCustomPlot *newPlot)
{
    plot = newPlot;
    rebuildPending = true;
    refresh();
}

QList<QCPGraph *> ChartDataModel::currentGraphs() const
{
    QList<QCPGraph *> result;
    if (plot)
        for (auto i = 0; i < plot->graphCount(); ++i)
            result.append(plot->graph(i));
    return result;
}

// Keys of all graphs from fromKey on, merged in order. A key appears as often as the graph with the
// most points at that key has them, so each of those points gets its own row.
std::vector<double> ChartDataModel::mergedKeysFrom(double fromKey) const
{
    using Iterator = QCPGraphDataContainer::const_iterator;
    QList<Iterator> heads, ends;
    for (const auto &graph : graphs)
    {
        if (!graph)
            continue;
        const auto data = graph->data();
        heads.append(std::isnan(fromKey) ? data->constBegin() : data->findBegin(fromKey, false));
        ends.append(data->constEnd());
    }

    std::vector<double> keys;
    while (true)
    {
        double key = std::numeric_limits<double>::infinity();
        for (auto g = 0; g < heads.count(); ++g)
            if (heads[g] != ends[g])
                key = qMin(key, heads[g]->key);
        if (std::isinf(key))
            break;

        int rows = 0;
        for (auto g = 0; g < heads.count(); ++g)
        {
            int count = 0;
            while (heads[g] != ends[g] && heads[g]->key == key)
            {
                ++heads[g];
                ++count;
            }
            rows = qMax(rows, count);
        }
        keys.insert(keys.end(), size_t(rows), key);
    }
    return keys;
}

void ChartDataModel::rememberLastKeys()
{
    lastKeys.clear();
    for (const auto &graph : graphs)
        lastKeys.append(graph && !graph->data()->isEmpty() ? (graph->data()->constEnd() - 1)->key : noKey);
}

void ChartDataModel::rebuild(const QList<QCPGraph *> &newGraphs)
{
    QStringList newNames;
    for (auto *graph : newGraphs)
        newNames.append(graph->name());
    const bool columnsDiffer = newNames != names;

    beginResetModel();
    graphs.clear();
    for (auto *graph : newGraphs)
        graphs.append(graph);
    names = newNames;
    rowKeys = mergedKeysFrom(noKey);
    rememberLastKeys();
    rebuildPending = false;
    endResetModel();

    if (columnsDiffer)
        emit columnsChanged();
}

void ChartDataModel::refresh()
{
    const QList<QCPGraph *> current = currentGraphs();

    bool needsRebuild = rebuildPending || current.count() != graphs.count();
    for (auto g = 0; !needsRebuild && g < current.count(); ++g)
    {
        if (graphs[g] != current[g])
        {
            needsRebuild = true;
            break;
        }
        const auto data = current[g]->data();
        if (std::isnan(lastKeys[g]))
            continue; // had no data yet
        if (data->isEmpty() || (data->constEnd() - 1)->key < lastKeys[g]) // cleared or rewritten
            needsRebuild = true;
    }

    if (needsRebuild || rowKeys.empty())
    {
        rebuild(current);
        return;
    }

    // New rows: everything from the last known key on, minus the rows that are already there.
    const double last = rowKeys.back();
    const auto existingAtLast = size_t(rowKeys.end() - std::lower_bound(rowKeys.begin(), rowKeys.end(), last));
    const std::vector<double> newKeys = mergedKeysFrom(last);
    const auto atLast = size_t(std::upper_bound(newKeys.begin(), newKeys.end(), last) - newKeys.begin());
    const size_t skip = qMin(existingAtLast, atLast);
    if (newKeys.size() > skip)
    {
        const int first = int(rowKeys.size());
        beginInsertRows(QModelIndex(), first, first + int(newKeys.size() - skip) - 1);
        rowKeys.insert(rowKeys.end(), newKeys.begin() + qsizetype(skip), newKeys.end());
        endInsertRows();
    }

    // Rows the chart no longer holds ("Keep data for").
    double oldest = std::numeric_limits<double>::infinity();
    for (const auto &graph : graphs)
        if (!graph->data()->isEmpty())
            oldest = qMin(oldest, graph->data()->constBegin()->key);
    const int stale = int(std::lower_bound(rowKeys.begin(), rowKeys.end(), oldest) - rowKeys.begin());
    if (stale > 0)
    {
        beginRemoveRows(QModelIndex(), 0, stale - 1);
        rowKeys.erase(rowKeys.begin(), rowKeys.begin() + stale);
        endRemoveRows();
    }

    rememberLastKeys();
}

double ChartDataModel::keyAt(int row) const
{
    return row >= 0 && row < int(rowKeys.size()) ? rowKeys[size_t(row)] : noKey;
}

int ChartDataModel::rowForKey(double key) const
{
    return int(std::lower_bound(rowKeys.begin(), rowKeys.end(), key) - rowKeys.begin());
}

double ChartDataModel::lastKey() const
{
    return rowKeys.empty() ? noKey : rowKeys.back();
}

int ChartDataModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(rowKeys.size());
}

int ChartDataModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(graphs.count()) + 1;
}

QString ChartDataModel::timeText(double key)
{
    constexpr qint64 day = 24 * 3600 * 1000;
    const qint64 ms = ((qRound64(key * 1000.0) % day) + day) % day;
    return QTime::fromMSecsSinceStartOfDay(int(ms)).toString("hh:mm:ss:zzz");
}

QVariant ChartDataModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= int(rowKeys.size()) || index.column() > graphs.count())
        return QVariant();

    if (role == Qt::TextAlignmentRole && index.column() > 0)
        return int(Qt::AlignRight | Qt::AlignVCenter);
    if (role != Qt::DisplayRole)
        return QVariant();

    const double key = rowKeys[size_t(index.row())];
    if (index.column() == 0)
        return timeText(key);

    const QCPGraph *graph = graphs[index.column() - 1];
    if (!graph)
        return QVariant();

    // The n-th row with this key shows each graph's n-th point at this key.
    const auto occurrence = index.row() - int(std::lower_bound(rowKeys.begin(), rowKeys.end(), key) - rowKeys.begin());
    const auto data = graph->data();
    auto it = data->findBegin(key, false);
    for (auto i = 0; i < occurrence && it != data->constEnd() && it->key == key; ++i)
        ++it;
    if (it == data->constEnd() || it->key != key)
        return QVariant();
    return QString::number(it->value);
}

QVariant ChartDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    if (orientation == Qt::Vertical)
        return section + 1;
    return section == 0 ? QString("Time") : names.value(section - 1);
}
