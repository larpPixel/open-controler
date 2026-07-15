cmake_minimum_required(VERSION 3.21)

# --- Fetch zxing-cpp for QR decoding ---
include(FetchContent)
FetchContent_Declare(
    zxing-cpp
    GIT_REPOSITORY https://github.com/zxing-cpp/zxing-cpp.git
    GIT_TAG        v3.1.0
)
set(ZXING_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_BLACKBOX_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_PYTHON_MODULE OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(zxing-cpp)

set_property(TARGET ZXing PROPERTY AUTOMOC OFF)
set_property(TARGET ZXing PROPERTY CXX_STANDARD 20)
set_property(TARGET ZXing PROPERTY CXX_STANDARD_REQUIRED ON)

# --- Android client executable ---
qt_add_executable(qrcontroler_client
    main.cpp
    qrscanner.h qrscanner.cpp
    motionreader.h motionreader.cpp
    clientcontroller.h clientcontroller.cpp
)

# --- QML module ---
qt_add_qml_module(qrcontroler_client
    URI QRController
    VERSION 1.0
    QML_FILES
        Main.qml
        ScannerPage.qml
        ControllerPage.qml

)

# --- Link dependencies ---

target_link_libraries(qrcontroler_client PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Multimedia
    Qt6::Sensors
    qrc_common
    ZXing::ZXing
)

target_include_directories(qrcontroler_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

# Create a symlink so <ZXing/ReadBarcode.h> works
file(CREATE_LINK
    "${CMAKE_BINARY_DIR}/_deps/zxing-cpp-src/core/src"
    "${CMAKE_BINARY_DIR}/ZXing"
    SYMBOLIC
)
target_include_directories(qrcontroler_client PRIVATE
    ${CMAKE_BINARY_DIR}
)
set(QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android" CACHE INTERNAL "")

set_target_properties(qrcontroler_client PROPERTIES
    QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
    QT_ANDROID_MIN_SDK_VERSION "28"
    QT_ANDROID_TARGET_SDK_VERSION "36"
    QT_ANDROID_COMPILE_SDK_VERSION "36"
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
