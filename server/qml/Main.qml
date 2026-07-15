import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QRControllerServer

ApplicationWindow {
    id: root
    width: 620
    height: 520
    visible: true
    title: "QR Controller Server"
    color: "#0a0a1a"

    // Gradient background
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#0f0c29" }
            GradientStop { position: 0.5; color: "#1a1a3e" }
            GradientStop { position: 1.0; color: "#0a0a1a" }
        }
    }

    // Subtle animated background particles
    Repeater {
        model: 8
        Rectangle {
            property real startX: Math.random() * root.width
            property real startY: Math.random() * root.height
            x: startX
            y: startY
            width: 3 + Math.random() * 4
            height: width
            radius: width / 2
            color: Qt.rgba(0.4, 0.5, 1.0, 0.08 + Math.random() * 0.12)

            SequentialAnimation on opacity {
                loops: Animation.Infinite
                NumberAnimation { from: 0.05; to: 0.25; duration: 2000 + Math.random() * 3000; easing.type: Easing.InOutSine }
                NumberAnimation { from: 0.25; to: 0.05; duration: 2000 + Math.random() * 3000; easing.type: Easing.InOutSine }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 30
        spacing: 0

        // ── Header ──
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "QR Controller"
            font.pixelSize: 28
            font.weight: Font.Bold
            font.letterSpacing: 2
            color: "#e0e6ff"
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 4
            text: "VIRTUAL GAMEPAD SERVER"
            font.pixelSize: 11
            font.weight: Font.Medium
            font.letterSpacing: 4
            color: "#6670a0"
        }

        Item { Layout.fillHeight: true; Layout.maximumHeight: 24 }

        // ── QR Code View (when disconnected) ──
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !ServerController.connected
            opacity: visible ? 1.0 : 0.0

            Behavior on opacity { NumberAnimation { duration: 500; easing.type: Easing.InOutQuad } }

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 20

                // QR Code with decorative frame
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 264
                    height: 264
                    radius: 16
                    color: "#ffffff"

                    // Subtle outer glow
                    Rectangle {
                        anchors.centerIn: parent
                        width: parent.width + 20
                        height: parent.height + 20
                        radius: 22
                        color: "transparent"
                        border.color: Qt.rgba(0.45, 0.55, 1.0, 0.2)
                        border.width: 1

                        SequentialAnimation on border.color {
                            loops: Animation.Infinite
                            ColorAnimation { to: Qt.rgba(0.55, 0.4, 1.0, 0.35); duration: 2500; easing.type: Easing.InOutSine }
                            ColorAnimation { to: Qt.rgba(0.45, 0.55, 1.0, 0.2);  duration: 2500; easing.type: Easing.InOutSine }
                        }
                    }

                    Image {
                        anchors.centerIn: parent
                        width: 240
                        height: 240
                        sourceSize: Qt.size(240, 240)
                        source: ServerController.qrData ? ("image://qrcode/" + ServerController.qrData) : ""
                        fillMode: Image.PreserveAspectFit
                        smooth: false
                    }
                }

                // Scan instruction
                ColumnLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 6

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Scan with QR Controller app"
                        font.pixelSize: 15
                        font.weight: Font.Medium
                        color: "#b0b8d8"
                    }

                    // Address pill
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: addressText.implicitWidth + 28
                        height: 30
                        radius: 15
                        color: Qt.rgba(1, 1, 1, 0.06)
                        border.color: Qt.rgba(1, 1, 1, 0.1)
                        border.width: 1

                        Text {
                            id: addressText
                            anchors.centerIn: parent
                            text: ServerController.serverAddress || "—"
                            font.pixelSize: 13
                            font.family: "monospace"
                            font.weight: Font.Medium
                            color: "#8090cc"
                        }
                    }
                }
            }
        }

        // ── Connected View ──
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: ServerController.connected
            opacity: visible ? 1.0 : 0.0

            Behavior on opacity { NumberAnimation { duration: 500; easing.type: Easing.InOutQuad } }

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 24

                // Pulsing gamepad icon
                Item {
                    Layout.alignment: Qt.AlignHCenter
                    width: 120
                    height: 120

                    // Pulse ring
                    Rectangle {
                        id: pulseRing
                        anchors.centerIn: parent
                        width: 110
                        height: 110
                        radius: 55
                        color: "transparent"
                        border.color: Qt.rgba(0.3, 0.9, 0.5, 0.3)
                        border.width: 2

                        SequentialAnimation on scale {
                            loops: Animation.Infinite
                            NumberAnimation { from: 0.8; to: 1.3; duration: 1500; easing.type: Easing.InOutQuad }
                            NumberAnimation { from: 1.3; to: 0.8; duration: 1500; easing.type: Easing.InOutQuad }
                        }

                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            NumberAnimation { from: 0.6; to: 0.1; duration: 1500; easing.type: Easing.InOutQuad }
                            NumberAnimation { from: 0.1; to: 0.6; duration: 1500; easing.type: Easing.InOutQuad }
                        }
                    }

                    // Gamepad circle
                    Rectangle {
                        anchors.centerIn: parent
                        width: 90
                        height: 90
                        radius: 45
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#2dce6b" }
                            GradientStop { position: 1.0; color: "#1a9f50" }
                        }

                        Text {
                            anchors.centerIn: parent
                            text: "🎮"
                            font.pixelSize: 40
                        }
                    }
                }

                // Connection info
                ColumnLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 8

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: "CONNECTED"
                        font.pixelSize: 18
                        font.weight: Font.Bold
                        font.letterSpacing: 3
                        color: "#4ade80"
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: ServerController.clientName || ""
                        font.pixelSize: 14
                        font.family: "monospace"
                        color: "#7890b8"
                    }

                    // Active indicator dot row
                    Row {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: 8
                        spacing: 6

                        Repeater {
                            model: 3
                            Rectangle {
                                width: 6
                                height: 6
                                radius: 3
                                color: "#4ade80"

                                SequentialAnimation on opacity {
                                    loops: Animation.Infinite
                                    PauseAnimation { duration: index * 300 }
                                    NumberAnimation { from: 0.3; to: 1.0; duration: 600; easing.type: Easing.InOutSine }
                                    NumberAnimation { from: 1.0; to: 0.3; duration: 600; easing.type: Easing.InOutSine }
                                }
                            }
                        }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true; Layout.maximumHeight: 16 }

        // ── Status Bar ──
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            radius: 10
            color: Qt.rgba(1, 1, 1, 0.04)
            border.color: Qt.rgba(1, 1, 1, 0.06)
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16

                // Status dot
                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: ServerController.connected ? "#4ade80" : "#f59e0b"

                    SequentialAnimation on opacity {
                        loops: Animation.Infinite
                        NumberAnimation { from: 1.0; to: 0.4; duration: 1000; easing.type: Easing.InOutSine }
                        NumberAnimation { from: 0.4; to: 1.0; duration: 1000; easing.type: Easing.InOutSine }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: ServerController.status || ""
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    color: "#8898b8"
                    elide: Text.ElideRight
                }
            }
        }
    }
}
