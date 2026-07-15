#include "protocol.h"
#include <QDataStream>
#include <QIODevice>

namespace QRC {

static QByteArray makePacket(MessageType type, const QByteArray& payload = {})
{
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    // Header
    stream << static_cast<uint8_t>('Q');
    stream << static_cast<uint8_t>('R');
    stream << static_cast<uint8_t>(type);
    stream << static_cast<uint16_t>(payload.size());

    // Payload
    if (!payload.isEmpty()) {
        stream.writeRawData(payload.constData(), payload.size());
    }

    return packet;
}

QByteArray serializeInput(const InputState& state)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    stream << state.leftStickX;
    stream << state.leftStickY;
    stream << state.rightStickX;
    stream << state.rightStickY;
    stream << state.leftTrigger;
    stream << state.rightTrigger;
    stream << state.buttons;
    stream << state.accelX;
    stream << state.accelY;
    stream << state.accelZ;
    stream << state.gyroX;
    stream << state.gyroY;
    stream << state.gyroZ;
    stream << state.motionEnabled;
    stream << state.sequenceNumber;

    return makePacket(MessageType::Input, payload);
}

InputState deserializeInput(const QByteArray& data)
{
    InputState state;

    // Skip header (5 bytes)
    QByteArray payload = data.mid(HEADER_SIZE);
    QDataStream stream(payload);
    stream.setByteOrder(QDataStream::BigEndian);

    stream >> state.leftStickX;
    stream >> state.leftStickY;
    stream >> state.rightStickX;
    stream >> state.rightStickY;
    stream >> state.leftTrigger;
    stream >> state.rightTrigger;
    stream >> state.buttons;
    stream >> state.accelX;
    stream >> state.accelY;
    stream >> state.accelZ;
    stream >> state.gyroX;
    stream >> state.gyroY;
    stream >> state.gyroZ;
    stream >> state.motionEnabled;
    stream >> state.sequenceNumber;

    return state;
}

QByteArray makeHello()
{
    return makePacket(MessageType::Hello);
}

QByteArray makeAck()
{
    return makePacket(MessageType::Ack);
}

QByteArray makeHeartbeat()
{
    return makePacket(MessageType::Heartbeat);
}

QByteArray makeDisconnect()
{
    return makePacket(MessageType::Disconnect);
}

MessageType peekType(const QByteArray& data)
{
    if (data.size() < HEADER_SIZE) {
        return MessageType::Disconnect;  // Invalid, treat as disconnect
    }

    // Verify magic bytes
    if (data[0] != 'Q' || data[1] != 'R') {
        return MessageType::Disconnect;  // Invalid packet
    }

    return static_cast<MessageType>(static_cast<uint8_t>(data[2]));
}

} // namespace QRC
