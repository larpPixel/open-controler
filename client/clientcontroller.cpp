#include "clientcontroller.h"
#include "protocol.h"
#include <QDebug>
#include <QtMath>

ClientController::ClientController(QObject *parent)
    : QObject(parent)
{
    m_socket = new QUdpSocket(this);
    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(2000);

    m_sendTimer = new QTimer(this);
    m_sendTimer->setInterval(1); // 1ms = 1000Hz
    m_sendTimer->setTimerType(Qt::PreciseTimer);

    connect(m_socket, &QUdpSocket::readyRead, this, &ClientController::onReadyRead);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &ClientController::onHeartbeat);
    connect(m_sendTimer, &QTimer::timeout, this, &ClientController::onSendTimer);
}

ClientController::~ClientController()
{
    if (m_connected) {
        disconnectFromServer();
    }
}

bool ClientController::connected() const { return m_connected; }
int ClientController::latencyMs() const { return m_latencyMs; }

void ClientController::connectToServer(const QString &address)
{
    // Parse "ip:port"
    QStringList parts = address.split(':');
    if (parts.size() != 2) {
        emit connectionFailed("Invalid address format. Expected ip:port");
        return;
    }

    QHostAddress host(parts[0]);
    if (host.isNull()) {
        emit connectionFailed("Invalid IP address: " + parts[0]);
        return;
    }

    bool ok;
    quint16 port = parts[1].toUShort(&ok);
    if (!ok) {
        emit connectionFailed("Invalid port: " + parts[1]);
        return;
    }

    m_serverAddress = host;
    m_serverPort = port;
    m_sequenceNumber = 0;

    m_useTcp = (m_serverAddress == QHostAddress::LocalHost || m_serverAddress.toString() == QStringLiteral("127.0.0.1"));

    if (m_useTcp) {
        if (m_socket->state() == QAbstractSocket::BoundState) {
            m_socket->close();
        }
        if (!m_tcpSocket) {
            m_tcpSocket = new QTcpSocket(this);
            connect(m_tcpSocket, &QTcpSocket::readyRead, this, &ClientController::onTcpReadyRead);
            connect(m_tcpSocket, &QTcpSocket::connected, this, [this]() {
                m_latencyTimer.start();
                sendPacket(QRC::makeHello());
                qDebug() << "TCP connected, sending HELLO";
            });
            connect(m_tcpSocket, &QTcpSocket::disconnected, this, [this]() {
                if (m_connected) {
                    m_connected = false;
                    m_heartbeatTimer->stop();
                    m_sendTimer->stop();
                    emit connectedChanged();
                    qDebug() << "TCP disconnected from server";
                }
            });
        }
        m_tcpSocket->connectToHost(m_serverAddress, m_serverPort);
        qDebug() << "Connecting via TCP to" << m_serverAddress.toString() << ":" << m_serverPort;
    } else {
        if (m_tcpSocket && m_tcpSocket->state() != QAbstractSocket::UnconnectedState) {
            m_tcpSocket->disconnectFromHost();
        }
        // Bind socket to any port
        if (m_socket->state() != QAbstractSocket::BoundState) {
            m_socket->bind(QHostAddress::Any, 0);
        }

        // Send HELLO
        m_latencyTimer.start();
        sendPacket(QRC::makeHello());

        qDebug() << "Sending HELLO to" << m_serverAddress.toString() << ":" << m_serverPort;
    }
}

void ClientController::disconnectFromServer()
{
    if (!m_connected && m_serverPort == 0) return;

    sendPacket(QRC::makeDisconnect());
    m_heartbeatTimer->stop();
    m_sendTimer->stop();
    m_connected = false;
    m_serverPort = 0;

    if (m_useTcp && m_tcpSocket) {
        m_tcpSocket->disconnectFromHost();
    }

    emit connectedChanged();
    qDebug() << "Disconnected from server";
}

void ClientController::sendInput(double lx, double ly, double rx, double ry,
                                  double lt, double rt, int buttons,
                                  double accelX, double accelY, double accelZ,
                                  double gyroX, double gyroY, double gyroZ,
                                  int motionMode)
{
    // Legacy method - just updates volatile state
    setLeftStick(lx, ly);
    setRightStick(rx, ry);
    setTriggers(lt, rt);
    m_currentState.buttons = static_cast<uint16_t>(buttons);
    setAccel(accelX, accelY, accelZ);
    setGyro(gyroX, gyroY, gyroZ);
    setMotionEnabled(motionMode);
}

void ClientController::setLeftStick(double x, double y)
{
    m_currentState.leftStickX = static_cast<int16_t>(qBound(-1.0, x, 1.0) * 32767.0);
    m_currentState.leftStickY = static_cast<int16_t>(qBound(-1.0, y, 1.0) * 32767.0);
}

