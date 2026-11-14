#ifndef ORIENTATIONVIEW_H
#define ORIENTATIONVIEW_H

#include <QVector3D>
#include <QWidget>

// Lightweight 3D view of a sensor board rotated by roll / pitch / yaw (degrees), drawn with QPainter.
//
// Body frame: +X forward, +Y left, +Z up.
//   roll  - about the forward axis, positive = right side down
//   pitch - about the lateral axis, positive = nose up
//   yaw   - heading about the vertical axis, positive = clockwise seen from above
// Applied in the usual aerospace order: yaw, then pitch, then roll.
class OrientationView : public QWidget
{
    Q_OBJECT
public:
    explicit OrientationView(QWidget *parent = nullptr);

    void setOrientation(double rollDegrees, double pitchDegrees, double yawDegrees);
    void setHasData(bool hasData);
    double roll() const { return rollDeg; }
    double pitch() const { return pitchDeg; }
    double yaw() const { return yawDeg; }

    // Rotates a body-frame vector into the world frame.
    static QVector3D rotate(const QVector3D &body, double rollDegrees, double pitchDegrees, double yawDegrees);

    QSize sizeHint() const override { return QSize(480, 360); }
    QSize minimumSizeHint() const override { return QSize(200, 160); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double rollDeg = 0.0;
    double pitchDeg = 0.0;
    double yawDeg = 0.0;
    bool hasData = false;
};

#endif // ORIENTATIONVIEW_H
