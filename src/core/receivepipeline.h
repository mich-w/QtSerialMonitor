#ifndef RECEIVEPIPELINE_H
#define RECEIVEPIPELINE_H

#include "utf8decoder.h"

#include <QObject>
#include <QTimer>

// Batches incoming bytes from a Transport and splits them into
//  - received():      everything received since the last batch (raw bytes + decoded text), for display,
//  - linesReceived(): only complete lines, for parsing and logging.
// A trailing line without a newline is released after a short idle period, so
// devices that never terminate their last line still get parsed.
class ReceivePipeline : public QObject
{
    Q_OBJECT
public:
    explicit ReceivePipeline(QObject *parent = nullptr);

    void setBatchInterval(int milliseconds);
    void setPartialLineTimeout(int milliseconds);
    void reset();

public slots:
    void append(const QByteArray &bytes, bool isCompleteMessage = false);
    void flush();

signals:
    void received(const QByteArray &bytes, const QString &text);
    void linesReceived(const QString &lines);

private:
    void flushPartialLine();

    Utf8Decoder decoder;
    QByteArray pendingBytes;
    QString pendingText;
    QString lineBuffer;
    QTimer batchTimer;
    QTimer partialLineTimer;
};

#endif // RECEIVEPIPELINE_H
