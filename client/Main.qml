import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QRController

ApplicationWindow {
    id: appWindow
    visible: true
    width: 360
    height: 640
    title: "QR Controller"
    color: "#0a0a0f"

    // --- Shared C++ backend objects ---
    QRScanner   { id: cppScanner }
    ClientController { id: cppClientController }
    MotionReader { id: cppMotionReader }

    // --- Connection status handling ---
    Connections {
        target: cppClientController
        function onServerConnected() {
            stackView.push(controllerPage)
        }
        function onConnectionFailed(reason) {
            console.warn("Connection failed:", reason)
        }
    }

    // --- Page navigation ---
    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: scannerPage

        pushEnter: Transition {
            PropertyAnimation { property: "opacity"; from: 0; to: 1; duration: 300; easing.type: Easing.InOutQuad }
        }
        pushExit: Transition {
            PropertyAnimation { property: "opacity"; from: 1; to: 0; duration: 300; easing.type: Easing.InOutQuad }
        }
        popEnter: Transition {
            PropertyAnimation { property: "opacity"; from: 0; to: 1; duration: 300; easing.type: Easing.InOutQuad }
        }
        popExit: Transition {
            PropertyAnimation { property: "opacity"; from: 1; to: 0; duration: 300; easing.type: Easing.InOutQuad }
        }
    }

    Component {
        id: scannerPage
        ScannerPage {
            qrScanner: cppScanner
            clientController: cppClientController
        }
    }

    Component {
        id: controllerPage
        ControllerPage {
            clientController: cppClientController
            motionReader: cppMotionReader
        }
    }
}
