#ifndef CSVEXPORT_H
#define CSVEXPORT_H

#include <QList>
#include <QString>
#include <QStringList>

class QAbstractItemModel;

namespace CsvExport
{
    // Writes one column per label; shorter columns leave their remaining cells empty.
    bool writeColumns(const QString &path, const QStringList &labels, const QList<QList<double>> &columns,
                      QChar separator = ',', QString *error = nullptr);

    // Header + display text of every cell, quoted (quotes doubled); empty cells stay empty.
    bool writeTable(const QString &path, const QAbstractItemModel *model, QChar separator = ',', QString *error = nullptr);

    QString quoted(const QString &text);
}

#endif // CSVEXPORT_H
