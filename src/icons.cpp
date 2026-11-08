#include "icons.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QtMath>

namespace
{
    // All shapes are drawn on a 24x24 grid.
    void draw(QPainter &p, Icons::Shape shape, const QColor &color)
    {
        QPen pen(color, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        switch (shape)
        {
        case Icons::Shape::Connect: // plug
        {
            p.drawLine(QPointF(9, 3), QPointF(9, 8));
            p.drawLine(QPointF(15, 3), QPointF(15, 8));
            QPainterPath body;
            body.moveTo(6, 8);
            body.lineTo(18, 8);
            body.lineTo(18, 12);
            body.cubicTo(18, 15.5, 15.5, 17, 12, 17);
            body.cubicTo(8.5, 17, 6, 15.5, 6, 12);
            body.closeSubpath();
            p.drawPath(body);
            p.drawLine(QPointF(12, 17), QPointF(12, 21));
            break;
        }
        case Icons::Shape::Disconnect: // plug with slash
            draw(p, Icons::Shape::Connect, color);
            p.drawLine(QPointF(4, 4), QPointF(20, 20));
            break;
        case Icons::Shape::Pause:
            p.setBrush(color);
            p.drawRoundedRect(QRectF(6.5, 5, 3.5, 14), 1, 1);
            p.drawRoundedRect(QRectF(14, 5, 3.5, 14), 1, 1);
            break;
        case Icons::Shape::Play:
        {
            p.setBrush(color);
            QPolygonF triangle({QPointF(7, 5), QPointF(19, 12), QPointF(7, 19)});
            p.drawPolygon(triangle);
            break;
        }
        case Icons::Shape::Clear: // trash can
            p.drawLine(QPointF(4, 6.5), QPointF(20, 6.5));
            p.drawLine(QPointF(9.5, 6.5), QPointF(10, 3.5));
            p.drawLine(QPointF(10, 3.5), QPointF(14, 3.5));
            p.drawLine(QPointF(14, 3.5), QPointF(14.5, 6.5));
            p.drawRoundedRect(QRectF(6, 6.5, 12, 14), 2, 2);
            p.drawLine(QPointF(10, 10.5), QPointF(10, 16.5));
            p.drawLine(QPointF(14, 10.5), QPointF(14, 16.5));
            break;
        case Icons::Shape::Save: // floppy
            p.drawRoundedRect(QRectF(4, 4, 16, 16), 2, 2);
            p.drawRect(QRectF(8, 4, 8, 5));
            p.drawRoundedRect(QRectF(7.5, 13, 9, 7), 1, 1);
            break;
        case Icons::Shape::Image: // chart picture
            p.drawRoundedRect(QRectF(3.5, 4.5, 17, 15), 2, 2);
            p.drawPolyline(QPolygonF({QPointF(6.5, 16), QPointF(10, 11), QPointF(13, 14), QPointF(17.5, 8)}));
            break;
        case Icons::Shape::Sun:
            p.drawEllipse(QPointF(12, 12), 4, 4);
            for (auto i = 0; i < 8; ++i)
            {
                const double angle = i * M_PI / 4.0;
                p.drawLine(QPointF(12 + 7 * qCos(angle), 12 + 7 * qSin(angle)), QPointF(12 + 9.5 * qCos(angle), 12 + 9.5 * qSin(angle)));
            }
            break;
        case Icons::Shape::Moon:
        {
            QPainterPath moon;
            moon.addEllipse(QPointF(12, 12), 8, 8);
            QPainterPath cut;
            cut.addEllipse(QPointF(16.5, 8.5), 7, 7);
            p.drawPath(moon.subtracted(cut));
            break;
        }
        case Icons::Shape::Auto: // half-filled circle
        {
            p.drawEllipse(QPointF(12, 12), 8, 8);
            QPainterPath half;
            half.moveTo(12, 4);
            half.arcTo(QRectF(4, 4, 16, 16), 90, 180);
            half.closeSubpath();
            p.setBrush(color);
            p.drawPath(half);
            break;
        }
        case Icons::Shape::Demo: // sine wave
        {
            QPainterPath wave;
            wave.moveTo(3, 12);
            for (auto x = 3.0; x <= 21.0; x += 0.5)
                wave.lineTo(x, 12 - 6 * qSin((x - 3) / 18.0 * 2 * M_PI));
            p.drawPath(wave);
            break;
        }
        }
    }
}

QIcon Icons::make(Shape shape, const QColor &color)
{
    QIcon icon;
    for (const int size : {16, 24, 32, 48})
    {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.scale(size / 24.0, size / 24.0);
        draw(painter, shape, color);
        icon.addPixmap(pixmap);
    }
    return icon;
}
