// Chart drawn with Qt Graphs. Data, ranges and colours are pushed in from C++ (GraphsChartView).
import QtQuick
import QtGraphs

Item {
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

    GraphsView {
        id: graphs
        objectName: "graphsView"
        anchors.fill: parent
        marginLeft: 6
        marginRight: 14
        marginTop: 10
        marginBottom: 6

        theme: GraphsTheme {
            backgroundColor: root.backgroundColor
            plotAreaBackgroundColor: root.backgroundColor
            labelTextColor: root.mutedTextColor
            labelBackgroundVisible: false
            grid.mainColor: root.gridColor
            grid.subColor: root.gridColor
            axisX.mainColor: root.axisColor
            axisX.labelTextColor: root.mutedTextColor
            axisY.mainColor: root.axisColor
            axisY.labelTextColor: root.mutedTextColor
        }

        axisX: DateTimeAxis {
            objectName: "axisX"
            labelFormat: "hh:mm:ss.z"
        }
        axisY: ValueAxis {
            objectName: "axisY"
        }
    }

    // Qt Graphs has no built-in legend for 2D graphs.
    Rectangle {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 60
        anchors.bottomMargin: 40
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
