#include "clientcontroller.h"
#include "protocol.h"
#include <QDebug>
#include <QtMath>
#include <QMutexLocker>

// ============================================================================
//  NetworkWorker Implementation (Runs in background thread)
// ============================================================================

NetworkWorker::NetworkWorker(QObject *parent)
    : QObject(parent)
{
}

NetworkWorker::~NetworkWorker()
{
    stop();
}

void NetworkWorker::updateState(const QRC::InputState &state)
{
    QMutexLocker locker(&m_mutex);
    m_currentState = state;
}

void NetworkWorker::init(const QHostAddress &address, quint16 port, bool useTcp)
{
    stop(); // Clean up any old sockets/timers

    m_serverAddress = address;
    m_serverPort = port;
    m_useTcp = useTcp;
    m_sequenceNumber = 0;
    m_connected = false;

    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(2000);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &NetworkWorker::onHeartbeat);

    m_sendTimer = new QTimer(this);
    m_sendTimer->setInterval(1); // 1ms = 1000Hz
    m_sendTimer->setTimerType(Qt::PreciseTimer);
    connect(m_sendTimer, &QTimer::timeout, this, &NetworkWorker::onSendTimer);

    if (m_useTcp) {
        m_tcpSocket = new QTcpSocket(this);
        connect(m_tcpSocket, &QTcpSocket::readyRead, this, &NetworkWorker::onTcpReadyRead);
        connect(m_tcpSocket, &QTcpSocket::connected, this, [this]() {
            m_latencyTimer.start();
            sendPacket(QRC::makeHello());
            qDebug() << "Worker TCP connected, sending HELLO";
        });
        connect(m_tcpSocket, &QTcpSocket::disconnected, this, [this]() {
            if (m_connected) {
                m_connected = false;
                m_heartbeatTimer->stop();
                m_sendTimer->stop();
                emit disconnectedFromServer();
                qDebug() << "Worker TCP disconnected from server";
            }
        });
        connect(m_tcpSocket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError error) {
            qWarning() << "Worker TCP error:" << error;
            emit connectionError("TCP Connection failed");
        });
        m_tcpSocket->connectToHost(m_serverAddress, m_serverPort);
        qDebug() << "Worker connecting via TCP to" << m_serverAddress.toString() << ":" << m_serverPort;
    } else {
        m_socket = new QUdpSocket(this);
        connect(m_socket, &QUdpSocket::readyRead, this, &NetworkWorker::onReadyRead);
        m_socket->bind(QHostAddress::Any, 0);

        m_latencyTimer.start();
        sendPacket(QRC::makeHello());
        qDebug() << "Worker sending HELLO via UDP to" << m_serverAddress.toString() << ":" << m_serverPort;
    }
}

void NetworkWorker::stop()
{
    if (m_sendTimer) {
        m_sendTimer->stop();
        m_sendTimer->deleteLater();
        m_sendTimer = nullptr;
    }
    if (m_heartbeatTimer) {
        m_heartbeatTimer->stop();
        m_heartbeatTimer->deleteLater();
        m_heartbeatTimer = nullptr;
    }
    if (m_tcpSocket) {
        if (m_connected) {
            sendPacket(QRC::makeDisconnect());
        }
        m_tcpSocket->disconnectFromHost();
        m_tcpSocket->deleteLater();
        m_tcpSocket = nullptr;
    }
    if (m_socket) {
        if (m_connected) {
            sendPacket(QRC::makeDisconnect());
        }
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    if (m_connected) {
        m_connected = false;
        emit disconnectedFromServer();
    }
}

void NetworkWorker::onReadyRead()
{
    if (!m_socket) return;
    while (m_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_socket->pendingDatagramSize());
        m_socket->readDatagram(datagram.data(), datagram.size());
        processPacket(datagram);
    }
}

void NetworkWorker::onTcpReadyRead()
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

void NetworkWorker::processPacket(const QByteArray &packet)
{
    auto type = QRC::peekType(packet);

    switch (type) {
    case QRC::MessageType::Ack:
        if (!m_connected) {
            int elapsed = static_cast<int>(m_latencyTimer.elapsed());
            m_connected = true;
            m_heartbeatTimer->start();
            m_sendTimer->start();
            emit ackReceived(elapsed);
            qDebug() << "Worker connected, latency:" << elapsed << "ms";
        }
        break;

    case QRC::MessageType::Heartbeat: {
        int elapsed = static_cast<int>(m_latencyTimer.elapsed());
        emit heartbeatReceived(elapsed);
        break;
    }

    case QRC::MessageType::Disconnect:
        stop();
        break;

    default:
        break;
    }
}

void NetworkWorker::onHeartbeat()
{
    if (!m_connected) return;
    m_latencyTimer.restart();
    sendPacket(QRC::makeHeartbeat());
}

