#include "motionreader.h"
#include <QAccelerometerReading>
#include <QGyroscopeReading>
#include <QDebug>

MotionReader::MotionReader(QObject *parent)
    : QObject(parent)
{
    m_accel = new QAccelerometer(this);
    m_gyro = new QGyroscope(this);

    // Fastest data rate
    m_accel->setDataRate(0);  // 0 = fastest available
    m_gyro->setDataRate(0);

    connect(m_accel, &QAccelerometer::readingChanged, this, [this]() {
        auto *r = m_accel->reading();
        if (!r) return;
        m_accelX = r->x() - m_accelOffX;
        m_accelY = r->y() - m_accelOffY;
        m_accelZ = r->z() - m_accelOffZ;
        emit accelChanged();
    });

    connect(m_gyro, &QGyroscope::readingChanged, this, [this]() {
        auto *r = m_gyro->reading();
        if (!r) return;
        m_gyroX = r->x() - m_gyroOffX;
        m_gyroY = r->y() - m_gyroOffY;
        m_gyroZ = r->z() - m_gyroOffZ;
        emit gyroChanged();
    });
}

MotionReader::~MotionReader()
{
    m_accel->stop();
    m_gyro->stop();
}

bool MotionReader::enabled() const { return m_enabled; }

void MotionReader::setEnabled(bool on)
{
    if (m_enabled == on) return;
    m_enabled = on;
    updateSensors();
    emit enabledChanged();
}

int MotionReader::mode() const { return m_mode; }

void MotionReader::setMode(int m)
{
    if (m_mode == m) return;
    m_mode = m;
    m_enabled = (m != 0);
    updateSensors();
    emit modeChanged();
    emit enabledChanged();
}

double MotionReader::accelX() const { return m_accelX; }
double MotionReader::accelY() const { return m_accelY; }
double MotionReader::accelZ() const { return m_accelZ; }

double MotionReader::gyroX() const { return m_gyroX; }
double MotionReader::gyroY() const { return m_gyroY; }
double MotionReader::gyroZ() const { return m_gyroZ; }

void MotionReader::calibrate()
{
    if (m_accel->isActive() && m_accel->reading()) {
        m_accelOffX = m_accel->reading()->x();
        m_accelOffY = m_accel->reading()->y();
        m_accelOffZ = m_accel->reading()->z();
    }
    if (m_gyro->isActive() && m_gyro->reading()) {
        m_gyroOffX = m_gyro->reading()->x();
        m_gyroOffY = m_gyro->reading()->y();
        m_gyroOffZ = m_gyro->reading()->z();
    }
    // Force re-read with new offsets
    m_accelX = 0; m_accelY = 0; m_accelZ = 0;
    m_gyroX = 0; m_gyroY = 0; m_gyroZ = 0;
    emit accelChanged();
    emit gyroChanged();
    qDebug() << "Motion calibrated";
}

void MotionReader::updateSensors()
{
    bool needAccel = m_enabled && (m_mode == 1 || m_mode == 3);
    bool needGyro  = m_enabled && (m_mode == 2 || m_mode == 3);

    if (needAccel && !m_accel->isActive()) {
        m_accel->start();
        qDebug() << "Accelerometer started";
    } else if (!needAccel && m_accel->isActive()) {
        m_accel->stop();
        m_accelX = 0; m_accelY = 0; m_accelZ = 0;
        emit accelChanged();
    }

    if (needGyro && !m_gyro->isActive()) {
        m_gyro->start();
        qDebug() << "Gyroscope started";
    } else if (!needGyro && m_gyro->isActive()) {
        m_gyro->stop();
        m_gyroX = 0; m_gyroY = 0; m_gyroZ = 0;
        emit gyroChanged();
    }
}
