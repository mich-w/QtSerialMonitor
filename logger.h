#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QtDebug>
#include <QDir>

class Logger : public QObject
{
    Q_OBJECT
public:
    explicit Logger(QObject *parent = nullptr);

    void openFile(QString fileName);
    void closeFile();
    bool beginLog(QString path, bool autoLogging, QString fileName);
    bool isOpen();
    void writeLogLine(QString lineToAppend, bool simplifyText, bool appendDate);
    void writeLogParsedData(QStringList labelList, QList<double> dataList, bool appendDate = false);
signals:

public slots:

private:
    QFile *logFile = nullptr;

};

#endif // LOGGER_H
