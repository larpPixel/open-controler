# QR Controller

Turn your Android phone into a wireless gamepad for your Linux PC. Connect instantly by scanning a QR code — no pairing, no setup, no internet required.

## How It Works

1. **Launch the server** on your Linux PC — it displays a QR code
2. **Scan the QR code** with the Android app on your phone
3. **Play!** Your phone is now a full gamepad with joysticks, buttons, D-pad, triggers, and motion controls

```
  ┌─────────────┐         UDP over LAN         ┌─────────────┐
  │   Linux PC  │ ◄══════════════════════════► │   Android   │
  │   (Server)  │   ~34 bytes @ 60Hz = 16KB/s  │   (Client)  │
  │             │                               │             │
  │  QR Code ──────── scanned by ──────────────► Camera      │
  │  uinput     │                               │  Joysticks  │
  │  Gamepad    │ ◄── input packets ──────────── Buttons     │
  │             │                               │  Gyroscope  │
  └─────────────┘                               └─────────────┘
```

## Features

- **Instant QR connection** — no manual IP entry, no Bluetooth pairing
- **Ultra-low latency** — UDP protocol, <5ms on local network
- **Full gamepad** — 2 analog sticks, ABXY, D-pad, triggers, shoulders, start/select
- **Motion controls** — accelerometer & gyroscope with calibration
- **Real gamepad device** — uses Linux uinput, works with any game/emulator
- **Lightweight** — 34-byte packets at 60Hz, negligible bandwidth

## Building

### Prerequisites

- Qt 6.5+ (Core, Quick, Network, Multimedia, Sensors)
- CMake 3.21+
- Linux kernel headers (for uinput)
- Android SDK/NDK + JDK (for client)

### Server (Linux PC)

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

### Client (Android)

```bash
mkdir build-android && cd build-android
/path/to/qt6/android_arm64_v8a/bin/qt-cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DANDROID_ABI=arm64-v8a
cmake --build . --parallel
cmake --build . --target apk
adb install client/android-build/qrcontroler_client.apk
```

## uinput Permissions

The server needs access to `/dev/uinput`. Either run with `sudo` or install the udev rule:

```bash
sudo cp udev/99-uinput.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
sudo usermod -aG input $USER
# Log out and back in
```

## Project Structure

```
qrcontroler/
├── CMakeLists.txt              # Top-level build (auto-selects server/client)
├── common/                     # Shared UDP protocol library
│   ├── protocol.h/cpp          # Packet types, InputState, serialization
├── server/                     # Linux PC app
│   ├── inputemulator.h/cpp     # Virtual gamepad via /dev/uinput
│   ├── servercontroller.h/cpp  # Network + orchestration
│   ├── qrcodeprovider.h/cpp    # QR code image generation
│   └── qml/Main.qml           # Server UI
├── client/                     # Android phone app
│   ├── clientcontroller.h/cpp  # UDP networking
│   ├── qrscanner.h/cpp         # Camera QR scanning (ZXing)
│   ├── motionreader.h/cpp      # Gyroscope + accelerometer
│   ├── qml/Main.qml            # App shell
│   ├── qml/ScannerPage.qml     # QR scanner UI
│   ├── qml/ControllerPage.qml  # Virtual gamepad UI
│   └── android/                # AndroidManifest.xml
└── udev/                       # uinput permission rule
```

## Protocol

| Field | Size | Description |
|-------|------|-------------|
| Magic | 2B | `QR` |
| Type | 1B | Hello/Ack/Input/Heartbeat/Disconnect |
| Payload Size | 2B | Big-endian uint16 |
| Payload | NB | Type-specific data |

Input packets are 34 bytes total, sent at 60Hz as full state snapshots. Lost packets are harmless since each packet contains the complete controller state.

## License

MIT
