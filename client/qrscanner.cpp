#include "qrscanner.h"

#include <QDebug>
#include <QtConcurrent/QtConcurrent>

#include <ZXing/ImageView.h>
#include <ZXing/ReadBarcode.h>

QRScanner::QRScanner(QObject *parent) : QObject(parent) {
  m_throttleTimer.start();
}

QRScanner::~QRScanner() {
  if (m_decodeFuture.isRunning()) {
    m_decodeFuture.waitForFinished();
  }
}

QVideoSink *QRScanner::videoSink() const { return m_videoSink; }

void QRScanner::setVideoSink(QVideoSink *sink) {
  if (m_videoSink == sink)
    return;

  if (m_videoSink) {
    disconnect(m_videoSink, &QVideoSink::videoFrameChanged, this,
               &QRScanner::onFrameChanged);
  }

  m_videoSink = sink;

  if (m_videoSink) {
    connect(m_videoSink, &QVideoSink::videoFrameChanged, this,
            &QRScanner::onFrameChanged);
  }

  emit videoSinkChanged();
}

bool QRScanner::scanning() const { return m_scanning; }

void QRScanner::setScanning(bool enabled) {
  if (m_scanning == enabled)
    return;
  m_scanning = enabled;
  emit scanningChanged();
}

void QRScanner::onFrameChanged(const QVideoFrame &frame) {
  if (!m_scanning || m_decoding)
    return;

  // Throttle: only process every ~200ms
  if (m_throttleTimer.elapsed() < 200)
    return;
  m_throttleTimer.restart();

  QVideoFrame f = frame;
  if (!f.map(QVideoFrame::ReadOnly))
    return;

  QImage image = f.toImage();
  f.unmap();

  if (image.isNull())
    return;

  // Convert to grayscale for better recognition
  image = image.convertToFormat(QImage::Format_Grayscale8);

  m_decoding = true;

  m_decodeFuture = QtConcurrent::run([this, image]() { processFrame(image); });
}

void QRScanner::processFrame(const QImage &image) {
  try {
    auto iv = ZXing::ImageView(image.constBits(), image.width(), image.height(),
                               ZXing::ImageFormat::Lum,
                               static_cast<int>(image.bytesPerLine()));

    auto hints = ZXing::ReaderOptions();
    hints.setFormats(ZXing::BarcodeFormat::QRCode);
    hints.setTryHarder(true);
    hints.setTryRotate(true);

    auto results = ZXing::ReadBarcodes(iv, hints);

    if (!results.empty()) {
      QString text = QString::fromStdString(results.front().text());
      if (!text.isEmpty()) {
        QMetaObject::invokeMethod(
            this, [this, text]() { emit codeDetected(text); },
            Qt::QueuedConnection);
      }
    }
  } catch (const std::exception &e) {
    qWarning() << "ZXing decode error:" << e.what();
  }

  QMetaObject::invokeMethod(
      this, [this]() { m_decoding = false; }, Qt::QueuedConnection);
}