void NetworkWorker::onSendTimer()
{
    if (!m_connected) return;

    QRC::InputState state;
    {
        QMutexLocker locker(&m_mutex);
        state = m_currentState;
    }
    state.sequenceNumber = m_sequenceNumber++;
    sendPacket(QRC::serializeInput(state));
}

void NetworkWorker::sendPacket(const QByteArray &data)
{
    if (m_useTcp && m_tcpSocket && m_tcpSocket->state() == QAbstractSocket::ConnectedState) {
        m_tcpSocket->write(data);
    } else if (!m_useTcp && m_socket) {
        m_socket->writeDatagram(data, m_serverAddress, m_serverPort);
    }
}


// ============================================================================
//  ClientController Implementation (Runs in GUI thread)
// ============================================================================

ClientController::ClientController(QObject *parent)
    : QObject(parent)
{
    m_workerThread = new QThread(this);
    m_worker = new NetworkWorker(); // No parent, will be moved to thread
    m_worker->moveToThread(m_workerThread);

    // Wire up worker signals back to the main thread
    connect(m_worker, &NetworkWorker::ackReceived, this, [this](int latencyMs) {
        m_latencyMs = latencyMs;
        m_connected = true;
        emit connectedChanged();
        emit latencyChanged();
        emit serverConnected();
    });

    connect(m_worker, &NetworkWorker::heartbeatReceived, this, [this](int latencyMs) {
        m_latencyMs = latencyMs;
        emit latencyChanged();
    });

    connect(m_worker, &NetworkWorker::disconnectedFromServer, this, [this]() {
        m_connected = false;
        emit connectedChanged();
    });

    connect(m_worker, &NetworkWorker::connectionError, this, [this](const QString &reason) {
        m_connected = false;
        emit connectedChanged();
        emit connectionFailed(reason);
    });

    // Cleanup worker when thread terminates
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    m_workerThread->start();
}

ClientController::~ClientController()
{
    if (m_workerThread) {
        QMetaObject::invokeMethod(m_worker, "stop", Qt::BlockingQueuedConnection);
        m_workerThread->quit();
        m_workerThread->wait();
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

    bool useTcp = (host == QHostAddress::LocalHost || host.toString() == QStringLiteral("127.0.0.1"));

    QMetaObject::invokeMethod(m_worker, "init", Qt::QueuedConnection,
                              Q_ARG(QHostAddress, host),
                              Q_ARG(quint16, port),
                              Q_ARG(bool, useTcp));
}

void ClientController::disconnectFromServer()
{
    QMetaObject::invokeMethod(m_worker, "stop", Qt::QueuedConnection);
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

void ClientController::pushState()
{
    if (m_worker) {
        m_worker->updateState(m_currentState);
    }
}

void ClientController::setLeftStick(double x, double y)
{
    m_currentState.leftStickX = static_cast<int16_t>(qBound(-1.0, x, 1.0) * 32767.0);
    m_currentState.leftStickY = static_cast<int16_t>(qBound(-1.0, y, 1.0) * 32767.0);
    pushState();
}

void ClientController::setRightStick(double x, double y)
{
    m_currentState.rightStickX = static_cast<int16_t>(qBound(-1.0, x, 1.0) * 32767.0);
    m_currentState.rightStickY = static_cast<int16_t>(qBound(-1.0, y, 1.0) * 32767.0);
    pushState();
}

void ClientController::setTriggers(double lt, double rt)
{
    m_currentState.leftTrigger = static_cast<uint8_t>(qBound(0.0, lt, 1.0) * 255.0);
    m_currentState.rightTrigger = static_cast<uint8_t>(qBound(0.0, rt, 1.0) * 255.0);
    pushState();
}

void ClientController::setButton(int bitFlag, bool pressed)
{
    if (pressed) {
        m_currentState.buttons |= static_cast<uint16_t>(bitFlag);
    } else {
        m_currentState.buttons &= ~static_cast<uint16_t>(bitFlag);
    }
    pushState();
}

void ClientController::setAccel(double x, double y, double z)
{
    m_currentState.accelX = static_cast<int16_t>(qBound(-20.0, x, 20.0) / 20.0 * 32767.0);
    m_currentState.accelY = static_cast<int16_t>(qBound(-20.0, y, 20.0) / 20.0 * 32767.0);
    m_currentState.accelZ = static_cast<int16_t>(qBound(-20.0, z, 20.0) / 20.0 * 32767.0);
    pushState();
}

void ClientController::setGyro(double x, double y, double z)
{
    m_currentState.gyroX = static_cast<int16_t>(qBound(-2000.0, x, 2000.0) / 2000.0 * 32767.0);
    m_currentState.gyroY = static_cast<int16_t>(qBound(-2000.0, y, 2000.0) / 2000.0 * 32767.0);
    m_currentState.gyroZ = static_cast<int16_t>(qBound(-2000.0, z, 2000.0) / 2000.0 * 32767.0);
    pushState();
}

void ClientController::setMotionEnabled(int mode)
{
    m_currentState.motionEnabled = static_cast<uint8_t>(mode);
    pushState();
}
