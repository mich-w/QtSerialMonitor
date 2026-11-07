#include "demotransport.h"

#include <QRandomGenerator>
#include <QtMath>

DemoTransport::DemoTransport(QObject *parent) : Transport(parent)
{
    sampleTimer.setTimerType(Qt::PreciseTimer);
    connect(&sampleTimer, &QTimer::timeout, this, &DemoTransport::emitSample);
}

bool DemoTransport::start(int requestedRateHz)
{
    rateHz = qBound(1, requestedRateHz, 1000);
    clock.start();
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

QByteArray DemoTransport::sampleLine(double seconds, double noise)
{
    constexpr double period = 5.0; // seconds, like the README animation
    const double phase = 2.0 * M_PI * seconds / period;

    const double sin1 = qSin(phase);
    const double sin2 = 0.7 * qSin(phase - M_PI / 3.0);
    const double noisy = 0.3 * qSin(2.0 * M_PI * seconds / 13.0) + noise;

    return QString("sin1 %1 sin2 %2 noisy %3\r\n")
        .arg(sin1, 0, 'f', 2)
        .arg(sin2, 0, 'f', 2)
        .arg(noisy, 0, 'f', 2)
        .toLatin1();
}

void DemoTransport::emitSample()
{
    // Use the real elapsed time so timer jitter does not distort the waves.
    const double seconds = clock.elapsed() / 1000.0;
    const double noise = (QRandomGenerator::global()->generateDouble() - 0.5) * 0.1;
    emit dataReceived(sampleLine(seconds, noise), false);
}
