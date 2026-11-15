#include "csvexport.h"

#include <QAbstractItemModel>
#include <QFile>
#include <QTextStream>

QString CsvExport::quoted(const QString &text)
{
    return "\"" + QString(text).replace("\"", "\"\"") + "\"";
}

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
        header.append(quoted(label));
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

bool CsvExport::writeTable(const QString &path, const QAbstractItemModel *model, QChar separator, QString *error)
{
    QFile file(path);
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
    {
        if (error)
            *error = file.errorString();
        return false;
    }

    QTextStream out(&file);
    auto cell = [](const QString &text) { return text.isEmpty() ? QString() : quoted(text); };

    for (int column = 0; column < model->columnCount(); ++column)
        out << (column > 0 ? QString(separator) : QString()) << cell(model->headerData(column, Qt::Horizontal).toString());
    out << '\n';

    for (int row = 0; row < model->rowCount(); ++row)
    {
        for (int column = 0; column < model->columnCount(); ++column)
            out << (column > 0 ? QString(separator) : QString()) << cell(model->index(row, column).data().toString());
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
