#include "orientationview.h"
#include "theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>

#include <algorithm>
#include <array>
#include <optional>
#include <vector>

namespace
{
    // Fixed "chase camera": behind, slightly to the right of and above the board.
    struct Camera
    {
        QVector3D eye{-3.6f, -2.4f, 2.2f};
        QVector3D target{0.0f, 0.0f, -0.1f};
        QVector3D right, up, forward;

        Camera()
        {
            forward = (target - eye).normalized();
            right = QVector3D::crossProduct(forward, QVector3D(0, 0, 1)).normalized();
            up = QVector3D::crossProduct(right, forward);
        }

        // Returns view-space depth (larger = further away) and the projected point.
        std::optional<std::pair<double, QPointF>> project(const QVector3D &world, const QPointF &centre, double focal) const
        {
            const QVector3D d = world - eye;
            const double depth = QVector3D::dotProduct(d, forward);
            if (depth < 0.1)
                return std::nullopt;
            const double x = QVector3D::dotProduct(d, right);
            const double y = QVector3D::dotProduct(d, up);
            return std::make_pair(depth, QPointF(centre.x() + focal * x / depth, centre.y() - focal * y / depth));
        }
    };

    QColor mix(const QColor &a, const QColor &b, double t)
    {
        return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t, a.greenF() + (b.greenF() - a.greenF()) * t,
                                a.blueF() + (b.blueF() - a.blueF()) * t);
    }
}

