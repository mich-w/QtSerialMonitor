#include "logtablemodel.h"

#include <QSet>
#include <QTime>

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    constexpr double missing = std::numeric_limits<double>::quiet_NaN();
}

LogTableModel::LogTableModel(QObject *parent) : QAbstractTableModel(parent)
{
}

void LogTableModel::append(const QList<long> &timeStamps, const QStringList &labels, const QList<double> &values)
{
    const auto count = qMin(labels.count(), qMin(timeStamps.count(), values.count()));
    if (count == 0)
        return;

    // New labels -> new columns (announced before the rows that use them).
    QStringList newLabels;
    for (auto i = 0; i < count; ++i)
    {
        const QString label = labels[i].trimmed();
        if (!columnOfLabel.contains(label) && !newLabels.contains(label))
            newLabels.append(label);
    }
    if (!newLabels.isEmpty())
    {
        const int first = columnLabels.count() + 1; // column 0 is Time
        beginInsertColumns(QModelIndex(), first, first + newLabels.count() - 1);
        for (const auto &label : newLabels)
        {
            columnOfLabel.insert(label, columnLabels.count());
            columnLabels.append(label);
        }
        for (auto &row : rows)
        {
            const int previous = row.values.size();
            row.values.resize(columnLabels.count()); // resize(n, value) is Qt 6 only
            std::fill(row.values.begin() + previous, row.values.end(), missing);
        }
        endInsertColumns();
        emit columnsAdded();
    }

    // Build the new rows, then insert them in one go.
    std::vector<Row> newRows;
    QSet<int> filled;
    for (auto i = 0; i < count; ++i)
    {
        const int column = columnOfLabel.value(labels[i].trimmed());
        if (newRows.empty() || filled.contains(column))
        {
            newRows.push_back({timeStamps[i], QVector<double>(columnLabels.count(), missing)});
            filled.clear();
        }
        newRows.back().values[column] = values[i];
        filled.insert(column);
    }

    const int first = int(rows.size());
    beginInsertRows(QModelIndex(), first, first + int(newRows.size()) - 1);
    for (auto &row : newRows)
        rows.push_back(std::move(row));
    endInsertRows();

    trimToMaxRows();
}

void LogTableModel::clear()
{
    beginResetModel();
    rows.clear();
    columnLabels.clear();
    columnOfLabel.clear();
    endResetModel();
}

void LogTableModel::setMaxRows(int newMaxRows)
{
    maxRows = qMax(0, newMaxRows);
    trimToMaxRows();
}

void LogTableModel::trimToMaxRows()
{
    if (maxRows <= 0 || int(rows.size()) <= maxRows)
        return;

    const int excess = int(rows.size()) - maxRows;
    beginRemoveRows(QModelIndex(), 0, excess - 1);
    rows.erase(rows.begin(), rows.begin() + excess);
    endRemoveRows();
}

int LogTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(rows.size());
}

int LogTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : columnLabels.count() + 1;
}

QVariant LogTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= int(rows.size()))
        return QVariant();

    const Row &row = rows[size_t(index.row())];

    if (role == Qt::DisplayRole)
    {
        if (index.column() == 0)
            return QTime::fromMSecsSinceStartOfDay(int(row.timeMs)).toString("hh:mm:ss:zzz");

        const double value = row.values.value(index.column() - 1, missing);
        return std::isnan(value) ? QVariant() : QVariant(QString::number(value));
    }

    if (role == Qt::TextAlignmentRole && index.column() > 0)
        return int(Qt::AlignRight | Qt::AlignVCenter);

    return QVariant();
}

QVariant LogTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    if (orientation == Qt::Vertical)
        return section + 1;
    return section == 0 ? QString("Time") : columnLabels.value(section - 1);
}

// Sorts the rows once (header click); rows received later are appended at the end.
void LogTableModel::sort(int column, Qt::SortOrder order)
{
    if (column < 0 || column > columnLabels.count())
        return;

    emit layoutAboutToBeChanged();
    auto key = [column](const Row &row)
    {
        if (column == 0)
            return double(row.timeMs);
        const double value = row.values.value(column - 1, missing);
        return std::isnan(value) ? -std::numeric_limits<double>::infinity() : value;
    };
    std::stable_sort(rows.begin(), rows.end(), [&](const Row &a, const Row &b)
                     { return order == Qt::AscendingOrder ? key(a) < key(b) : key(a) > key(b); });
    emit layoutChanged();
}
