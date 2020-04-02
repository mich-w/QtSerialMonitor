#ifndef FILEREADER_H
#define FILEREADER_H

#include <QFile>
#include <QObject>
#include <QTextStream>
#include <QTimer>
#include <QTime>
#include <QDebug>

class FileReader : public QObject
{
    Q_OBJECT
public:
    explicit FileReader(QObject *parent = nullptr);
    bool readAllAtOnce(QFile *fileToRead);
    void abortRead();
    void setReadInterval(int newVal);
    QList<QTime> getFileTimeRange(QFile *file);
signals:
    void fileReadFinished();
    void lineReady(QString *, int *);
    void textReady(QString *);
public slots:
private slots:
    void readLineSendLine();

private:
    int lineReadIterator = 0;
    int readInterval = 1; // ms
    QStringList readFileSplitLines;
    QTimer fileReadTimer;
};

#endif // FILEREADER_H
