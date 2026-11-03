#ifndef LOGGER_H
#define LOGGER_H

#include <QDateTime>
#include <QRegularExpression>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QtDebug>
#include <QTextStream>
#include <QTime>

class Logger : public QObject
{
    Q_OBJECT
public:
    explicit Logger(QObject *parent = nullptr);
    ~Logger();
    bool beginLog(QString path, bool autoLogging, QString fileName, bool truncateFile = false);
    bool isOpen();
    void clearWriteBuffer();
    void closeFile();
    bool openFile(QString fileName, bool trunc = false);
    void writeLogCSV(QStringList labelList, QList<double> dataList, bool addTime = true);
    void writeLogTXTLine(QString lineToAppend, bool simplifyText);
    void writeLogTXTParsedData(QStringList labelList, QList<double> dataList);
signals:

public slots:

private:
    static QString timeStamp();
    void rewriteCSVHeader();
    void writeLine(const QString &line);

    QFile *logFile = nullptr;
    QStringList csvLabelsBuffer;
};

#endif // LOGGER_H