OrientationView::OrientationView(QWidget *parent) : QWidget(parent)
{
    setObjectName("orientationView");
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void OrientationView::setOrientation(double rollDegrees, double pitchDegrees, double yawDegrees)
{
    rollDeg = rollDegrees;
    pitchDeg = pitchDegrees;
    yawDeg = yawDegrees;
    hasData = true;
    update();
}

void OrientationView::setHasData(bool newHasData)
{
    hasData = newHasData;
    update();
}

QVector3D OrientationView::rotate(const QVector3D &body, double rollDegrees, double pitchDegrees, double yawDegrees)
{
    const double r = qDegreesToRadians(rollDegrees);
    const double p = qDegreesToRadians(-pitchDegrees); // nose up = rotate +X towards +Z
    const double y = qDegreesToRadians(-yawDegrees);   // clockwise from above

    // Roll about X
    double x1 = body.x();
    double y1 = body.y() * qCos(r) - body.z() * qSin(r);
    double z1 = body.y() * qSin(r) + body.z() * qCos(r);
    // Pitch about Y
    double x2 = x1 * qCos(p) + z1 * qSin(p);
    double y2 = y1;
    double z2 = -x1 * qSin(p) + z1 * qCos(p);
    // Yaw about Z
    double x3 = x2 * qCos(y) - y2 * qSin(y);
    double y3 = x2 * qSin(y) + y2 * qCos(y);
    return QVector3D(float(x3), float(y3), float(z2));
}

void OrientationView::paintEvent(QPaintEvent *)
{
    const auto &colors = ThemeManager::instance().colors();
    const auto &theme = ThemeManager::instance();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), colors.chartBackground);

    const Camera camera;
    const QPointF centre(width() / 2.0, height() / 2.0 + height() * 0.05);
    const double focal = qMin(width(), height()) * 1.55;
    auto project = [&](const QVector3D &world) { return camera.project(world, centre, focal); };

    // ----- Ground grid (horizon reference) ----- //
    const float groundZ = -0.9f;
    painter.setPen(QPen(colors.chartGrid, 1));
    for (int i = -4; i <= 4; ++i)
    {
        const float t = i * 0.5f;
        for (const auto &line : {std::make_pair(QVector3D(t, -2, groundZ), QVector3D(t, 2, groundZ)),
                                 std::make_pair(QVector3D(-2, t, groundZ), QVector3D(2, t, groundZ))})
        {
            const auto a = project(line.first), b = project(line.second);
            if (a && b)
                painter.drawLine(a->second, b->second);
        }
    }

    // ----- Board: a box 2.0 x 1.3 x 0.16 ----- //
    const float hx = 1.0f, hy = 0.65f, hz = 0.08f;
    std::array<QVector3D, 8> corners = {QVector3D(-hx, -hy, -hz), QVector3D(hx, -hy, -hz), QVector3D(hx, hy, -hz), QVector3D(-hx, hy, -hz),
                                        QVector3D(-hx, -hy, hz), QVector3D(hx, -hy, hz), QVector3D(hx, hy, hz), QVector3D(-hx, hy, hz)};
    for (auto &corner : corners)
        corner = rotate(corner, rollDeg, pitchDeg, yawDeg);

    struct Face
    {
        std::array<int, 4> index;
        QVector3D normalBody;
        bool top;
    };
    const std::array<Face, 6> faces = {{
        {{4, 5, 6, 7}, QVector3D(0, 0, 1), true},   // top
        {{0, 3, 2, 1}, QVector3D(0, 0, -1), false}, // bottom
        {{1, 2, 6, 5}, QVector3D(1, 0, 0), false},  // front
        {{0, 4, 7, 3}, QVector3D(-1, 0, 0), false}, // back
        {{0, 1, 5, 4}, QVector3D(0, -1, 0), false}, // right
        {{3, 7, 6, 2}, QVector3D(0, 1, 0), false},  // left
    }};

    const QVector3D light = QVector3D(-0.4f, -0.5f, 1.0f).normalized();
    const QColor boardColor = colors.dark ? QColor("#2f7d4f") : QColor("#2e8b57"); // PCB green
    const QColor edgeColor = colors.dark ? QColor("#1d4f32") : QColor("#1f5f3b");

    struct DrawFace
    {
        double depth;
        QPolygonF polygon;
        QColor color;
        bool top;
    };
    std::vector<DrawFace> drawFaces;
    for (const auto &face : faces)
    {
        const QVector3D normal = rotate(face.normalBody, rollDeg, pitchDeg, yawDeg);
        QVector3D centreWorld;
        for (int index : face.index)
            centreWorld += corners[size_t(index)] / 4.0f;
        if (QVector3D::dotProduct(normal, camera.eye - centreWorld) <= 0)
            continue; // back face

        QPolygonF polygon;
        double depth = 0;
        bool visible = true;
        for (int index : face.index)
        {
            const auto p = project(corners[size_t(index)]);
            if (!p)
            {
                visible = false;
                break;
            }
            polygon << p->second;
            depth += p->first / 4.0;
        }
        if (!visible)
            continue;

        const double shade = 0.45 + 0.55 * qMax(0.0f, QVector3D::dotProduct(normal, light));
        const QColor base = face.top ? boardColor : edgeColor;
        drawFaces.push_back({depth, polygon, mix(colors.chartBackground, base, 0.35 + 0.65 * shade), face.top});
    }
    std::sort(drawFaces.begin(), drawFaces.end(), [](const DrawFace &a, const DrawFace &b) { return a.depth > b.depth; });

    for (const auto &face : drawFaces)
    {
        painter.setPen(QPen(mix(face.color, colors.text, 0.25), 1.2));
        painter.setBrush(face.color);
        painter.drawPolygon(face.polygon);

        if (face.top)
        {
            // Arrow on the top surface pointing forward (+X).
            const float z = hz + 0.001f;
            const std::array<QVector3D, 7> arrow = {QVector3D(0.75f, 0, z), QVector3D(0.25f, 0.32f, z), QVector3D(0.25f, 0.12f, z),
                                                    QVector3D(-0.6f, 0.12f, z), QVector3D(-0.6f, -0.12f, z), QVector3D(0.25f, -0.12f, z),
                                                    QVector3D(0.25f, -0.32f, z)};
            QPolygonF arrowPolygon;
            for (const auto &point : arrow)
                if (const auto p = project(rotate(point, rollDeg, pitchDeg, yawDeg)))
                    arrowPolygon << p->second;
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(255, 255, 255, 200));
            painter.drawPolygon(arrowPolygon);
        }
    }

    // ----- Body axes ----- //
    const auto origin = project(QVector3D(0, 0, 0));
    struct Axis
    {
        QVector3D direction;
        QString name;
        QColor color;
    };
    const std::array<Axis, 3> axes = {{{QVector3D(1.6f, 0, 0), "X", theme.seriesColor(1)},
                                       {QVector3D(0, 1.2f, 0), "Y", theme.seriesColor(2)},
                                       {QVector3D(0, 0, 1.0f), "Z", theme.seriesColor(0)}}};
    QFont labelFont = font();
    labelFont.setBold(true);
    painter.setFont(labelFont);
    for (const auto &axis : axes)
    {
        const auto tip = project(rotate(axis.direction, rollDeg, pitchDeg, yawDeg));
        if (!origin || !tip)
            continue;
        painter.setPen(QPen(axis.color, 2.5, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(origin->second, tip->second);
        painter.setBrush(axis.color);
        painter.drawEllipse(tip->second, 3.5, 3.5);
        painter.drawText(tip->second + QPointF(6, -6), axis.name);
    }

    // ----- Readout ----- //
    painter.setFont(font());
    painter.setPen(colors.text);
    const QFontMetrics metrics(font());
    const int lineHeight = metrics.height();
    const QPoint textOrigin(12, 12 + metrics.ascent());
    if (hasData)
    {
        const QStringList lines = {QString("Roll   %1°").arg(rollDeg, 7, 'f', 1), QString("Pitch  %1°").arg(pitchDeg, 7, 'f', 1),
                                   QString("Yaw    %1°").arg(yawDeg, 7, 'f', 1)};
        QFont mono("monospace");
        mono.setStyleHint(QFont::Monospace);
        mono.setPointSizeF(font().pointSizeF());
        painter.setFont(mono);
        for (int i = 0; i < lines.count(); ++i)
            painter.drawText(textOrigin + QPoint(0, i * lineHeight), lines[i]);
    }
    else
    {
        painter.setPen(colors.mutedText);
        painter.drawText(textOrigin, "Waiting for roll / pitch / yaw values...");
    }
}
