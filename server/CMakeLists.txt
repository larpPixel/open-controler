qt_add_executable(qrc_server
    main.cpp
    inputemulator.h inputemulator.cpp
    qrcodeprovider.h qrcodeprovider.cpp
    servercontroller.h servercontroller.cpp
)

qt_add_qml_module(qrc_server
    URI QRControllerServer
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_link_libraries(qrc_server PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::Network
    qrcodegen_lib
    qrc_common
)
