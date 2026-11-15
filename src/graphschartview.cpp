#include "graphschartview.h"
#include "qcustomplot.h"
#include "theme.h"

#include <QMouseEvent>
#include <QRubberBand>
#include <QWheelEvent>
#include <QLineSeries>
#include <QQuickItem>
#include <QValueAxis>

#include <cmath>
#include <limits>

GraphsChartView::GraphsChartView(QWidget *parent) : QQuickWidget(parent)
{
    setObjectName("graphsChartView");
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSource(QUrl("qrc:/qml/GraphsChart.qml"));

    if (QQuickItem *root = rootObject())
    {
        graphsView = root->findChild<QObject *>("graphsView");
        axisX = root->findChild<QValueAxis *>("axisX");
        axisY = root->findChild<QValueAxis *>("axisY");
    }
}

bool GraphsChartView::isReady() const
{
    return graphsView && axisX && axisY;
}

QString GraphsChartView::errorString() const
{
    QStringList messages;
    for (const auto &error : errors())
        messages.append(error.toString());
    return messages.isEmpty() ? QString("Qt Graphs QML module could not be loaded") : messages.join("; ");
}

int GraphsChartView::pointCount(const QString &name) const
{
    const auto it = series.constFind(name);
    return it == series.constEnd() ? 0 : int((*it)->count());
}

QLineSeries *GraphsChartView::seriesFor(const QString &name, int colorIndex)
{
    if (QLineSeries *existing = series.value(name))
        return existing;

    auto *line = new QLineSeries(graphsView);
    line->setName(name);
    line->setColor(ThemeManager::instance().seriesColor(colorIndex));
    line->setWidth(fast ? 1.0 : 2.0);
    QMetaObject::invokeMethod(graphsView, "addSeries", Q_ARG(QObject *, line));
    series.insert(name, line);
    return line;
}

void GraphsChartView::sync(QCustomPlot *source)
{
    this->source = source;
    if (!isReady())
        return;

    const QCPRange xRange = source->xAxis->range();
    const QCPRange yRange = source->yAxis->range();

    // About two points per horizontal pixel is all a line chart can show.
    const int maxPoints = qMax(200, width() * 2);

    QSet<QString> present;
    QVariantList newLegend;
    for (auto i = 0; i < source->graphCount(); ++i)
    {
        QCPGraph *graph = source->graph(i);
        present.insert(graph->name());
        const int colorIndex = graph->property("colorIndex").toInt();
        QLineSeries *line = seriesFor(graph->name(), colorIndex);
        line->setVisible(graph->visible());
        line->setColor(ThemeManager::instance().seriesColor(colorIndex)); // follows theme changes
        newLegend.append(QVariantMap{{"name", graph->name()}, {"color", line->color()}, {"visible", graph->visible()}});

        // Visible window plus one point on each side so lines reach the edges.
        auto begin = graph->data()->findBegin(xRange.lower, true);
        auto end = graph->data()->findEnd(xRange.upper, true);
        const int count = int(end - begin);

        QList<QPointF> points;
        if (count <= maxPoints)
        {
            points.reserve(count);
            for (auto it = begin; it != end; ++it)
                points.append(QPointF(it->key, it->value));
        }
        else
        {
            // Min/max per bucket keeps peaks visible while dropping invisible detail.
            const int buckets = maxPoints / 2;
            points.reserve(maxPoints);
            for (int bucket = 0; bucket < buckets; ++bucket)
            {
                auto from = begin + qint64(count) * bucket / buckets;
                auto to = begin + qint64(count) * (bucket + 1) / buckets;
                if (from == to)
                    continue;
                auto minIt = from, maxIt = from;
                for (auto it = from; it != to; ++it)
                {
                    if (it->value < minIt->value)
                        minIt = it;
                    if (it->value > maxIt->value)
                        maxIt = it;
                }
                const auto first = minIt->key <= maxIt->key ? minIt : maxIt;
                const auto second = first == minIt ? maxIt : minIt;
                points.append(QPointF(first->key, first->value));
                if (second != first)
                    points.append(QPointF(second->key, second->value));
            }
        }
        line->replace(points);
    }

    // Drop series whose graph is gone.
    for (auto it = series.begin(); it != series.end();)
    {
        if (!present.contains(it.key()))
        {
            QMetaObject::invokeMethod(graphsView, "removeSeries", Q_ARG(QObject *, it.value()));
            it.value()->deleteLater();
            it = series.erase(it);
        }
        else
            ++it;
    }

    // Only touch the legend when it changes - rebuilding the QML delegates every frame is wasteful.
    if (newLegend != legend)
    {
        legend = newLegend;
        if (QQuickItem *root = rootObject())
            root->setProperty("legendModel", legend);
    }

    axisX->setMin(xRange.lower); // seconds of the day; labels and vertical grid are drawn in QML
    axisX->setMax(xRange.upper);
    if (QQuickItem *root = rootObject())
    {
        root->setProperty("xMin", xRange.lower);
        root->setProperty("xMax", xRange.upper);
        root->setProperty("yMin", yRange.lower);
        root->setProperty("yMax", yRange.upper);

        // Marker of the Samples table: a QCustomPlot item, mirrored here.
        double marker = std::numeric_limits<double>::quiet_NaN();
        auto *line = source->findChild<QCPItemStraightLine *>("sampleMarker", Qt::FindDirectChildrenOnly);
        if (line && line->visible())
            marker = line->point1->key();
        root->setProperty("markerX", marker);
    }
    axisY->setMin(yRange.lower);
    axisY->setMax(yRange.upper);
}

