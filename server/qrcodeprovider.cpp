#include "qrcodeprovider.h"
#include <qrcodegen.hpp>
#include <QPainter>

using qrcodegen::QrCode;

QRCodeProvider::QRCodeProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage QRCodeProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QMutexLocker locker(&m_mutex);

    const QString data = id.isEmpty() ? QStringLiteral("empty") : id;

    // Generate QR code
    QrCode qr = QrCode::encodeText(data.toUtf8().constData(), QrCode::Ecc::MEDIUM);

    const int qrSize = qr.getSize();
    const int border = 2;
    const int totalModules = qrSize + border * 2;

    // Determine scale: each module maps to `scale` pixels
    int scale = 8;
    if (requestedSize.isValid() && requestedSize.width() > 0) {
        scale = qMax(1, requestedSize.width() / totalModules);
    }

    const int imgSize = totalModules * scale;

    QImage image(imgSize, imgSize, QImage::Format_RGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setPen(Qt::NoPen);

    for (int y = 0; y < qrSize; ++y) {
        for (int x = 0; x < qrSize; ++x) {
            if (qr.getModule(x, y)) {
                painter.setBrush(Qt::black);
                painter.drawRect(
                    (x + border) * scale,
                    (y + border) * scale,
                    scale, scale
                );
            }
        }
    }

    painter.end();

    if (size)
        *size = image.size();

    return image;
}
