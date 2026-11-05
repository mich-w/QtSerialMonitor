#include "receivepipeline.h"

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
    lineBuffer.clear();
    decoder.reset();
}

void ReceivePipeline::append(const QByteArray &bytes, bool isCompleteMessage)
{
    if (bytes.isEmpty())
        return;

    const QString text = decoder.decode(bytes);
    pendingBytes += bytes;
    pendingText += text;
    lineBuffer += text;

    if (isCompleteMessage && !lineBuffer.isEmpty() && !lineBuffer.endsWith('\n') && !lineBuffer.endsWith('\r'))
        lineBuffer += '\n';

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

    const int lastLineEnd = qMax(lineBuffer.lastIndexOf('\n'), lineBuffer.lastIndexOf('\r'));
    if (lastLineEnd >= 0)
    {
        const QString lines = lineBuffer.left(lastLineEnd + 1);
        lineBuffer.remove(0, lastLineEnd + 1);
        emit linesReceived(lines);
    }

    if (lineBuffer.isEmpty())
        partialLineTimer.stop();
    else
        partialLineTimer.start();
}

void ReceivePipeline::flushPartialLine()
{
    if (lineBuffer.isEmpty())
        return;

    const QString line = lineBuffer;
    lineBuffer.clear();
    emit linesReceived(line);
}
