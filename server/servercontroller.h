#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QHostAddress>
#include <qqmlintegration.h>

class InputEmulator;

class ServerController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString qrData READ qrData NOTIFY qrDataChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString clientName READ clientName NOTIFY clientNameChanged)
    Q_PROPERTY(QString serverAddress READ serverAddress NOTIFY serverAddressChanged)

public:
    explicit ServerController(QObject *parent = nullptr);
    ~ServerController();

    QString status() const { return m_status; }
    QString qrData() const { return m_qrData; }
    bool isConnected() const { return m_connected; }
    QString clientName() const { return m_clientName; }
    QString serverAddress() const { return m_serverAddress; }

signals:
    void statusChanged();
    void qrDataChanged();
    void connectedChanged();
    void clientNameChanged();
    void serverAddressChanged();

private slots:
    void onReadyRead();
    void onNewTcpConnection();
    void onTcpReadyRead();
    void onTcpDisconnected();
    void onHeartbeatTimeout();

private:
    void startListening();
    QString detectLocalIP();
    void handleHello(const QHostAddress &sender, quint16 senderPort);
    void handleInput(const QByteArray &data);
    void handleHeartbeat(const QHostAddress &sender, quint16 senderPort);
    void handleDisconnect();
    void setStatus(const QString &s);
    void setConnected(bool c);
    void sendPacket(const QByteArray &data, const QHostAddress &target, quint16 port);
    void processPacket(const QByteArray &packet, const QHostAddress &sender, quint16 senderPort);

    QUdpSocket *m_socket = nullptr;
    class QTcpServer *m_tcpServer = nullptr;
    class QTcpSocket *m_tcpSocket = nullptr;
    QTimer *m_heartbeatTimer = nullptr;
    InputEmulator *m_emulator = nullptr;

    QString m_status;
    QString m_qrData;
    QString m_serverAddress;
    bool m_connected = false;
    bool m_isTcp = false;
    QString m_clientName;

    QHostAddress m_clientAddress;
    quint16 m_clientPort = 0;
    quint16 m_listenPort = 0;
};