void ClientController::setRightStick(double x, double y)
{
    m_currentState.rightStickX = static_cast<int16_t>(qBound(-1.0, x, 1.0) * 32767.0);
    m_currentState.rightStickY = static_cast<int16_t>(qBound(-1.0, y, 1.0) * 32767.0);
}

void ClientController::setTriggers(double lt, double rt)
{
    m_currentState.leftTrigger = static_cast<uint8_t>(qBound(0.0, lt, 1.0) * 255.0);
    m_currentState.rightTrigger = static_cast<uint8_t>(qBound(0.0, rt, 1.0) * 255.0);
}

void ClientController::setButton(int bitFlag, bool pressed)
{
    if (pressed) {
        m_currentState.buttons |= static_cast<uint16_t>(bitFlag);
    } else {
        m_currentState.buttons &= ~static_cast<uint16_t>(bitFlag);
    }
}

void ClientController::setAccel(double x, double y, double z)
{
    m_currentState.accelX = static_cast<int16_t>(qBound(-20.0, x, 20.0) / 20.0 * 32767.0);
    m_currentState.accelY = static_cast<int16_t>(qBound(-20.0, y, 20.0) / 20.0 * 32767.0);
    m_currentState.accelZ = static_cast<int16_t>(qBound(-20.0, z, 20.0) / 20.0 * 32767.0);
}

void ClientController::setGyro(double x, double y, double z)
{
    m_currentState.gyroX = static_cast<int16_t>(qBound(-2000.0, x, 2000.0) / 2000.0 * 32767.0);
    m_currentState.gyroY = static_cast<int16_t>(qBound(-2000.0, y, 2000.0) / 2000.0 * 32767.0);
    m_currentState.gyroZ = static_cast<int16_t>(qBound(-2000.0, z, 2000.0) / 2000.0 * 32767.0);
}

void ClientController::setMotionEnabled(int mode)
{
    m_currentState.motionEnabled = static_cast<uint8_t>(mode);
}

void ClientController::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_socket->pendingDatagramSize());
        m_socket->readDatagram(datagram.data(), datagram.size());
        processPacket(datagram);
    }
}

void ClientController::onTcpReadyRead()
{
    if (!m_tcpSocket) return;

    while (m_tcpSocket->bytesAvailable() >= QRC::HEADER_SIZE) {
        QByteArray header = m_tcpSocket->peek(QRC::HEADER_SIZE);
        if (header.size() < QRC::HEADER_SIZE) break;

        if (header[0] != 'Q' || header[1] != 'R') {
            m_tcpSocket->read(1);
            continue;
        }

        uint16_t payloadSize = (static_cast<uint8_t>(header[3]) << 8) | static_cast<uint8_t>(header[4]);
        int packetSize = QRC::HEADER_SIZE + payloadSize;

        if (m_tcpSocket->bytesAvailable() < packetSize) {
            break;
        }

        QByteArray packet = m_tcpSocket->read(packetSize);
        processPacket(packet);
    }
}

void ClientController::processPacket(const QByteArray &packet)
{
    auto type = QRC::peekType(packet);

    switch (type) {
    case QRC::MessageType::Ack:
        if (!m_connected) {
            m_latencyMs = static_cast<int>(m_latencyTimer.elapsed());
            m_connected = true;
            m_heartbeatTimer->start();
            m_sendTimer->start();
            emit connectedChanged();
            emit latencyChanged();
            emit serverConnected();
            qDebug() << "Connected to server, latency:" << m_latencyMs << "ms";
        }
        break;

    case QRC::MessageType::Heartbeat:
        m_latencyMs = static_cast<int>(m_latencyTimer.elapsed());
        emit latencyChanged();
        break;

    case QRC::MessageType::Disconnect:
        m_heartbeatTimer->stop();
        m_sendTimer->stop();
        m_connected = false;
        emit connectedChanged();
        qDebug() << "Server disconnected";
        break;

    default:
        break;
    }
}

void ClientController::onHeartbeat()
{
    if (!m_connected) return;
    m_latencyTimer.restart();
    sendPacket(QRC::makeHeartbeat());
}

void ClientController::onSendTimer()
{
    if (!m_connected) return;
    m_currentState.sequenceNumber = m_sequenceNumber++;
    sendPacket(QRC::serializeInput(m_currentState));
}

void ClientController::sendPacket(const QByteArray &data)
{
    if (m_useTcp && m_tcpSocket && m_tcpSocket->state() == QAbstractSocket::ConnectedState) {
        m_tcpSocket->write(data);
    } else {
        m_socket->writeDatagram(data, m_serverAddress, m_serverPort);
    }
}
