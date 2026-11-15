#include "receivepipeline.h"

#include <QRegularExpression>
#include <QTime>

ReceivePipeline::ReceivePipeline(QObject *parent) : QObject(parent)
{
    batchTimer.setSingleShot(true);
    batchTimer.setInterval(1);
    partialLineTimer.setSingleShot(true);
    partialLineTimer.setInterval(100);

    connect(&batchTimer, &QTimer::timeout, this, &ReceivePipeline::flush);
    connect(&partialLineTimer, &QTimer::timeout, this, &ReceivePipeline::flushPartialLine);
}

void ReceivePipeline::setBatchInterval(int milliseconds)
{
    batchTimer.setInterval(qMax(1, milliseconds));
}

void ReceivePipeline::setPartialLineTimeout(int milliseconds)
{
    partialLineTimer.setInterval(qMax(1, milliseconds));
}

void ReceivePipeline::reset()
{
    batchTimer.stop();
    partialLineTimer.stop();
    pendingBytes.clear();
    pendingText.clear();
    partialLine.clear();
    pendingLines.clear();
    pendingTimes.clear();
    lastChunkTime = -1;
    decoder.reset();
}

void ReceivePipeline::setClock(std::function<long()> newClock)
{
    clock = std::move(newClock);
}

void ReceivePipeline::append(const QByteArray &bytes, bool isCompleteMessage)
{
    if (bytes.isEmpty())
        return;

    const long now = clock ? clock() : QTime::currentTime().msecsSinceStartOfDay();
    const QString text = decoder.decode(bytes);
    pendingBytes += bytes;
    pendingText += text;

    // Split off the lines completed by this chunk.
    QString buffer = partialLine + text;
    if (isCompleteMessage && !buffer.isEmpty() && !buffer.endsWith('\n') && !buffer.endsWith('\r'))
        buffer += '\n';
    const int lastLineEnd = qMax(buffer.lastIndexOf('\n'), buffer.lastIndexOf('\r'));
    partialLine = buffer.mid(lastLineEnd + 1);

    if (lastLineEnd >= 0)
    {
        static const QRegularExpression lineBreaks("[\\r\\n]+");
        const QStringList lines = buffer.left(lastLineEnd).split(lineBreaks, Qt::SkipEmptyParts);

        // They were transmitted one after another since the previous chunk - spread their times.
        const long window = lastChunkTime < 0 ? 0 : qBound(0L, now - lastChunkTime, MaxSpreadMs);
        for (auto i = 0; i < lines.count(); ++i)
        {
            pendingLines.append(lines[i]);
            pendingTimes.append(now - window * (lines.count() - 1 - i) / lines.count());
        }
    }
    lastChunkTime = now;

    // The timer only runs while there is data - no polling when idle.
    if (!batchTimer.isActive())
        batchTimer.start();
}

void ReceivePipeline::flush()
{
    batchTimer.stop();

    if (!pendingBytes.isEmpty())
    {
        const QByteArray bytes = pendingBytes;
        const QString text = pendingText;
        pendingBytes.clear();
        pendingText.clear();
        emit received(bytes, text);
    }

    if (!pendingLines.isEmpty())
    {
        const QString lines = pendingLines.join('\n') + '\n';
        const QList<long> times = pendingTimes;
        pendingLines.clear();
        pendingTimes.clear();
        emit linesReceived(lines, times);
    }

    if (partialLine.isEmpty())
        partialLineTimer.stop();
    else
        partialLineTimer.start();
}

void ReceivePipeline::flushPartialLine()
{
    if (partialLine.trimmed().isEmpty())
        return;

    const QString line = partialLine;
    partialLine.clear();
    emit linesReceived(line, {lastChunkTime});
}
