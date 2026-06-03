import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import SpectraEQ 1.0

Window {
    id: root

    title: qsTr("SpectraEQ")
    width: 860
    height: 460
    minimumWidth: 480
    minimumHeight: 300
    color: "#0a0a0a"
    visible: true

    readonly property int   barCount: 80
    readonly property int   barSpacing: 3
    readonly property real  cornerRadius: 3
    readonly property real  peakHoldHeight: 3
    readonly property real  glowOpacity: 0.25

    function rainbowColor(index, total, lightness) {
        var hue = (index / total) * 300
        return Qt.hsla(hue / 360.0, 0.90, lightness, 1.0)
    }
    function rainbowColorTop(index, total) { return rainbowColor(index, total, 0.80) }
    function rainbowColorBot(index, total) { return rainbowColor(index, total, 0.45) }
    function rainbowGlow(index, total)     { return rainbowColor(index, total, 0.65) }

    // ─── Device Bar ──────────────────────────────────────────────────────────
    Item {
        id: deviceBar
        anchors {
            top:    parent.top
            left:   parent.left
            right:  parent.right
        }
        height: 40

        // Hairline separator at the bottom
        Rectangle {
            anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
            height: 1
            color: "#ffffff"
            opacity: 0.06
        }

        RowLayout {
            anchors {
                verticalCenter: parent.verticalCenter
                left:  parent.left
                right: parent.right
                leftMargin:  16
                rightMargin: 16
            }
            spacing: 8

            // Label
            Text {
                text: "OUTPUT"
                font.family: "Inter"
                font.pixelSize: 9
                font.letterSpacing: 2
                color: "#ffffff"
                opacity: 0.30
                Layout.alignment: Qt.AlignVCenter
            }

            // ── Dropdown ────────────────────────────────────────────────────
            ComboBox {
                id: deviceCombo
                Layout.fillWidth: true
                Layout.preferredHeight: 26
                Layout.alignment: Qt.AlignVCenter

                model: AppState.availableDevices

                // Push selection back to backend
                onCurrentIndexChanged: {
                    if (AppState.availableDevices &&
                        currentIndex >= 0 &&
                        currentIndex < AppState.availableDevices.length) {
                        AppState.selectedDeviceIndex = currentIndex
                    }
                }

                // Keep combo in sync if the backend changes it externally
                Connections {
                    target: AppState
                    function onSelectedDeviceIndexChanged() {
                        if (deviceCombo.currentIndex !== AppState.selectedDeviceIndex)
                            deviceCombo.currentIndex = AppState.selectedDeviceIndex
                    }
                }

                // ── Visuals ─────────────────────────────────────────────────
                contentItem: Text {
                    leftPadding: 10
                    text: deviceCombo.displayText
                    font.family: "Inter"
                    font.pixelSize: 11
                    color: "#e0e0e0"
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 4
                    color: deviceCombo.pressed ? "#1e1e1e" : deviceCombo.hovered ? "#181818" : "#111111"
                    border.color: deviceCombo.pressed ? "#444444" : "#2a2a2a"
                    border.width: 1
                }

                indicator: Canvas {
                    id: chevron
                    x: deviceCombo.width - width - 8
                    y: (deviceCombo.height - height) / 2
                    width: 10
                    height: 6
                    contextType: "2d"
                    onPaint: {
                        context.reset()
                        context.strokeStyle = "#888888"
                        context.lineWidth   = 1.5
                        context.lineCap     = "round"
                        context.lineJoin    = "round"
                        context.beginPath()
                        context.moveTo(0, 0)
                        context.lineTo(width / 2, height)
                        context.lineTo(width, 0)
                        context.stroke()
                    }
                    // Re-draw if theme changes
                    Connections {
                        target: deviceCombo
                        function onHoveredChanged() { chevron.requestPaint() }
                    }
                }
            }
            // ── End Dropdown ─────────────────────────────────────────────────

            // ── Refresh Button ───────────────────────────────────────────────
            Rectangle {
                id: refreshBtn
                width: 26
                height: 26
                radius: 4
                color: refreshMouse.pressed ? "#1e1e1e" : refreshMouse.containsMouse ? "#181818" : "#111111"
                border.color: refreshMouse.pressed ? "#444444" : "#2a2a2a"
                border.width: 1
                Layout.alignment: Qt.AlignVCenter

                // Rotation animation triggered on click
                property real angle: 0
                RotationAnimation on angle {
                    id: spinAnim
                    from: 0
                    to: 360
                    duration: 600
                    easing.type: Easing.OutCubic
                    running: false
                }

                // Refresh icon drawn with Canvas
                Canvas {
                    id: refreshIcon
                    anchors.centerIn: parent
                    width: 14
                    height: 14
                    rotation: refreshBtn.angle
                    contextType: "2d"

                    onPaint: {
                        context.reset()
                        context.strokeStyle = refreshMouse.containsMouse ? "#cccccc" : "#888888"
                        context.lineWidth   = 1.5
                        context.lineCap     = "round"

                        var cx = width  / 2
                        var cy = height / 2
                        var r  = 5.5

                        // Arc: 300° of a circle (leaving a gap for the arrowhead)
                        context.beginPath()
                        context.arc(cx, cy, r,
                            -Math.PI * 0.15,   // start  (~-27°)
                            Math.PI * 1.72,   // end    (~310°)
                            false)
                        context.stroke()

                        // Arrowhead at the end of the arc
                        var endAngle = Math.PI * 1.72
                        var tx = cx + r * Math.cos(endAngle)
                        var ty = cy + r * Math.sin(endAngle)
                        // tangent direction perpendicular to radius
                        var ax = -Math.sin(endAngle)
                        var ay =  Math.cos(endAngle)
                        var as_ = 4.0
                        context.beginPath()
                        context.moveTo(tx - ax * as_ * 0.5 - ay * as_ * 0.4,
                            ty - ay * as_ * 0.5 + ax * as_ * 0.4)
                        context.lineTo(tx, ty)
                        context.lineTo(tx - ax * as_ * 0.5 + ay * as_ * 0.4,
                            ty - ay * as_ * 0.5 - ax * as_ * 0.4)
                        context.stroke()
                    }

                    Connections {
                        target: refreshMouse
                        function onContainsMouseChanged() { refreshIcon.requestPaint() }
                    }
                }

                MouseArea {
                    id: refreshMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor

                    onClicked: {
                        spinAnim.restart()
                        AppState.refreshDevices()
                    }
                }

                // Tooltip
                ToolTip {
                    visible: refreshMouse.containsMouse
                    text: "Refresh devices"
                    delay: 500
                }
            }
            // ── End Refresh Button ────────────────────────────────────────────
        }
    }
    // ─── End Device Bar ───────────────────────────────────────────────────────

    // ─── Plot Area ────────────────────────────────────────────────────────────
    Item {
        id: plotArea
        anchors {
            top:          deviceBar.bottom
            bottom:       freqBar.top
            left:         parent.left
            right:        parent.right
            topMargin:    12
            bottomMargin: 0
            leftMargin:   16
            rightMargin:  16
        }

        // Subtle horizontal grid
        Repeater {
            model: 4
            Rectangle {
                y:      plotArea.height * (1.0 - (index + 1) / 4.0)
                width:  plotArea.width
                height: 1
                color:  "#ffffff"
                opacity: 0.04
            }
        }

        // Bars
        Repeater {
            id: barRepeater
            model: root.barCount

            Item {
                id: barRoot

                property real colW: (plotArea.width - (root.barCount - 1) * root.barSpacing) / root.barCount
                x: index * (colW + root.barSpacing)
                width: colW
                height: plotArea.height

                property real rawValue: {
                    var b = AppState.frequencyBands
                    return (b && index < b.length) ? b[index] : 0.0
                }

                property real dispValue: 0.0
                Behavior on dispValue {
                    SmoothedAnimation {
                        velocity: -1
                        duration: dispValue > barRoot.rawValue ? 220 : 55
                    }
                }
                onRawValueChanged: dispValue = rawValue

                property real peakValue: 0.0
                property real peakTimer: 0.0

                Timer {
                    interval: 16; running: true; repeat: true
                    onTriggered: {
                        if (barRoot.dispValue >= barRoot.peakValue) {
                            barRoot.peakValue = barRoot.dispValue
                            barRoot.peakTimer = 0
                        } else {
                            barRoot.peakTimer += 0.016
                            if (barRoot.peakTimer > 0.6)
                                barRoot.peakValue = Math.max(0, barRoot.peakValue - 0.008)
                        }
                    }
                }

                readonly property color colTop:  root.rainbowColorTop(index, root.barCount)
                readonly property color colBot:  root.rainbowColorBot(index, root.barCount)
                readonly property color colGlow: root.rainbowGlow(index, root.barCount)

                // Bloom
                Rectangle {
                    anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }
                    width:   barRoot.colW + 8
                    height:  Math.max(1, barRoot.dispValue * plotArea.height) + 6
                    radius:  root.cornerRadius + 2
                    color:   barRoot.colGlow
                    opacity: root.glowOpacity * barRoot.dispValue
                    layer.enabled: true
                    layer.effect: null
                    Rectangle {
                        anchors.centerIn: parent
                        width:   parent.width  + 6
                        height:  parent.height + 6
                        radius:  parent.radius + 2
                        color:   barRoot.colGlow
                        opacity: 0.4
                    }
                }

                // Main bar
                Rectangle {
                    id: mainBar
                    anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }
                    width:  barRoot.colW
                    height: Math.max(2, barRoot.dispValue * plotArea.height)
                    radius: root.cornerRadius
                    gradient: Gradient {
                        orientation: Gradient.Vertical
                        GradientStop { position: 0.0; color: barRoot.colTop }
                        GradientStop { position: 1.0; color: barRoot.colBot }
                    }
                }

                // Peak-hold tick
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    y:      plotArea.height - barRoot.peakValue * plotArea.height - root.peakHoldHeight - 1
                    width:  barRoot.colW
                    height: root.peakHoldHeight
                    radius: 1
                    color:  barRoot.colTop
                    opacity: barRoot.peakValue > 0.02 ? 0.9 : 0.0
                    Behavior on opacity { NumberAnimation { duration: 200 } }
                }
            }
        }

        // Floor reflection
        Item {
            anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
            height: 28
            clip: true
            opacity: 0.18

            Repeater {
                model: root.barCount
                Rectangle {
                    property real colW: (plotArea.width - (root.barCount - 1) * root.barSpacing) / root.barCount
                    x: index * (colW + root.barSpacing)
                    width: colW
                    height: {
                        var b = AppState.frequencyBands
                        return (b && index < b.length) ? b[index] * 28 : 0
                    }
                    anchors.top: parent.top
                    radius: 1
                    color: root.rainbowColorBot(index, root.barCount)
                    transform: Scale { yScale: -1; origin.y: parent.height / 2 }
                }
            }

            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#00000000" }
                    GradientStop { position: 1.0; color: "#0a0a0a" }
                }
            }
        }
    }

    // ─── Frequency Label Bar ─────────────────────────────────────────────────
    Item {
        id: freqBar
        anchors {
            bottom: parent.bottom
            left:   parent.left
            right:  parent.right
        }
        height: 22

        readonly property var  labelHz: [40, 100, 250, 500, 1000, 2000, 4000, 8000, 16000]
        readonly property real logMin:  Math.log(40)
        readonly property real logMax:  Math.log(16000)

        Repeater {
            model: freqBar.labelHz
            Text {
                property real frac: (Math.log(modelData) - freqBar.logMin) / (freqBar.logMax - freqBar.logMin)
                x: 16 + frac * (freqBar.width - 32) - width / 2
                y: 4
                text: modelData >= 1000 ? (modelData / 1000) + "k" : "" + modelData
                font.family: "Inter"
                font.pixelSize: 9
                font.letterSpacing: 1
                color: root.rainbowColor(frac * root.barCount, root.barCount, 0.55)
            }
        }
    }
}
