#pragma once

#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QThread>
#include <QMutex>
#include <QtQml/qqmlregistration.h>
#include "protocol.h"

class NetworkWorker : public QObject
{
    Q_OBJECT
public:
    explicit NetworkWorker(QObject *parent = nullptr);
    ~NetworkWorker() override;

    void updateState(const QRC::InputState &state);

signals:
    void ackReceived(int latencyMs);
    void heartbeatReceived(int latencyMs);
    void disconnectedFromServer();
    void connectionError(const QString &reason);

public slots:
    void init(const QHostAddress &address, quint16 port, bool useTcp);
    void stop();

private slots:
    void onReadyRead();
    void onTcpReadyRead();
    void onHeartbeat();
    void onSendTimer();

private:
    void sendPacket(const QByteArray &data);
    void processPacket(const QByteArray &packet);

    QUdpSocket *m_socket = nullptr;
    QTcpSocket *m_tcpSocket = nullptr;
    QTimer *m_heartbeatTimer = nullptr;
    QTimer *m_sendTimer = nullptr;
    QElapsedTimer m_latencyTimer;

    QHostAddress m_serverAddress;
    quint16 m_serverPort = 0;
    bool m_useTcp = false;
    bool m_connected = false;
    uint32_t m_sequenceNumber = 0;

    QMutex m_mutex;
    QRC::InputState m_currentState;
};

class ClientController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(int latencyMs READ latencyMs NOTIFY latencyChanged)

public:
    explicit ClientController(QObject *parent = nullptr);
    ~ClientController() override;

    bool connected() const;
    int latencyMs() const;

    Q_INVOKABLE void connectToServer(const QString &address);
    Q_INVOKABLE void disconnectFromServer();
    Q_INVOKABLE void sendInput(double lx, double ly, double rx, double ry,
                               double lt, double rt, int buttons,
                               double accelX, double accelY, double accelZ,
                               double gyroX, double gyroY, double gyroZ,
                               int motionMode);

    // High precision setters for 1000Hz polling rate
    Q_INVOKABLE void setLeftStick(double x, double y);
    Q_INVOKABLE void setRightStick(double x, double y);
    Q_INVOKABLE void setTriggers(double lt, double rt);
    Q_INVOKABLE void setButton(int bitFlag, bool pressed);
    Q_INVOKABLE void setAccel(double x, double y, double z);
    Q_INVOKABLE void setGyro(double x, double y, double z);
    Q_INVOKABLE void setMotionEnabled(int mode);

signals:
    void connectedChanged();
    void latencyChanged();
    void connectionFailed(const QString &reason);
    void serverConnected();

private:
    void pushState();

    QThread *m_workerThread = nullptr;
    NetworkWorker *m_worker = nullptr;

    bool m_connected = false;
    int m_latencyMs = 0;
    QRC::InputState m_currentState;
};
