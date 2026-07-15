#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>

#include "qrcodeprovider.h"
#include "servercontroller.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("QR Controller Server"));
    app.setOrganizationName(QStringLiteral("QRController"));

    QQmlApplicationEngine engine;

    // Register QR code image provider (engine takes ownership)
    engine.addImageProvider(QStringLiteral("qrcode"), new QRCodeProvider);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection
    );

    engine.loadFromModule("QRControllerServer", "Main");

    return app.exec();
}
