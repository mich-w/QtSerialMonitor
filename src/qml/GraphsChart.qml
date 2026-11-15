// Chart drawn with Qt Graphs. Qt Graphs only draws the data lines; the axes box, ticks, labels,
// grid and legend are drawn here to look like the QCustomPlot chart (and because Qt Graphs places
// some axis labels incorrectly). Data, ranges and colours are pushed in from C++ (GraphsChartView).
import QtQuick
import QtGraphs

Rectangle {
    id: root

    property color backgroundColor: "white"
    property color gridColor: "#eceef1"
    property color axisColor: "#9ca3af"
    property color textColor: "#1f2328"
    property color mutedTextColor: "#6b7280"
    property color legendBackground: "#e6ffffff"
    property color legendBorder: "#d0d5dd"
    property bool legendVisible: true
    property real markerX: NaN // time selected in the Samples table, NaN = none
    property color markerColor: "#3b82f6"
    property var legendModel: [] // [{ name, color, visible }], set from C++

    // Visible ranges, set from C++: X in seconds of the day, Y in data units.
    property real xMin: 0
    property real xMax: 1
    property real yMin: -1
    property real yMax: 1

    color: backgroundColor

    // GraphsView.plotArea is new in Qt 6.9; on 6.8 the plot fills the view minus the margins (the axes are hidden).
    readonly property bool hasPlotArea: graphs.plotArea !== undefined
    readonly property rect plot: hasPlotArea ? graphs.plotArea
        : Qt.rect(graphs.marginLeft, graphs.marginTop,
                  Math.max(0, graphs.width - graphs.marginLeft - graphs.marginRight), plotHeight)
    // Separate from plot: the y ticks set the left margin (label width), so they must not depend on it.
    readonly property real plotHeight: Math.max(0, graphs.height - graphs.marginTop - graphs.marginBottom)
    readonly property int tickLength: 5
    readonly property int subTickLength: 2

    // ----- Tick positions ----- //
    function niceStep(raw, mantissas) {
        if (!(raw > 0))
            return 1
        const magnitude = Math.pow(10, Math.floor(Math.log10(raw)))
        for (const m of mantissas)
            if (m * magnitude >= raw * 0.999)
                return m * magnitude
        return 10 * magnitude
    }
    function ticks(min, max, step) {
        const out = []
        if (!(step > 0) || !(max > min))
            return out
        for (let t = Math.ceil(min / step - 1e-9) * step; t <= max + step * 1e-9 && out.length < 200; t += step)
            out.push(Math.abs(t) < step * 1e-9 ? 0 : t)
        return out
    }

    // Time steps that read well: ..., 0.5 s, 1 s, 2 s, 5 s, 10 s, 15 s, 30 s, 1 min, ...
    readonly property real xStep: {
        const raw = (xMax - xMin) / Math.max(2, Math.floor(plot.width / 120))
        const steps = [0.001, 0.002, 0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1, 2, 5, 10, 15, 30,
                       60, 120, 300, 600, 900, 1800, 3600, 7200, 10800, 21600, 43200]
        for (const step of steps)
            if (step >= raw)
                return step
        return 86400
    }
    // QCustomPlot aims for about 10 value ticks; fewer on short charts.
    readonly property real yStep: niceStep((yMax - yMin) / Math.max(2, Math.min(10, Math.floor(plotHeight / 22))), [1, 2, 2.5, 5, 10])
    readonly property var xTicks: ticks(xMin, xMax, xStep)
    readonly property var yTicks: ticks(yMin, yMax, yStep)
    readonly property var xSubTicks: ticks(xMin, xMax, xStep / 5)
    readonly property var ySubTicks: ticks(yMin, yMax, yStep / 5)

    function xPos(value) { return plot.x + (value - xMin) / (xMax - xMin) * plot.width }
    function yPos(value) { return plot.y + plot.height - (value - yMin) / (yMax - yMin) * plot.height }

    // ----- Labels, formatted like QCustomPlot ----- //
    function timeLabel(seconds) { // hh:mm:ss:zzz, '-' prefix for negative times
        const sign = seconds < -0.0005 ? "-" : ""
        const ms = Math.round(Math.abs(seconds) * 1000)
        const pad = (n, w) => String(n).padStart(w, "0")
        return sign + pad(Math.floor(ms / 3600000) % 24, 2) + ":" + pad(Math.floor(ms / 60000) % 60, 2) + ":"
               + pad(Math.floor(ms / 1000) % 60, 2) + ":" + pad(ms % 1000, 3)
    }
    function valueLabel(value) { // locale decimal point, no trailing zeros
        const decimals = Math.max(0, Math.ceil(-Math.log10(yStep) - 1e-9) + (yStep / Math.pow(10, Math.floor(Math.log10(yStep))) === 2.5 ? 1 : 0))
        let text = Number(value).toLocaleString(Qt.locale(), "f", decimals)
        const point = Qt.locale().decimalPoint
        if (text.indexOf(point) >= 0)
            text = text.replace(new RegExp("0+$"), "").replace(new RegExp("\\" + point + "$"), "")
        return text === "-0" ? "0" : text
    }

    TextMetrics { id: yLabelMetrics; text: root.yTicks.map(root.valueLabel).reduce((a, b) => b.length > a.length ? b : a, "0") }
    TextMetrics { id: xLabelMetrics; text: "00:00:00:000" }

    // ----- Grid (behind the data) ----- //
    Repeater {
        model: root.xTicks
        delegate: Rectangle {
            required property real modelData
            x: Math.round(root.xPos(modelData))
            y: root.plot.y
            width: 1
            height: root.plot.height
            color: root.gridColor
        }
    }
    Repeater {
        model: root.yTicks
        delegate: Rectangle {
            required property real modelData
            x: root.plot.x
            y: Math.round(root.yPos(modelData))
            width: root.plot.width
            height: 1
            color: modelData === 0 ? root.axisColor : root.gridColor // zero line stands out
        }
    }

    // ----- Data (Qt Graphs) ----- //
    GraphsView {
        id: graphs
        objectName: "graphsView"
        anchors.fill: parent
        // Antialiased lines like QCustomPlot: render the graph into a multisampled layer.
        layer.enabled: true
        layer.samples: 4
        marginLeft: yLabelMetrics.width + root.tickLength + 8
        marginBottom: xLabelMetrics.height + 6
        marginTop: 8
        marginRight: 8 // narrow like QCustomPlot; edge labels are nudged inside below

        theme: GraphsTheme {
            backgroundVisible: false
            plotAreaBackgroundVisible: false
            labelBackgroundVisible: false
            axisX.mainColor: "transparent" // Qt Graphs' own axis line and ticks are not used
            axisX.subColor: "transparent"
            axisY.mainColor: "transparent"
            axisY.subColor: "transparent"
            grid.mainColor: "transparent"
            grid.subColor: "transparent"
        }
        // Hidden completely so Qt Graphs reserves no room for them; they still map the data.
        axisX: ValueAxis { objectName: "axisX"; visible: false; labelsVisible: false; lineVisible: false; gridVisible: false; subGridVisible: false }
        axisY: ValueAxis { objectName: "axisY"; visible: false; labelsVisible: false; lineVisible: false; gridVisible: false; subGridVisible: false }
    }

    // ----- Marker for the row selected in the Samples table ----- //
    Rectangle {
        visible: !isNaN(root.markerX) && root.markerX >= root.xMin && root.markerX <= root.xMax
        x: visible ? Math.round(root.xPos(root.markerX)) : 0
        y: Math.round(root.plot.y)
        width: 1
        height: Math.round(root.plot.height)
        color: root.markerColor
    }

    // ----- Axes box with inward ticks, like QCustomPlot's full axes box ----- //
    Rectangle {
        x: Math.round(root.plot.x)
        y: Math.round(root.plot.y)
        width: Math.round(root.plot.width) + 1
        height: Math.round(root.plot.height) + 1
        color: "transparent"
        border.color: root.axisColor
        border.width: 1
    }
    Repeater { // bottom and top ticks
        model: root.xTicks.concat(root.xSubTicks.map(t => -t - 1e12))
        delegate: Item {
            required property real modelData
            readonly property bool sub: modelData < -1e11
            readonly property real value: sub ? -(modelData + 1e12) : modelData
            readonly property int length: sub ? root.subTickLength : root.tickLength
            Rectangle { x: Math.round(root.xPos(parent.value)); y: Math.round(root.plot.y + root.plot.height) - parent.length + 1; width: 1; height: parent.length; color: root.axisColor }
            Rectangle { x: Math.round(root.xPos(parent.value)); y: Math.round(root.plot.y); width: 1; height: parent.length; color: root.axisColor }
        }
    }
    Repeater { // left and right ticks
        model: root.yTicks.concat(root.ySubTicks.map(t => -t - 1e12))
        delegate: Item {
            required property real modelData
            readonly property bool sub: modelData < -1e11
            readonly property real value: sub ? -(modelData + 1e12) : modelData
            readonly property int length: sub ? root.subTickLength : root.tickLength
            Rectangle { x: Math.round(root.plot.x); y: Math.round(root.yPos(parent.value)); width: parent.length; height: 1; color: root.axisColor }
            Rectangle { x: Math.round(root.plot.x + root.plot.width) - parent.length + 1; y: Math.round(root.yPos(parent.value)); width: parent.length; height: 1; color: root.axisColor }
        }
    }

    // ----- Tick labels ----- //
    Repeater {
        model: root.xTicks
        delegate: Text {
            required property real modelData
            // Centred under the tick, but kept inside the chart at both edges.
            x: Math.round(Math.max(2, Math.min(root.width - width - 2, root.xPos(modelData) - width / 2)))
            y: Math.round(root.plot.y + root.plot.height) + 4
            text: root.timeLabel(modelData)
            color: root.mutedTextColor
        }
    }
    Repeater {
        model: root.yTicks
        delegate: Text {
            required property real modelData
            x: Math.round(root.plot.x) - width - 6
            y: Math.round(root.yPos(modelData) - height / 2)
            text: root.valueLabel(modelData)
            color: root.mutedTextColor
        }
    }

    // ----- Legend: bottom left inside the plot, like QCustomPlot's ----- //
    Rectangle {
        x: Math.round(root.plot.x) + 8
        y: Math.round(root.plot.y + root.plot.height) - height - 8
        visible: root.legendVisible && legendRepeater.count > 0
        color: root.legendBackground
        border.color: root.legendBorder
        width: legendColumn.implicitWidth + 10
        height: legendColumn.implicitHeight + 6

        Column {
            id: legendColumn
            x: 5
            y: 3
            spacing: 1

            Repeater {
                id: legendRepeater
                model: root.legendModel
                delegate: Row {
                    required property var modelData
                    spacing: 5
                    height: legendText.implicitHeight
                    Rectangle {
                        width: 15
                        height: 2
                        anchors.verticalCenter: parent.verticalCenter
                        color: modelData.color
                        opacity: modelData.visible ? 1.0 : 0.35
                    }
                    Text {
                        id: legendText
                        text: modelData.name
                        color: modelData.visible ? root.textColor : root.mutedTextColor
                        font.pointSize: Qt.application.font.pointSize * 0.9
                    }
                }
            }
        }
    }
}
