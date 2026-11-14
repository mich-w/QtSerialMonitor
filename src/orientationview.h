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

    // Orbit camera around the board, controlled with the mouse:
    // drag = rotate, wheel = zoom, double-click = reset.
    double cameraAzimuth() const { return azimuthDeg; }
    double cameraElevation() const { return elevationDeg; }
    double cameraDistance() const { return distance; }
    void resetCamera();

    static constexpr double DefaultAzimuth = -146.3;  // behind and to the right of the board
    static constexpr double DefaultElevation = 28.0;  // looking down a little
    static constexpr double DefaultDistance = 4.9;
    static constexpr double MinElevation = -85.0;
    static constexpr double MaxElevation = 85.0;
    static constexpr double MinDistance = 2.0;
    static constexpr double MaxDistance = 15.0;

    // Rotates a body-frame vector into the world frame.
    static QVector3D rotate(const QVector3D &body, double rollDegrees, double pitchDegrees, double yawDegrees);

    QSize sizeHint() const override { return QSize(480, 360); }
    QSize minimumSizeHint() const override { return QSize(200, 160); }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    double rollDeg = 0.0;
    double pitchDeg = 0.0;
    double yawDeg = 0.0;
    bool hasData = false;

    double azimuthDeg = DefaultAzimuth;
    double elevationDeg = DefaultElevation;
    double distance = DefaultDistance;
    bool dragging = false;
    QPoint lastMousePosition;
};

#endif // ORIENTATIONVIEW_H
