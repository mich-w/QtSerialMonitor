#ifndef FILEREADER_H
#define FILEREADER_H

#include <QFile>
#include <QList>
#include <QObject>
#include <QTime>

class FileReader : public QObject
{
    Q_OBJECT
public:
    explicit FileReader(QObject *parent = nullptr);
    bool readAllAtOnce(QFile *fileToRead);
    static QList<QTime> getFileTimeRange(QFile *file);
signals:
    void fileReadFinished();
    void textReady(QString *);
};

#endif // FILEREADER_H
