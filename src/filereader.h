#ifndef FILEREADER_H
#define FILEREADER_H

#include <QObject>
#include <QTimer>
#include <QFile>
#include <QTextStream>

class FileReader : public QObject
{
    Q_OBJECT
public:
    explicit FileReader(QObject *parent = nullptr);
    bool startRead(QFile *fileToRead);
    void setReadInterval(int newVal);
    void abortRead();
    bool readAllAtOnce(QFile *fileToRead);
signals:
    void lineReady(QString*, int*);
    void fileReadFinished();
    void textReady(QString*);
public slots:

private slots:
    void readLineSendLine();
private:
    QTimer fileReadTimer;
    int readInterval = 1; // ms
    QStringList readFileSplitLines;
    int lineReadIterator = 0;
};

#endif // FILEREADER_H
