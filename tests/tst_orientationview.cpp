#include "orientationview.h"

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
