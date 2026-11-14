#include "orientationview.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QtTest>

// Body frame: +X forward, +Y left, +Z up.
class TestOrientationView : public QObject
{
    Q_OBJECT

    static void compare(const QVector3D &actual, const QVector3D &expected)
    {
        QVERIFY2((actual - expected).length() < 1e-4f,
                 qPrintable(QString("(%1, %2, %3) != (%4, %5, %6)").arg(actual.x()).arg(actual.y()).arg(actual.z())
                                .arg(expected.x()).arg(expected.y()).arg(expected.z())));
    }

    // QTest::mouseMove moves the real cursor on Qt 5 (no event when headless), so send the event directly.
    static void dragTo(QWidget &widget, const QPoint &position, Qt::MouseButtons buttons = Qt::LeftButton)
    {
        QMouseEvent event(QEvent::MouseMove, QPointF(position), QPointF(widget.mapToGlobal(position)), Qt::NoButton, buttons, Qt::NoModifier);
        QApplication::sendEvent(&widget, &event);
    }

private slots:
    void identity() { compare(OrientationView::rotate({1, 2, 3}, 0, 0, 0), {1, 2, 3}); }

    void pitchUpRaisesNose() { compare(OrientationView::rotate({1, 0, 0}, 0, 90, 0), {0, 0, 1}); }

    void rollRightLowersRightSide() { compare(OrientationView::rotate({0, -1, 0}, 90, 0, 0), {0, 0, -1}); }

    void yawIsClockwiseFromAbove() { compare(OrientationView::rotate({1, 0, 0}, 0, 0, 90), {0, -1, 0}); } // nose turns right

    void yawThenPitchOrder()
    {
        // Heading 90 deg (facing right) and nose up 90 deg: nose points straight up regardless of heading.
        compare(OrientationView::rotate({1, 0, 0}, 0, 90, 90), {0, 0, 1});
        // Rolling does not move the nose.
        compare(OrientationView::rotate({1, 0, 0}, 45, 0, 0), {1, 0, 0});
    }

    void dragRotatesAndClampsElevation()
    {
        OrientationView view;
        view.resize(300, 200);
        view.show();

        QTest::mousePress(&view, Qt::LeftButton, {}, QPoint(100, 100));
        dragTo(view, QPoint(150, 100));
        QVERIFY(view.cameraAzimuth() != OrientationView::DefaultAzimuth);
        QCOMPARE(view.cameraElevation(), OrientationView::DefaultElevation);

        dragTo(view, QPoint(150, 1000)); // far down: elevation stops at the limit
        QCOMPARE(view.cameraElevation(), OrientationView::MaxElevation);
        QTest::mouseRelease(&view, Qt::LeftButton, {}, QPoint(150, 1000));

        const double azimuth = view.cameraAzimuth();
        dragTo(view, QPoint(10, 10), Qt::NoButton); // not dragging anymore
        QCOMPARE(view.cameraAzimuth(), azimuth);
    }

    void wheelZoomsWithinLimits()
    {
        OrientationView view;
        view.resize(300, 200);

        auto wheel = [&view](int delta)
        {
            QWheelEvent event(QPointF(150, 100), QPointF(view.mapToGlobal(QPoint(150, 100))), QPoint(), QPoint(0, delta),
                              Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
            QApplication::sendEvent(&view, &event);
        };

        wheel(120);
        QVERIFY(view.cameraDistance() < OrientationView::DefaultDistance); // wheel up = closer
        for (int i = 0; i < 50; ++i)
            wheel(120);
        QCOMPARE(view.cameraDistance(), OrientationView::MinDistance);
        for (int i = 0; i < 50; ++i)
            wheel(-120);
        QCOMPARE(view.cameraDistance(), OrientationView::MaxDistance);
    }

    void doubleClickResets()
    {
        OrientationView view;
        view.resize(300, 200);
        view.show();
        QTest::mousePress(&view, Qt::LeftButton, {}, QPoint(100, 100));
        dragTo(view, QPoint(180, 140));
        QTest::mouseRelease(&view, Qt::LeftButton, {}, QPoint(180, 140));
        QTest::mouseDClick(&view, Qt::LeftButton, {}, QPoint(180, 140));
        QCOMPARE(view.cameraAzimuth(), OrientationView::DefaultAzimuth);
        QCOMPARE(view.cameraElevation(), OrientationView::DefaultElevation);
        QCOMPARE(view.cameraDistance(), OrientationView::DefaultDistance);
    }

    void paintsWithoutData()
    {
        OrientationView view;
        view.resize(300, 200);
        QImage image(view.size(), QImage::Format_ARGB32);
        view.render(&image);
        view.setOrientation(10, 20, 30);
        view.render(&image);
        QCOMPARE(view.yaw(), 30.0);
    }
};

QTEST_MAIN(TestOrientationView)
#include "tst_orientationview.moc"
