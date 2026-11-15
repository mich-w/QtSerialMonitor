#ifndef RECEIVEPIPELINE_H
#define RECEIVEPIPELINE_H

#include "utf8decoder.h"

#include <QList>
#include <QObject>
#include <QStringList>
#include <QTimer>

#include <functional>

// Batches incoming bytes from a Transport and splits them into
//  - received():      everything received since the last batch (raw bytes + decoded text), for display,
//  - linesReceived(): only complete lines, for parsing and logging, each with its arrival time.
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

    // Milliseconds since midnight; replaceable for tests.
    void setClock(std::function<long()> clock);

    // Lines that arrive together in one chunk are spread over the time since the previous chunk,
    // at most this long, so they don't all get the same time stamp (issue #8).
    static constexpr long MaxSpreadMs = 100;

public slots:
    void append(const QByteArray &bytes, bool isCompleteMessage = false);
    void flush();

signals:
    void received(const QByteArray &bytes, const QString &text);
    void linesReceived(const QString &lines, const QList<long> &arrivalTimes); // one time per non-empty line

private:
    void flushPartialLine();

    Utf8Decoder decoder;
    QByteArray pendingBytes;
    QString pendingText;
    QString partialLine;      // text after the last line break
    QStringList pendingLines; // complete, non-empty lines not yet emitted
    QList<long> pendingTimes; // their arrival times
    std::function<long()> clock;
    long lastChunkTime = -1;
    QTimer batchTimer;
    QTimer partialLineTimer;
};

#endif // RECEIVEPIPELINE_H
