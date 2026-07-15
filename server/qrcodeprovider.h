#pragma once

#include <QQuickImageProvider>
#include <QMutex>

class QRCodeProvider : public QQuickImageProvider
{
public:
    QRCodeProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    QMutex m_mutex;
};