void GraphsChartView::applyTheme(bool fastLines)
{
    fast = fastLines;
    const auto &colors = ThemeManager::instance().colors();

    if (QQuickItem *root = rootObject())
    {
        QColor legendBackground = colors.chartBackground;
        legendBackground.setAlpha(225);
        root->setProperty("backgroundColor", colors.chartBackground);
        root->setProperty("gridColor", colors.chartGrid);
        root->setProperty("axisColor", colors.chartAxis);
        root->setProperty("textColor", colors.text);
        root->setProperty("mutedTextColor", colors.mutedText);
        root->setProperty("legendBackground", legendBackground);
        root->setProperty("legendBorder", colors.border);
        root->setProperty("markerColor", colors.accent);
    }

    // Series are created with these, but the theme or line quality may change later.
    // Colours follow the QCustomPlot graphs' colour index; re-sync assigns them on the next tick.
    for (QLineSeries *line : std::as_const(series))
        line->setWidth(fast ? 1.0 : 2.0);
}

void GraphsChartView::setLegendVisible(bool visible)
{
    if (QQuickItem *root = rootObject())
        root->setProperty("legendVisible", visible);
}

// ----- Mouse interaction: same gestures as the QCustomPlot chart ----- //

QRectF GraphsChartView::plotArea() const
{
    const QRectF area = graphsView ? graphsView->property("plotArea").toRectF() : QRectF();
    return area.isValid() ? area : QRectF(rect());
}

QPointF GraphsChartView::toData(const QPointF &pixel) const
{
    const QRectF area = plotArea();
    const QCPRange x = source->xAxis->range();
    const QCPRange y = source->yAxis->range();
    return QPointF(x.lower + (pixel.x() - area.left()) / area.width() * x.size(),
                   y.upper - (pixel.y() - area.top()) / area.height() * y.size());
}

// Interactions also work while the chart is paused, so draw right away instead of on the refresh tick.
void GraphsChartView::redrawNow()
{
    if (source)
        sync(source);
}

void GraphsChartView::mousePressEvent(QMouseEvent *event)
{
    if (!source)
        return;

    pressPosition = lastPosition = event->pos();
    dragButton = event->button();
    horizontalDragDistance = 0;

    if (dragButton == Qt::LeftButton)
        setCursor(Qt::ClosedHandCursor);
    else if (dragButton == Qt::MiddleButton)
    {
        if (!rubberBand)
            rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
        rubberBand->setGeometry(QRect(pressPosition, QSize()));
        rubberBand->show();
    }
    else if (dragButton == Qt::RightButton)
        emit contextMenuRequested(event->globalPosition().toPoint());
}

void GraphsChartView::mouseMoveEvent(QMouseEvent *event)
{
    if (!source)
        return;

    if (dragButton == Qt::LeftButton)
    {
        // Pan: move both axes by the mouse movement converted to data units.
        const QRectF area = plotArea();
        const QPoint delta = event->pos() - lastPosition;
        const double dx = -delta.x() / area.width() * source->xAxis->range().size();
        const double dy = delta.y() / area.height() * source->yAxis->range().size();
        source->xAxis->moveRange(dx);
        source->yAxis->moveRange(dy);

        horizontalDragDistance += delta.x();
        if (qAbs(horizontalDragDistance) > 200)
            emit autoTrackOffRequested();

        lastPosition = event->pos();
        redrawNow();
    }
    else if (dragButton == Qt::MiddleButton && rubberBand)
        rubberBand->setGeometry(QRect(pressPosition, event->pos()).normalized());
}

void GraphsChartView::mouseReleaseEvent(QMouseEvent *event)
{
    if (source && dragButton == Qt::MiddleButton && rubberBand)
    {
        rubberBand->hide();
        const QRect selection = QRect(pressPosition, event->pos()).normalized();
        if (selection.width() > 4 && selection.height() > 4) // zoom into the rectangle
        {
            const QPointF topLeft = toData(selection.topLeft());
            const QPointF bottomRight = toData(selection.bottomRight());
            source->xAxis->setRange(topLeft.x(), bottomRight.x());
            source->yAxis->setRange(bottomRight.y(), topLeft.y());
            redrawNow();
        }
    }
    dragButton = Qt::NoButton;
    unsetCursor();
}

void GraphsChartView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!source)
        return;

    if (event->button() == Qt::LeftButton) // fit all data
    {
        source->rescaleAxes(true);
        source->yAxis->scaleRange(1.20);
        redrawNow();
    }
    else if (event->button() == Qt::MiddleButton)
        emit autoTrackToggleRequested();
}

void GraphsChartView::wheelEvent(QWheelEvent *event)
{
    if (!source)
        return;

    const double steps = event->angleDelta().y() / 120.0; // touchpads send fractions of a notch
    if (steps == 0.0)
        return;

    // Zoom both axes around the point under the cursor (wheel up = zoom in), like QCustomPlot.
    const double factor = std::pow(0.85, steps);
    const QPointF centre = toData(event->position());
    source->xAxis->scaleRange(factor, centre.x());
    source->yAxis->scaleRange(factor, centre.y());
    redrawNow();
    event->accept();
}
