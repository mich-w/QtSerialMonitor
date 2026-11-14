// Chart drawn with Qt Graphs. Data, ranges and colours are pushed in from C++ (GraphsChartView).
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
    property var legendModel: [] // [{ name, color, visible }], set from C++

    // Visible time range in seconds of the day, set from C++.
    property real xMin: 0
    property real xMax: 1

    color: backgroundColor

    // ----- Time axis: drawn here because Qt Graphs places anchored tick labels incorrectly ----- //
    readonly property rect plot: graphs.plotArea
    readonly property real xStep: {
        const span = xMax - xMin
        const wanted = Math.max(2, Math.floor(plot.width / 110)) // about one label per 110 px
        const raw = span / wanted
        const steps = [0.001, 0.002, 0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1, 2, 5, 10, 15, 30,
                       60, 120, 300, 600, 900, 1800, 3600, 7200, 10800, 21600, 43200]
        for (const step of steps)
            if (step >= raw)
                return step
        return 86400
    }
    readonly property var xTicks: {
        const ticks = []
        if (!(xStep > 0) || !(xMax > xMin))
            return ticks
        for (let t = Math.ceil(xMin / xStep) * xStep; t <= xMax + 1e-9 && ticks.length < 100; t += xStep)
            ticks.push(t)
        return ticks
    }
    function timeLabel(seconds) {
        const decimals = xStep >= 1 ? 0 : Math.ceil(-Math.log10(xStep) - 1e-9)
        const ms = Math.round(seconds * 1000)
        const pad = (n, w) => String(n).padStart(w, "0")
        const base = pad(Math.floor(ms / 3600000) % 24, 2) + ":" + pad(Math.floor(ms / 60000) % 60, 2) + ":"
                     + pad(Math.floor(ms / 1000) % 60, 2)
        return decimals > 0 ? base + "." + pad(((ms % 1000) + 1000) % 1000, 3).substring(0, decimals) : base
    }

    Repeater {
        model: root.xTicks
        delegate: Item {
            required property real modelData
            x: root.plot.x + (modelData - root.xMin) / (root.xMax - root.xMin) * root.plot.width
            Rectangle { // vertical grid line, behind the series
                x: 0
                y: root.plot.y
                width: 1
                height: root.plot.height
                color: root.gridColor
            }
            Rectangle { // tick mark
                x: 0
                y: root.plot.y + root.plot.height
                width: 1
                height: 5
                color: root.axisColor
            }
            Text {
                x: -width / 2
                y: root.plot.y + root.plot.height + 7
                text: root.timeLabel(modelData)
                color: root.mutedTextColor
                font.pixelSize: 11
            }
        }
    }

    Rectangle { // time axis line (Qt Graphs' own is hidden, its ticks are misplaced)
        x: root.plot.x
        y: root.plot.y + root.plot.height
        width: root.plot.width
        height: 1
        color: root.axisColor
    }

    GraphsView {
        id: graphs
        objectName: "graphsView"
        anchors.fill: parent
        marginLeft: 6
        marginRight: 14
        marginTop: 10
        marginBottom: 26 // room for the time labels drawn above

        theme: GraphsTheme {
            backgroundVisible: false        // root draws the background
            plotAreaBackgroundVisible: false // so the time grid shows through
            labelTextColor: root.mutedTextColor
            labelBackgroundVisible: false
            grid.mainColor: root.gridColor
            grid.subColor: root.gridColor
            axisX.mainColor: root.axisColor
            axisY.mainColor: root.axisColor
            axisY.labelTextColor: root.mutedTextColor
        }

        axisX: ValueAxis {
            objectName: "axisX"
            labelsVisible: false
            lineVisible: false
            gridVisible: false
            subGridVisible: false
        }
        axisY: ValueAxis {
            objectName: "axisY"
        }
    }

    // Qt Graphs has no built-in legend for 2D graphs.
    Rectangle {
        x: root.plot.x + 8
        y: root.plot.y + root.plot.height - height - 8
        visible: root.legendVisible && legendRepeater.count > 0
        color: root.legendBackground
        border.color: root.legendBorder
        radius: 4
        width: legendColumn.implicitWidth + 16
        height: legendColumn.implicitHeight + 10

        Column {
            id: legendColumn
            x: 8
            y: 5
            spacing: 2

            Repeater {
                id: legendRepeater
                model: root.legendModel
                delegate: Row {
                    required property var modelData
                    spacing: 6
                    Rectangle {
                        width: 16
                        height: 3
                        anchors.verticalCenter: parent.verticalCenter
                        color: modelData.color
                        opacity: modelData.visible ? 1.0 : 0.35
                    }
                    Text {
                        text: modelData.name
                        color: modelData.visible ? root.textColor : root.mutedTextColor
                        font.pixelSize: 11
                    }
                }
            }
        }
    }
}
