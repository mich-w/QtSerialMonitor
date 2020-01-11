#ifndef LOGGER_H
#define LOGGER_H

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QtDebug>
#include <QTextStream>

class Logger : public QObject
{
    Q_OBJECT
public:
    explicit Logger(QObject *parent = nullptr);
    bool beginLog(QString path, bool autoLogging, QString fileName);
    bool isOpen();
    void closeFile();
    void openFile(QString fileName);
    void writeLogLine(QString lineToAppend, bool simplifyText, bool appendDate);
    void writeLogParsedData(QStringList labelList, QList<double> dataList, bool appendDate = false);
signals:

public slots:

private:
    QFile *logFile = nullptr;
};

#endif // LOGGER_H
