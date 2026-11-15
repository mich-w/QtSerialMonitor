#ifndef GRAPHSCHARTVIEW_H
#define GRAPHSCHARTVIEW_H

#include <QHash>
#include <QPointer>
#include <QQuickWidget>

class QCustomPlot;
class QRubberBand;
class QLineSeries;
class QValueAxis;

// Experimental chart engine: draws the chart with Qt Graphs (GPU, Qt Quick).
// QCustomPlot stays the data store - sync() copies the visible part of every graph and
// the current axis ranges, so pause, history, statistics etc. keep working unchanged.
class GraphsChartView : public QQuickWidget
{
    Q_OBJECT
public:
    explicit GraphsChartView(QWidget *parent = nullptr);

    bool isReady() const; // QML loaded and the graph found
    QString errorString() const;

    void sync(QCustomPlot *source);
    void applyTheme(bool fastLines);
    void setLegendVisible(bool visible);

    int seriesCount() const { return int(series.count()); }
    int pointCount(const QString &name) const;

signals:
    // Mouse interactions change the QCustomPlot axis ranges (the data model) directly.
    void autoTrackOffRequested();    // long horizontal drag, like the QCustomPlot chart
    void autoTrackToggleRequested(); // middle-button double-click
    void contextMenuRequested(const QPoint &globalPos);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QLineSeries *seriesFor(const QString &name, int colorIndex);
    QRectF plotArea() const;
    QPointF toData(const QPointF &pixel) const; // widget pixel -> (seconds, value)
    void redrawNow();

    QPointer<QCustomPlot> source;
    QRubberBand *rubberBand = nullptr;
    QPoint pressPosition;
    QPoint lastPosition;
    Qt::MouseButton dragButton = Qt::NoButton;
    int horizontalDragDistance = 0;

    QPointer<QObject> graphsView;
    QPointer<QValueAxis> axisX;
    QPointer<QValueAxis> axisY;
    QHash<QString, QLineSeries *> series;
    bool fast = false;
    QVariantList legend;
};

#endif // GRAPHSCHARTVIEW_H
