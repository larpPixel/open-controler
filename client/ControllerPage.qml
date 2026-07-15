import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: controllerPage

    required property var clientController
    required property var motionReader

    background: Rectangle {
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#0d0d14" }
            GradientStop { position: 0.5; color: "#0a0a10" }
            GradientStop { position: 1.0; color: "#0d0d14" }
        }
    }

    // Force landscape content
    readonly property bool isLandscape: width > height

    // ---- Color Palette ----
    readonly property color neonCyan:    "#00e5ff"
    readonly property color neonGreen:   "#00e676"
    readonly property color neonRed:     "#ff1744"
    readonly property color neonBlue:    "#2979ff"
    readonly property color neonYellow:  "#ffea00"
    readonly property color neonPurple:  "#d500f9"
    readonly property color dimWhite:    "#b0b0b0"
    readonly property color darkSurface: "#1a1a24"
    readonly property color midSurface:  "#22222e"

    // ---- Shared State ----
    property real leftStickX: 0
    property real leftStickY: 0
    property real rightStickX: 0
    property real rightStickY: 0
    property real leftTrigger: 0
    property real rightTrigger: 0
    property int  buttonMask: 0

    onLeftStickXChanged: clientController.setLeftStick(leftStickX, leftStickY)
    onLeftStickYChanged: clientController.setLeftStick(leftStickX, leftStickY)
    onRightStickXChanged: clientController.setRightStick(rightStickX, rightStickY)
    onRightStickYChanged: clientController.setRightStick(rightStickX, rightStickY)
    onLeftTriggerChanged: clientController.setTriggers(leftTrigger, rightTrigger)
    onRightTriggerChanged: clientController.setTriggers(leftTrigger, rightTrigger)

    Connections {
        target: controllerPage.motionReader
        function onAccelChanged() {
            clientController.setAccel(controllerPage.motionReader.accelX, controllerPage.motionReader.accelY, controllerPage.motionReader.accelZ)
        }
        function onGyroChanged() {
            clientController.setGyro(controllerPage.motionReader.gyroX, controllerPage.motionReader.gyroY, controllerPage.motionReader.gyroZ)
        }
        function onModeChanged() {
            clientController.setMotionEnabled(controllerPage.motionReader.mode)
        }
    }

    // =============================================
    //  TOP BAR: Shoulder buttons + center controls
    // =============================================
    Item {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 70

        // --- LB ---
        GamepadButton {
            id: lbBtn
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            width: 90; height: 44
            label: "LB"
            btnColor: controllerPage.neonCyan
            bitFlag: 16  // Btn::LB = 1<<4
            onPressedChanged: updateButton(bitFlag, pressed)
        }

        // --- LT Slider ---
        TriggerSlider {
            id: ltSlider
            anchors.left: lbBtn.right
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: 50; height: 50
            label: "LT"
            sliderColor: controllerPage.neonCyan
            onValueChanged: controllerPage.leftTrigger = value
        }

        // --- Center: Select / Connection Info / Start ---
        Row {
            anchors.centerIn: parent
            spacing: 16

            GamepadButton {
                width: 60; height: 30
                label: "SEL"
                btnColor: controllerPage.dimWhite
                fontSize: 10
                bitFlag: 128  // Btn::Select = 1<<7
                onPressedChanged: updateButton(bitFlag, pressed)
            }

            // Connection status
            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                Row {
                    spacing: 4
                    anchors.horizontalCenter: parent.horizontalCenter
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: controllerPage.clientController.connected ? controllerPage.neonGreen : controllerPage.neonRed
                        anchors.verticalCenter: parent.verticalCenter

                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            NumberAnimation { to: 0.3; duration: 800 }
                            NumberAnimation { to: 1.0; duration: 800 }
                        }
                    }
                    Text {
                        text: controllerPage.clientController.connected ? "ONLINE" : "OFFLINE"
                        color: controllerPage.dimWhite
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        font.letterSpacing: 1
                    }
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: controllerPage.clientController.latencyMs + " ms"
                    color: "#666"
                    font.pixelSize: 9
                    visible: controllerPage.clientController.connected
                }
            }

            GamepadButton {
                width: 60; height: 30
                label: "STA"
                btnColor: controllerPage.dimWhite
                fontSize: 10
                bitFlag: 64  // Btn::Start = 1<<6
                onPressedChanged: updateButton(bitFlag, pressed)
            }
        }

        // --- RT Slider ---
        TriggerSlider {
            id: rtSlider
            anchors.right: rbBtn.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: 50; height: 50
            label: "RT"
            sliderColor: controllerPage.neonRed
            onValueChanged: controllerPage.rightTrigger = value
        }

        // --- RB ---
        GamepadButton {
            id: rbBtn
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            width: 90; height: 44
            label: "RB"
            btnColor: controllerPage.neonRed
            bitFlag: 32  // Btn::RB = 1<<5
            onPressedChanged: updateButton(bitFlag, pressed)
        }
    }

    // =============================================
    //  MAIN AREA: Left side (D-pad + stick) | Right side (ABXY + stick)
    // =============================================
    Item {
        id: mainArea
        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: bottomBar.top

        // ===================== LEFT SIDE =====================
        Item {
            id: leftSide
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: parent.width * 0.5

            // --- LEFT ANALOG STICK ---
            AnalogStick {
                id: leftStick
                anchors.left: parent.left
                anchors.leftMargin: 60
                anchors.top: parent.top
                anchors.topMargin: 20
                width: 160; height: 160
                stickColor: controllerPage.neonCyan
                isLeftStick: true
                onPositionChanged: (xVal, yVal) => {
                    controllerPage.leftStickX = xVal
                    controllerPage.leftStickY = yVal
                }
                onClickedChanged: updateButton(4096, leftStick.clicked)  // L3 = 1<<12
            }

            // --- D-PAD ---
            Item {
                id: dpadArea
                anchors.left: leftStick.right
                anchors.leftMargin: 40
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20
                width: 140; height: 140

                // D-pad background circle
                Rectangle {
                    anchors.fill: parent
                    radius: 70
                    color: controllerPage.darkSurface
                    border.color: "#2a2a38"
                    border.width: 1
                }

                // D-pad buttons
                DpadButton {
                    id: upBtn
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: 6
                    width: 44; height: 50
                    direction: "up"
                    rotation: 0
                    bitFlag: 256  // Btn::Up = 1<<8
                    onActiveChanged: updateButton(bitFlag, active)
                }
                DpadButton {
                    id: downBtn
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 6
                    width: 44; height: 50
                    direction: "down"
                    rotation: 180
                    bitFlag: 512  // Btn::Down = 1<<9
                    onActiveChanged: updateButton(bitFlag, active)
                }
                DpadButton {
                    id: leftBtn
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 6
                    width: 50; height: 44
                    direction: "left"
                    rotation: 270
                    bitFlag: 1024  // Btn::Left = 1<<10
                    onActiveChanged: updateButton(bitFlag, active)
                }
                DpadButton {
                    id: rightBtn
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 6
                    width: 50; height: 44
                    direction: "right"
                    rotation: 90
                    bitFlag: 2048  // Btn::Right = 1<<11
                    onActiveChanged: updateButton(bitFlag, active)
                }

                // Center dot
                Rectangle {
                    anchors.centerIn: parent
                    width: 12; height: 12; radius: 6
                    color: "#333340"
                }

                MultiPointTouchArea {
                    anchors.fill: parent
                    minimumTouchPoints: 1
                    maximumTouchPoints: 4

                    touchPoints: [
                        TouchPoint { id: dt1 },
                        TouchPoint { id: dt2 },
                        TouchPoint { id: dt3 },
                        TouchPoint { id: dt4 }
                    ]

                    function updateDpadStates() {
                        let up = false
                        let down = false
                        let left = false
                        let right = false

                        let points = [dt1, dt2, dt3, dt4]
                        for (let i = 0; i < points.length; i++) {
                            let tp = points[i]
                            if (tp.pressed) {
                                let tx = tp.x
                                let ty = tp.y

                                // Up button zone
                                if (tx >= 30 && tx <= 110 && ty >= 0 && ty <= 56) up = true
                                // Down button zone
                                if (tx >= 30 && tx <= 110 && ty >= 84 && ty <= 140) down = true
                                // Left button zone
                                if (tx >= 0 && tx <= 56 && ty >= 30 && ty <= 110) left = true
                                // Right button zone
                                if (tx >= 84 && tx <= 140 && ty >= 30 && ty <= 110) right = true
                            }
                        }

                        upBtn.active = up
                        downBtn.active = down
                        leftBtn.active = left
                        rightBtn.active = right
                    }

                    onPressed: updateDpadStates()
                    onUpdated: updateDpadStates()
                    onReleased: updateDpadStates()
                }
            }
        }

        // ===================== RIGHT SIDE =====================
        Item {
            id: rightSide
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: parent.width * 0.5

            // --- RIGHT ANALOG STICK ---
            AnalogStick {
                id: rightStick
                anchors.right: parent.right
                anchors.rightMargin: 60
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20
                width: 160; height: 160
                stickColor: controllerPage.neonRed
                isLeftStick: false
                onPositionChanged: (xVal, yVal) => {
                    controllerPage.rightStickX = xVal
                    controllerPage.rightStickY = yVal
                }
                onClickedChanged: updateButton(8192, rightStick.clicked)  // R3 = 1<<13
            }

            // --- ABXY Buttons ---
            Item {
                id: abxyArea
                anchors.right: rightStick.left
                anchors.rightMargin: 40
                anchors.top: parent.top
                anchors.topMargin: 20
                width: 150; height: 150

                // Diamond layout
                // Y (top)
                FaceButton {
                    id: yBtn
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    label: "Y"
                    btnColor: controllerPage.neonYellow
                    bitFlag: 8  // Btn::Y = 1<<3
                    onActiveChanged: updateButton(bitFlag, active)
                }
                // X (left)
                FaceButton {
                    id: xBtn
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    label: "X"
                    btnColor: controllerPage.neonBlue
                    bitFlag: 4  // Btn::X = 1<<2
                    onActiveChanged: updateButton(bitFlag, active)
                }
                // B (right)
                FaceButton {
                    id: bBtn
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    label: "B"
                    btnColor: controllerPage.neonRed
                    bitFlag: 2  // Btn::B = 1<<1
                    onActiveChanged: updateButton(bitFlag, active)
                }
                // A (bottom)
                FaceButton {
                    id: aBtn
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    label: "A"
                    btnColor: controllerPage.neonGreen
                    bitFlag: 1  // Btn::A = 1<<0
                    onActiveChanged: updateButton(bitFlag, active)
                }

                MultiPointTouchArea {
                    anchors.fill: parent
                    minimumTouchPoints: 1
                    maximumTouchPoints: 4

                    touchPoints: [
                        TouchPoint { id: tp1 },
                        TouchPoint { id: tp2 },
                        TouchPoint { id: tp3 },
                        TouchPoint { id: tp4 }
                    ]

                    function updateFaceStates() {
                        let yVal = false
                        let aVal = false
                        let xVal = false
                        let bVal = false

                        let points = [tp1, tp2, tp3, tp4]
                        for (let i = 0; i < points.length; i++) {
                            let tp = points[i]
                            if (tp.pressed) {
                                let tx = tp.x
                                let ty = tp.y

                                // Y zone (top)
                                if (tx >= 35 && tx <= 115 && ty >= 0 && ty <= 60) yVal = true
                                // A zone (bottom)
                                if (tx >= 35 && tx <= 115 && ty >= 90 && ty <= 150) aVal = true
                                // X zone (left)
                                if (tx >= 0 && tx <= 60 && ty >= 35 && ty <= 115) xVal = true
                                // B zone (right)
                                if (tx >= 90 && tx <= 150 && ty >= 35 && ty <= 115) bVal = true
                            }
                        }

                        yBtn.active = yVal
                        aBtn.active = aVal
                        xBtn.active = xVal
                        bBtn.active = bVal
                    }

                    onPressed: updateFaceStates()
                    onUpdated: updateFaceStates()
                    onReleased: updateFaceStates()
                }
            }
        }
    }

    // =============================================
    //  BOTTOM BAR: Motion controls
    // =============================================
    Item {
        id: bottomBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 44

        Rectangle {
            anchors.fill: parent
            color: "#12121a"
            opacity: 0.8
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 12

            // Motion mode toggle
            Rectangle {
                Layout.preferredWidth: 100
                Layout.preferredHeight: 28
                radius: 14
                color: controllerPage.motionReader.mode > 0 ? "#1a2a2a" : "#1a1a24"
                border.color: controllerPage.motionReader.mode > 0 ? controllerPage.neonPurple : "#333"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: {
                        switch(controllerPage.motionReader.mode) {
                            case 0: return "MOTION: OFF"
                            case 1: return "ACCEL"
                            case 2: return "GYRO"
                            case 3: return "BOTH"
                        }
                    }
                    color: controllerPage.motionReader.mode > 0 ? controllerPage.neonPurple : "#666"
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    font.letterSpacing: 1
                }

                MultiPointTouchArea {
                    anchors.fill: parent
                    onPressed: {
                        var next = (controllerPage.motionReader.mode + 1) % 4
                        controllerPage.motionReader.mode = next
                    }
                }
            }

            // Calibrate button
            Rectangle {
                Layout.preferredWidth: 70
                Layout.preferredHeight: 28
                radius: 14
                color: "#1a1a24"
                border.color: "#444"
                border.width: 1
                visible: controllerPage.motionReader.mode > 0

                Text {
                    anchors.centerIn: parent
                    text: "CAL"
                    color: controllerPage.dimWhite
                    font.pixelSize: 10
                    font.weight: Font.Bold
                }

                MultiPointTouchArea {
                    anchors.fill: parent
                    onPressed: controllerPage.motionReader.calibrate()
                }
            }

            // Motion readout
            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                visible: controllerPage.motionReader.mode > 0
                text: {
                    var s = ""
                    if (controllerPage.motionReader.mode === 1 || controllerPage.motionReader.mode === 3)
                        s += "A:" + controllerPage.motionReader.accelX.toFixed(1) + "," +
                             controllerPage.motionReader.accelY.toFixed(1) + "," +
                             controllerPage.motionReader.accelZ.toFixed(1)
                    if (controllerPage.motionReader.mode === 3) s += "  "
                    if (controllerPage.motionReader.mode === 2 || controllerPage.motionReader.mode === 3)
                        s += "G:" + controllerPage.motionReader.gyroX.toFixed(0) + "," +
                             controllerPage.motionReader.gyroY.toFixed(0) + "," +
                             controllerPage.motionReader.gyroZ.toFixed(0)
                    return s
                }
                color: "#555"
                font.pixelSize: 9
                font.family: "monospace"
            }
        }
    }

    // ---- Button state helper ----
    function updateButton(bit, pressed) {
        if (pressed) {
            buttonMask |= bit
        } else {
            buttonMask &= ~bit
        }
        clientController.setButton(bit, pressed)
        // Haptic feedback placeholder
        if (pressed) console.log("haptic: button " + bit)
    }

    // ================================================================
    //  INLINE COMPONENTS
    // ================================================================

    // -------- Analog Stick --------
    component AnalogStick: Item {
        id: stick
        property color stickColor: "#00e5ff"
        property bool isLeftStick: true
        property bool clicked: false

        signal positionChanged(real xVal, real yVal)

        readonly property real stickRadius: width / 2
        readonly property real thumbRadius: 28
        readonly property real deadZone: 0.0

        Rectangle {
            id: stickBase
            anchors.fill: parent
            radius: stick.stickRadius
            color: "transparent"
            border.color: Qt.rgba(stick.stickColor.r, stick.stickColor.g, stick.stickColor.b, 0.15)
            border.width: 2

            // Inner ring
            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.6
                height: parent.height * 0.6
                radius: width / 2
                color: "transparent"
                border.color: Qt.rgba(stick.stickColor.r, stick.stickColor.g, stick.stickColor.b, 0.08)
                border.width: 1
            }
        }

        // Thumb nub
        Rectangle {
            id: thumb
            width: stick.thumbRadius * 2
            height: stick.thumbRadius * 2
            radius: stick.thumbRadius
            x: stick.stickRadius - stick.thumbRadius
            y: stick.stickRadius - stick.thumbRadius
            color: Qt.rgba(stick.stickColor.r, stick.stickColor.g, stick.stickColor.b, 0.25)
            border.color: stick.stickColor
            border.width: 2

            // Inner highlight
            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 0.5
                height: parent.height * 0.5
                radius: width / 2
                color: Qt.rgba(stick.stickColor.r, stick.stickColor.g, stick.stickColor.b, 0.4)
            }

            Behavior on x { NumberAnimation { duration: thumb.resetAnim ? 150 : 0; easing.type: Easing.OutBack } }
            Behavior on y { NumberAnimation { duration: thumb.resetAnim ? 150 : 0; easing.type: Easing.OutBack } }
            property bool resetAnim: false
        }

        MultiPointTouchArea {
            anchors.fill: parent
            minimumTouchPoints: 1
            maximumTouchPoints: 1

            touchPoints: [
                TouchPoint { id: tp }
            ]

            onPressed: {
                thumb.resetAnim = false
            }

            onUpdated: {
                var dx = tp.x - stick.stickRadius
                var dy = tp.y - stick.stickRadius
                var dist = Math.sqrt(dx * dx + dy * dy)
                var maxDist = stick.stickRadius - stick.thumbRadius

                if (dist > maxDist) {
                    dx = dx / dist * maxDist
                    dy = dy / dist * maxDist
                }

                thumb.x = stick.stickRadius + dx - stick.thumbRadius
                thumb.y = stick.stickRadius + dy - stick.thumbRadius

                var nx = dx / maxDist
                var ny = dy / maxDist

                // Apply dead zone
                if (Math.abs(nx) < stick.deadZone) nx = 0
                if (Math.abs(ny) < stick.deadZone) ny = 0

                stick.positionChanged(nx, ny)
            }

            onReleased: {
                thumb.resetAnim = true
                thumb.x = stick.stickRadius - stick.thumbRadius
                thumb.y = stick.stickRadius - stick.thumbRadius
                stick.positionChanged(0, 0)
            }
        }

        // Double-tap for L3/R3
        TapHandler {
            gesturePolicy: TapHandler.ReleaseWithinBounds
            onDoubleTapped: {
                stick.clicked = !stick.clicked
                console.log("haptic: stick click")
            }
        }
    }

    // -------- Face Button (ABXY) --------
    component FaceButton: Item {
        id: faceBtn
        width: 52; height: 52

        property string label: ""
        property color btnColor: "#fff"
        property int bitFlag: 0
        property bool active: false

        Rectangle {
            id: faceBg
            anchors.fill: parent
            radius: width / 2
            color: faceBtn.active ?
                Qt.rgba(faceBtn.btnColor.r, faceBtn.btnColor.g, faceBtn.btnColor.b, 0.35) :
                controllerPage.darkSurface
            border.color: faceBtn.active ? faceBtn.btnColor :
                Qt.rgba(faceBtn.btnColor.r, faceBtn.btnColor.g, faceBtn.btnColor.b, 0.4)
            border.width: 2

            scale: faceBtn.active ? 0.9 : 1.0
            Behavior on scale { NumberAnimation { duration: 60 } }
            Behavior on color { ColorAnimation { duration: 60 } }

            // Glow when pressed
            Rectangle {
                anchors.fill: parent
                anchors.margins: -6
                radius: width / 2
                color: "transparent"
                border.color: faceBtn.active ?
                    Qt.rgba(faceBtn.btnColor.r, faceBtn.btnColor.g, faceBtn.btnColor.b, 0.3) :
                    "transparent"
                border.width: 4
                visible: faceBtn.active
            }
        }

        Text {
            anchors.centerIn: parent
            text: faceBtn.label
            color: faceBtn.active ? "#fff" : faceBtn.btnColor
            font.pixelSize: 18
            font.weight: Font.Bold
            scale: faceBg.scale
        }

    }

    // -------- D-Pad Button --------
    component DpadButton: Item {
        id: dpadBtn
        property string direction: "up"
        property int bitFlag: 0
        property bool active: false

        Rectangle {
            anchors.fill: parent
            radius: 6
            color: dpadBtn.active ?
                Qt.rgba(controllerPage.neonCyan.r, controllerPage.neonCyan.g, controllerPage.neonCyan.b, 0.3) :
                controllerPage.midSurface
            border.color: dpadBtn.active ? controllerPage.neonCyan : "#333340"
            border.width: 1

            Behavior on color { ColorAnimation { duration: 60 } }

            // Arrow
            Text {
                anchors.centerIn: parent
                text: dpadBtn.direction === "up" ? "▲" :
                      dpadBtn.direction === "down" ? "▼" :
                      dpadBtn.direction === "left" ? "◄" : "►"
                color: dpadBtn.active ? controllerPage.neonCyan : "#555"
                font.pixelSize: 14
                rotation: 0  // We use proper arrow chars instead of rotation
            }
        }

        // Override the parent rotation — arrows are already correct
        rotation: 0

    }

    // -------- Shoulder / Generic Button --------
    component GamepadButton: Item {
        id: gpBtn
        property string label: ""
        property color btnColor: "#fff"
        property int bitFlag: 0
        property int fontSize: 13
        property bool pressed: false

        Rectangle {
            anchors.fill: parent
            radius: 10
            color: gpBtn.pressed ?
                Qt.rgba(gpBtn.btnColor.r, gpBtn.btnColor.g, gpBtn.btnColor.b, 0.25) :
                controllerPage.darkSurface
            border.color: gpBtn.pressed ? gpBtn.btnColor :
                Qt.rgba(gpBtn.btnColor.r, gpBtn.btnColor.g, gpBtn.btnColor.b, 0.35)
            border.width: 1.5

            scale: gpBtn.pressed ? 0.93 : 1.0
            Behavior on scale { NumberAnimation { duration: 50 } }
            Behavior on color { ColorAnimation { duration: 50 } }
        }

        Text {
            anchors.centerIn: parent
            text: gpBtn.label
            color: gpBtn.pressed ? "#fff" : gpBtn.btnColor
            font.pixelSize: gpBtn.fontSize
            font.weight: Font.Bold
            font.letterSpacing: 1
        }

        MultiPointTouchArea {
            anchors.fill: parent
            onPressed: gpBtn.pressed = true
            onReleased: gpBtn.pressed = false
        }
    }

    // -------- Trigger Slider --------
    component TriggerSlider: Item {
        id: trigSlider
        property string label: ""
        property color sliderColor: "#00e5ff"
        property real value: 0

        Rectangle {
            anchors.fill: parent
            radius: 10
            color: controllerPage.darkSurface
            border.color: Qt.rgba(trigSlider.sliderColor.r, trigSlider.sliderColor.g, trigSlider.sliderColor.b, 0.3)
            border.width: 1

            // Fill level
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: parent.height * trigSlider.value
                radius: 10
                color: Qt.rgba(trigSlider.sliderColor.r, trigSlider.sliderColor.g, trigSlider.sliderColor.b, 0.3)
                border.color: trigSlider.sliderColor
                border.width: trigSlider.value > 0.01 ? 1 : 0

                Behavior on height { NumberAnimation { duration: 30 } }
            }
        }

        Text {
            anchors.centerIn: parent
            text: trigSlider.label
            color: trigSlider.value > 0.3 ? "#fff" : "#666"
            font.pixelSize: 10
            font.weight: Font.Bold
        }

        MultiPointTouchArea {
            anchors.fill: parent
            minimumTouchPoints: 1
            maximumTouchPoints: 1
            touchPoints: [ TouchPoint { id: trigTp } ]

            onPressed: updateTrigger()
            onUpdated: updateTrigger()
            onReleased: {
                trigSlider.value = 0
            }

            function updateTrigger() {
                // Top = 0, Bottom = 1 (inverted for natural feel)
                var normalized = 1.0 - (trigTp.y / trigSlider.height)
                trigSlider.value = Math.max(0, Math.min(1, normalized))
            }
        }
    }
}
