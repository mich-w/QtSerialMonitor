#include "demotransport.h"

#include <QRandomGenerator>
#include <QtMath>

#include <cmath>

DemoTransport::DemoTransport(QObject *parent) : Transport(parent)
{
    sampleTimer.setTimerType(Qt::PreciseTimer);
    connect(&sampleTimer, &QTimer::timeout, this, &DemoTransport::emitSample);
}

bool DemoTransport::start(int requestedRateHz)
{
    rateHz = qBound(1, requestedRateHz, 1000);
    clock.start();
    samplesSent = 0;
    sampleTimer.start(1000 / rateHz);
    return true;
}

void DemoTransport::stop()
{
    sampleTimer.stop();
}

bool DemoTransport::isOpen()
{
    return sampleTimer.isActive();
}

int DemoTransport::rate() const
{
    return rateHz;
}

bool DemoTransport::send(const QByteArray &bytes)
{
    if (!isOpen())
        return false;

    const QByteArray command = bytes.trimmed();
    if (!command.isEmpty())
        emit dataReceived("demo> received: " + command + "\r\n", false);
    return true;
}

QByteArray DemoTransport::sampleLine(double seconds, double noise, bool orientation)
{
    constexpr double period = 5.0; // seconds, like the README animation
    const double phase = 2.0 * M_PI * seconds / period;

    const double sin1 = qSin(phase);
    const double sin2 = 0.7 * qSin(phase - M_PI / 3.0);
    const double noisy = 0.3 * qSin(2.0 * M_PI * seconds / 13.0) + noise;

    QString line = QString("sin1 %1 sin2 %2 noisy %3")
                       .arg(sin1, 0, 'f', 2)
                       .arg(sin2, 0, 'f', 2)
                       .arg(noisy, 0, 'f', 2);

    if (orientation)
    {
        // A gently rocking board that slowly turns around - like an IMU on a boat.
        const double roll = 35.0 * qSin(2.0 * M_PI * seconds / 6.0);
        const double pitch = 20.0 * qSin(2.0 * M_PI * seconds / 9.0 + 1.0);
        const double yaw = std::fmod(seconds * 24.0, 360.0) - 180.0;
        line += QString(" roll %1 pitch %2 yaw %3").arg(roll, 0, 'f', 1).arg(pitch, 0, 'f', 1).arg(yaw, 0, 'f', 1);
    }

    return (line + "\r\n").toLatin1();
}

void DemoTransport::setOrientationOutput(bool enabled)
{
    withOrientation = enabled;
}

// Sends every sample that is due by the clock, like a real device that keeps transmitting
// while the app is busy - so the demo rate stays exact even if timer ticks are delayed.
void DemoTransport::emitSample()
{
    const qint64 due = clock.elapsed() * rateHz / 1000 + 1;
    if (due - samplesSent > MaxCatchUp)
        samplesSent = due - MaxCatchUp; // after a long stall, skip ahead instead of flooding

    QByteArray chunk;
    for (; samplesSent < due; ++samplesSent)
    {
        const double seconds = double(samplesSent) / rateHz; // sample time, not arrival time
        const double noise = (QRandomGenerator::global()->generateDouble() - 0.5) * 0.1;
        chunk += sampleLine(seconds, noise, withOrientation);
    }
    if (!chunk.isEmpty())
        emit dataReceived(chunk, false);
}
