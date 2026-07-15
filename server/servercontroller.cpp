#include "servercontroller.h"
#include "inputemulator.h"
#include "protocol.h"

#include <QNetworkInterface>
#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>

static constexpr quint16 DEFAULT_PORT = 7654;
static constexpr int HEARTBEAT_TIMEOUT_MS = 6000;

ServerController::ServerController(QObject *parent)
    : QObject(parent)
    , m_socket(new QUdpSocket(this))
    , m_heartbeatTimer(new QTimer(this))
{
    m_heartbeatTimer->setInterval(HEARTBEAT_TIMEOUT_MS);
    m_heartbeatTimer->setSingleShot(true);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &ServerController::onHeartbeatTimeout);

    startListening();
}

ServerController::~ServerController()
{
    if (m_emulator) {
        m_emulator->destroy();
        delete m_emulator;
        m_emulator = nullptr;
    }
}

void ServerController::startListening()
{
    m_listenPort = DEFAULT_PORT;

    if (!m_socket->bind(QHostAddress::Any, m_listenPort)) {
        // Try a few more ports if the default is taken
        for (quint16 p = DEFAULT_PORT + 1; p < DEFAULT_PORT + 10; ++p) {
            if (m_socket->bind(QHostAddress::Any, p)) {
                m_listenPort = p;
                break;
            }
        }
        if (m_socket->state() != QAbstractSocket::BoundState) {
            setStatus(QStringLiteral("Failed to bind UDP socket"));
            qCritical() << "ServerController: Could not bind to any port";
            return;
        }
    }

    connect(m_socket, &QUdpSocket::readyRead, this, &ServerController::onReadyRead);

    // Bind TCP Server on the same port
    m_tcpServer = new QTcpServer(this);
    if (!m_tcpServer->listen(QHostAddress::Any, m_listenPort)) {
        qWarning() << "ServerController: Could not start TCP server on port" << m_listenPort;
    } else {
        connect(m_tcpServer, &QTcpServer::newConnection, this, &ServerController::onNewTcpConnection);
        qInfo() << "ServerController: TCP server listening on port" << m_listenPort;
    }

    const QString ip = detectLocalIP();
    m_serverAddress = QStringLiteral("%1:%2").arg(ip).arg(m_listenPort);
    emit serverAddressChanged();

    m_qrData = m_serverAddress;
    emit qrDataChanged();

    setStatus(QStringLiteral("Waiting for connection…"));
    qInfo() << "ServerController: Listening on" << m_serverAddress;
}

QString ServerController::detectLocalIP()
{
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const auto &iface : interfaces) {
        if (iface.flags().testFlag(QNetworkInterface::IsLoopBack))
            continue;
        if (!iface.flags().testFlag(QNetworkInterface::IsUp))
            continue;
        if (!iface.flags().testFlag(QNetworkInterface::IsRunning))
            continue;

        const auto entries = iface.addressEntries();
        for (const auto &entry : entries) {
            const auto addr = entry.ip();
            if (addr.protocol() == QAbstractSocket::IPv4Protocol
                && !addr.isLoopback()) {
                return addr.toString();
            }
        }
    }

    qWarning() << "ServerController: No suitable network interface found, using localhost";
    return QStringLiteral("127.0.0.1");
}

void ServerController::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_socket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        m_socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        processPacket(datagram, sender, senderPort);
    }
}

void ServerController::onNewTcpConnection()
{
    QTcpSocket *clientSocket = m_tcpServer->nextPendingConnection();
    if (!clientSocket) return;

    if (m_connected) {
        qInfo() << "ServerController: Already connected, ignoring TCP connection from" << clientSocket->peerAddress().toString();
        clientSocket->disconnectFromHost();
        clientSocket->deleteLater();
        return;
    }

    m_tcpSocket = clientSocket;
    m_isTcp = true;
    m_clientAddress = m_tcpSocket->peerAddress();
    m_clientPort = m_tcpSocket->peerPort();

    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &ServerController::onTcpReadyRead);
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, &ServerController::onTcpDisconnected);

    qInfo() << "ServerController: TCP client connected from" << m_clientAddress.toString();
}

