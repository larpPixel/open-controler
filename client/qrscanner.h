#pragma once

#include <QObject>
#include <QVideoSink>
#include <QVideoFrame>
#include <QImage>
#include <QElapsedTimer>
#include <QFuture>
#include <QtQml/qqmlregistration.h>

class QRScanner : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVideoSink* videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged)
    Q_PROPERTY(bool scanning READ scanning WRITE setScanning NOTIFY scanningChanged)

public:
    explicit QRScanner(QObject *parent = nullptr);
    ~QRScanner() override;

    QVideoSink *videoSink() const;
    void setVideoSink(QVideoSink *sink);

    bool scanning() const;
    void setScanning(bool enabled);

signals:
    void videoSinkChanged();
    void scanningChanged();
    void codeDetected(const QString &code);

private slots:
    void onFrameChanged(const QVideoFrame &frame);

private:
    void processFrame(const QImage &image);

    QVideoSink *m_videoSink = nullptr;
    bool m_scanning = true;
    QElapsedTimer m_throttleTimer;
    QFuture<void> m_decodeFuture;
    bool m_decoding = false;
};
