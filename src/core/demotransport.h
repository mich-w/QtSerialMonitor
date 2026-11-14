#ifndef DEMOTRANSPORT_H
#define DEMOTRANSPORT_H

#include "transport.h"

#include <QElapsedTimer>
#include <QTimer>

// Simulated serial device for demos and testing without hardware.
// Emits lines such as "sin1 0.79 sin2 0.40 noisy 0.12\r\n" at a fixed rate,
// like an Arduino sketch printing sensor values. Text sent to it is echoed back.
class DemoTransport : public Transport
{
    Q_OBJECT
public:
    static constexpr int DefaultRateHz = 50;

    explicit DemoTransport(QObject *parent = nullptr);

    bool start(int rateHz = DefaultRateHz);
    void stop();
    bool isOpen() override;
    bool send(const QByteArray &bytes);
    int rate() const;

    // Also send "roll", "pitch" and "yaw" (degrees) - used while the 3D orientation view is shown.
    void setOrientationOutput(bool enabled);
    bool orientationOutput() const { return withOrientation; }

    // One line of output for time t (seconds) - deterministic apart from the noise term.
    static QByteArray sampleLine(double seconds, double noise = 0.0, bool orientation = false);

private slots:
    void emitSample();

private:
    QTimer sampleTimer;
    QElapsedTimer clock;
    int rateHz = DefaultRateHz;
    bool withOrientation = false;
};

#endif // DEMOTRANSPORT_H
