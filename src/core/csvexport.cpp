#include "csvexport.h"

#include <QFile>
#include <QTextStream>

bool CsvExport::writeColumns(const QString &path, const QStringList &labels, const QList<QList<double>> &columns, QChar separator,
                             QString *error)
{
    QFile file(path);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
    {
        if (error)
            *error = file.errorString();
        return false;
    }

    QTextStream out(&file);

    QStringList header;
    for (const auto &label : labels)
        header.append("\"" + QString(label).replace("\"", "\"\"") + "\"");
    out << header.join(separator) << '\n';

    qsizetype rows = 0;
    for (const auto &column : columns)
        rows = qMax(rows, qsizetype(column.count())); // int on Qt 5

    // Written row by row straight into the stream - linear in the number of values.
    for (qsizetype row = 0; row < rows; ++row)
    {
        for (qsizetype c = 0; c < qsizetype(columns.count()); ++c)
        {
            if (c > 0)
                out << separator;
            if (row < qsizetype(columns[c].count()))
                out << QString::number(columns[c][row], 'g', 10);
        }
        out << '\n';
    }

    out.flush();
    if (out.status() != QTextStream::Ok)
    {
        if (error)
            *error = file.errorString();
        return false;
    }
    return true;
}
