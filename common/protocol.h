#pragma once
#include <cstdint>
#include <QByteArray>

namespace QRC {

// --- Message Types ---
enum class MessageType : uint8_t {
    Hello      = 0x01,  // Client → Server: connection request
    Ack        = 0x02,  // Server → Client: connection accepted
    Input      = 0x10,  // Client → Server: controller input state
    Heartbeat  = 0x20,  // Bidirectional: keepalive
    Disconnect = 0xFF,  // Client → Server: clean disconnect
};

// --- Input State (full snapshot, sent at ~60Hz) ---
struct InputState {
    // Joysticks: normalized to int16 range [-32767, 32767]
    int16_t leftStickX  = 0;
    int16_t leftStickY  = 0;
    int16_t rightStickX = 0;
    int16_t rightStickY = 0;

    // Triggers: [0, 255]
    uint8_t leftTrigger  = 0;
    uint8_t rightTrigger = 0;

    // Buttons: bitmask
    // Bit 0: A, 1: B, 2: X, 3: Y
    // Bit 4: LB, 5: RB, 6: Start, 7: Select
    // Bit 8: DPad Up, 9: Down, 10: Left, 11: Right
    // Bit 12: L3 (left stick press), 13: R3 (right stick press)
    uint16_t buttons = 0;

    // --- Motion Sensor Data ---
    // Accelerometer: m/s², scaled to int16 [-32767, 32767]
    // Maps ~[-20, +20] m/s² → full int16 range
    int16_t accelX = 0;
    int16_t accelY = 0;
    int16_t accelZ = 0;

    // Gyroscope: degrees/sec, scaled to int16 [-32767, 32767]
    // Maps ~[-2000, +2000] °/s → full int16 range
    int16_t gyroX = 0;
    int16_t gyroY = 0;
    int16_t gyroZ = 0;

    // Motion control flags
    uint8_t motionEnabled = 0;  // 0 = off, 1 = accel only, 2 = gyro only, 3 = both

    uint32_t sequenceNumber = 0;  // Monotonic counter for ordering
};

// --- Packet Header ---
struct PacketHeader {
    uint8_t  magic[2] = {'Q', 'R'};  // Magic bytes
    MessageType type;
    uint16_t payloadSize = 0;
};

// Serialize / Deserialize
QByteArray serializeInput(const InputState& state);
InputState deserializeInput(const QByteArray& data);
QByteArray makeHello();
QByteArray makeAck();
QByteArray makeHeartbeat();
QByteArray makeDisconnect();
MessageType peekType(const QByteArray& data);

// Header size constant
constexpr int HEADER_SIZE = 5;  // 2 (magic) + 1 (type) + 2 (payload size)

// Button flag helpers
namespace Btn {
    constexpr uint16_t A     = 1 << 0;
    constexpr uint16_t B     = 1 << 1;
    constexpr uint16_t X     = 1 << 2;
    constexpr uint16_t Y     = 1 << 3;
    constexpr uint16_t LB    = 1 << 4;
    constexpr uint16_t RB    = 1 << 5;
    constexpr uint16_t Start = 1 << 6;
    constexpr uint16_t Select= 1 << 7;
    constexpr uint16_t Up    = 1 << 8;
    constexpr uint16_t Down  = 1 << 9;
    constexpr uint16_t Left  = 1 << 10;
    constexpr uint16_t Right = 1 << 11;
    constexpr uint16_t L3    = 1 << 12;
    constexpr uint16_t R3    = 1 << 13;
}

// Motion control mode flags
namespace Motion {
    constexpr uint8_t Off       = 0;
    constexpr uint8_t AccelOnly = 1;
    constexpr uint8_t GyroOnly  = 2;
    constexpr uint8_t Both      = 3;
}

} // namespace QRC
