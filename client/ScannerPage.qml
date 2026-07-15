import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import QtCore

Page {
    id: rootPage

    required property var qrScanner
    required property var clientController

    background: Rectangle { color: "#0a0a0f" }

    // --- Camera Setup ---
    CaptureSession {
        id: captureSession
        camera: Camera {
            id: camera
            active: false  // Start inactive; activated after permission granted
        }
        videoOutput: videoOutput
    }

    CameraPermission {
        id: cameraPermission
        onStatusChanged: {
            if (status === Qt.PermissionStatus.Granted) {
                camera.active = true
            }
        }
    }

    // --- Android Camera Permission Handling ---
    Component.onCompleted: {
        if (Qt.platform.os === "android") {
            if (cameraPermission.status === Qt.PermissionStatus.Granted) {
                camera.active = true
            } else {
                cameraPermission.request()
            }
        } else {
            camera.active = true
        }
    }

    // Bind QR scanner to the video sink
    Binding {
        target: rootPage.qrScanner || null
        property: "videoSink"
        value: videoOutput.videoSink
    }

    // Handle detected code
    Connections {
        target: rootPage.qrScanner || null
        ignoreUnknownSignals: true
        function onCodeDetected(code) {
            if (!rootPage.qrScanner.scanning) return
            rootPage.qrScanner.scanning = false
            statusText.text = "Connecting to " + code + "..."
            statusText.color = "#00e5ff"
            rootPage.clientController.connectToServer(code)
        }
    }

    // --- Full screen viewfinder ---
    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop
    }

    // --- Dark overlay with cutout ---
    Item {
        anchors.fill: parent

        // Scan region size
        readonly property real scanSize: Math.min(parent.width, parent.height) * 0.65

        // Top overlay
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: (parent.height - parent.scanSize) / 2
            color: "#cc0a0a0f"
        }
        // Bottom overlay
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: (parent.height - parent.scanSize) / 2
            color: "#cc0a0a0f"
        }
        // Left overlay
        Rectangle {
            anchors.left: parent.left
            y: (parent.height - parent.scanSize) / 2
            width: (parent.width - parent.scanSize) / 2
            height: parent.scanSize
            color: "#cc0a0a0f"
        }
        // Right overlay
        Rectangle {
            anchors.right: parent.right
            y: (parent.height - parent.scanSize) / 2
            width: (parent.width - parent.scanSize) / 2
            height: parent.scanSize
            color: "#cc0a0a0f"
        }

        // Scan region frame - corner brackets
        Item {
            id: scanRegion
            width: parent.scanSize
            height: parent.scanSize
            anchors.centerIn: parent

            readonly property real cornerLen: 30
            readonly property real cornerThk: 3
            readonly property color cornerColor: "#00e5ff"

            // Top-left corner
            Rectangle { x: 0; y: 0; width: parent.cornerLen; height: parent.cornerThk; color: parent.cornerColor }
            Rectangle { x: 0; y: 0; width: parent.cornerThk; height: parent.cornerLen; color: parent.cornerColor }
            // Top-right corner
            Rectangle { x: parent.width - parent.cornerLen; y: 0; width: parent.cornerLen; height: parent.cornerThk; color: parent.cornerColor }
            Rectangle { x: parent.width - parent.cornerThk; y: 0; width: parent.cornerThk; height: parent.cornerLen; color: parent.cornerColor }
            // Bottom-left corner
            Rectangle { x: 0; y: parent.height - parent.cornerThk; width: parent.cornerLen; height: parent.cornerThk; color: parent.cornerColor }
            Rectangle { x: 0; y: parent.height - parent.cornerLen; width: parent.cornerThk; height: parent.cornerLen; color: parent.cornerColor }
            // Bottom-right corner
            Rectangle { x: parent.width - parent.cornerLen; y: parent.height - parent.cornerThk; width: parent.cornerLen; height: parent.cornerThk; color: parent.cornerColor }
            Rectangle { x: parent.width - parent.cornerThk; y: parent.height - parent.cornerLen; width: parent.cornerThk; height: parent.cornerLen; color: parent.cornerColor }

            // Animated scan line
            Rectangle {
                id: scanLine
                width: parent.width - 20
                height: 2
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#00e5ff"
                opacity: 0.8

                // Glow effect
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: -4
                    color: "transparent"
                    border.color: "#4000e5ff"
                    border.width: 4
                    radius: 2
                }

                SequentialAnimation on y {
                    loops: Animation.Infinite
                    NumberAnimation {
                        from: 10
                        to: scanRegion.height - 12
                        duration: 2500
                        easing.type: Easing.InOutSine
                    }
                    NumberAnimation {
                        from: scanRegion.height - 12
                        to: 10
                        duration: 2500
                        easing.type: Easing.InOutSine
                    }
                }
            }
        }
    }

    // --- Status text ---
    Text {
        id: statusText
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 80
        anchors.horizontalCenter: parent.horizontalCenter
        text: "Point camera at QR code on PC"
        color: "#aaaaaa"
        font.pixelSize: 16
        font.family: "sans-serif"
        font.weight: Font.Medium

        // Subtle pulse animation
        SequentialAnimation on opacity {
            loops: Animation.Infinite
            NumberAnimation { to: 0.5; duration: 1500; easing.type: Easing.InOutQuad }
            NumberAnimation { to: 1.0; duration: 1500; easing.type: Easing.InOutQuad }
        }
    }

    // --- App title ---
    Text {
        id: appTitle
        anchors.top: parent.top
        anchors.topMargin: 50
        anchors.horizontalCenter: parent.horizontalCenter
        text: "QR CONTROLLER"
        color: "#00e5ff"
        font.pixelSize: 22
        font.weight: Font.Bold
        font.letterSpacing: 4
    }

    Row {
        id: buttonRow
        anchors.top: appTitle.bottom
        anchors.topMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 20

        Button {
            id: autoConnectButton
            text: "Auto-Connect (Wi-Fi)"
            background: Rectangle {
                implicitWidth: 180
                implicitHeight: 40
                color: autoConnectButton.pressed ? "#0097a7" : "#00e5ff"
                radius: 4
            }
            contentItem: Text {
                text: autoConnectButton.text
                color: "#0a0a0f"
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: {
                statusText.text = "Connecting to 10.239.167.160:7654..."
                statusText.color = "#00e5ff"
                rootPage.clientController.connectToServer("10.239.167.160:7654")
            }
        }

        Button {
            id: adbConnectButton
            text: "Connect (ADB / USB)"
            background: Rectangle {
                implicitWidth: 180
                implicitHeight: 40
                color: adbConnectButton.pressed ? "#b388ff" : "#e040fb"
                radius: 4
            }
            contentItem: Text {
                text: adbConnectButton.text
                color: "#ffffff"
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: {
                statusText.text = "Connecting to ADB (127.0.0.1:7654)..."
                statusText.color = "#e040fb"
                rootPage.clientController.connectToServer("127.0.0.1:7654")
            }
        }
    }
}
