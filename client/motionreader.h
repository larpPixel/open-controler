#pragma once

#include <QObject>
#include <QAccelerometer>
#include <QGyroscope>
#include <QtQml/qqmlregistration.h>

class MotionReader : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(double accelX READ accelX NOTIFY accelChanged)
    Q_PROPERTY(double accelY READ accelY NOTIFY accelChanged)
    Q_PROPERTY(double accelZ READ accelZ NOTIFY accelChanged)
    Q_PROPERTY(double gyroX READ gyroX NOTIFY gyroChanged)
    Q_PROPERTY(double gyroY READ gyroY NOTIFY gyroChanged)
    Q_PROPERTY(double gyroZ READ gyroZ NOTIFY gyroChanged)

public:
    explicit MotionReader(QObject *parent = nullptr);
    ~MotionReader() override;

    bool enabled() const;
    void setEnabled(bool on);

    int mode() const;
    void setMode(int m);

    double accelX() const;
    double accelY() const;
    double accelZ() const;

    double gyroX() const;
    double gyroY() const;
    double gyroZ() const;

    Q_INVOKABLE void calibrate();

signals:
    void enabledChanged();
    void modeChanged();
    void accelChanged();
    void gyroChanged();

private:
    void updateSensors();

    QAccelerometer *m_accel = nullptr;
    QGyroscope *m_gyro = nullptr;

    bool m_enabled = false;
    int m_mode = 0;  // 0=off, 1=accel, 2=gyro, 3=both

    double m_accelX = 0, m_accelY = 0, m_accelZ = 0;
    double m_gyroX = 0, m_gyroY = 0, m_gyroZ = 0;

    // Calibration offsets
    double m_accelOffX = 0, m_accelOffY = 0, m_accelOffZ = 0;
    double m_gyroOffX = 0, m_gyroOffY = 0, m_gyroOffZ = 0;
};
