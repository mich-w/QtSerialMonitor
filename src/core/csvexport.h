#ifndef CSVEXPORT_H
#define CSVEXPORT_H

#include <QList>
#include <QString>
#include <QStringList>

namespace CsvExport
{
    // Writes one column per label; shorter columns leave their remaining cells empty.
    bool writeColumns(const QString &path, const QStringList &labels, const QList<QList<double>> &columns,
                      QChar separator = ',', QString *error = nullptr);
}

#endif // CSVEXPORT_H