void ServerController::onTcpReadyRead()
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
        processPacket(packet, m_tcpSocket->peerAddress(), m_tcpSocket->peerPort());
    }
}

void ServerController::onTcpDisconnected()
{
    if (m_tcpSocket) {
        m_tcpSocket->deleteLater();
        m_tcpSocket = nullptr;
    }
    m_isTcp = false;
    handleDisconnect();
}

void ServerController::processPacket(const QByteArray &packet, const QHostAddress &sender, quint16 senderPort)
{
    if (packet.size() < QRC::HEADER_SIZE)
        return;

    const auto type = QRC::peekType(packet);

    switch (type) {
    case QRC::MessageType::Hello:
        handleHello(sender, senderPort);
        break;
    case QRC::MessageType::Input:
        handleInput(packet);
        break;
    case QRC::MessageType::Heartbeat:
        handleHeartbeat(sender, senderPort);
        break;
    case QRC::MessageType::Disconnect:
        handleDisconnect();
        break;
    default:
        break;
    }
}

void ServerController::handleHello(const QHostAddress &sender, quint16 senderPort)
{
    if (m_connected) {
        qInfo() << "ServerController: Already connected, ignoring Hello from" << sender;
        return;
    }

    m_clientAddress = sender;
    m_clientPort = senderPort;

    // Send ACK
    const auto ack = QRC::makeAck();
    sendPacket(ack, m_clientAddress, m_clientPort);

    // Create input emulator
    m_emulator = new InputEmulator(this);
    if (!m_emulator->init()) {
        qWarning() << "ServerController: InputEmulator failed to init (need uinput permissions?)";
        setStatus(QStringLiteral("Connected (no gamepad — check uinput permissions)"));
    }

    m_clientName = sender.toString();
    emit clientNameChanged();

    setConnected(true);
    setStatus(QStringLiteral("Connected to ") + m_clientName);

    m_heartbeatTimer->start();

    qInfo() << "ServerController: Client connected from" << m_clientName << ":" << senderPort;
}

void ServerController::handleInput(const QByteArray &data)
{
    if (!m_connected)
        return;

    const auto state = QRC::deserializeInput(data);

    if (m_emulator && m_emulator->isValid()) {
        m_emulator->processInput(state);
    }

    // Input acts as an implicit heartbeat
    m_heartbeatTimer->start();
}

void ServerController::handleHeartbeat(const QHostAddress &sender, quint16 senderPort)
{
    if (!m_connected)
        return;

    // Reset heartbeat timer
    m_heartbeatTimer->start();

    // Echo heartbeat back
    const auto hb = QRC::makeHeartbeat();
    sendPacket(hb, sender, senderPort);
}

void ServerController::handleDisconnect()
{
    if (!m_connected)
        return;

    qInfo() << "ServerController: Client" << m_clientName << "disconnected";

    m_heartbeatTimer->stop();

    if (m_emulator) {
        m_emulator->destroy();
        delete m_emulator;
        m_emulator = nullptr;
    }

    if (m_isTcp && m_tcpSocket) {
        m_tcpSocket->disconnectFromHost();
    }

    m_clientAddress.clear();
    m_clientPort = 0;
    m_clientName.clear();
    emit clientNameChanged();
    m_isTcp = false;

    setConnected(false);
    setStatus(QStringLiteral("Waiting for connection…"));
}

void ServerController::onHeartbeatTimeout()
{
    qWarning() << "ServerController: Heartbeat timeout — disconnecting client";
    handleDisconnect();
}

void ServerController::setStatus(const QString &s)
{
    if (m_status != s) {
        m_status = s;
        emit statusChanged();
    }
}

void ServerController::setConnected(bool c)
{
    if (m_connected != c) {
        m_connected = c;
        emit connectedChanged();
    }
}

void ServerController::sendPacket(const QByteArray &data, const QHostAddress &target, quint16 port)
{
    if (m_isTcp && m_tcpSocket && m_tcpSocket->state() == QAbstractSocket::ConnectedState) {
        m_tcpSocket->write(data);
    } else {
        m_socket->writeDatagram(data, target, port);
    }
}
