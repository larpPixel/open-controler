cmake_minimum_required(VERSION 3.21)
project(qrcontroler VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick Network)
qt_standard_project_setup()

# Shared protocol library
add_subdirectory(common)

if(ANDROID)
    # Phone client app
    find_package(Qt6 REQUIRED COMPONENTS Multimedia Sensors QuickControls2)
    add_subdirectory(client)
else()
    # PC server app — also needs qrcodegen
    include(FetchContent)
    FetchContent_Declare(
        qrcodegen
        GIT_REPOSITORY https://github.com/nayuki/QR-Code-generator.git
        GIT_TAG        master
    )
    FetchContent_MakeAvailable(qrcodegen)

    # Build qrcodegen as a static library from fetched sources
    if(NOT TARGET qrcodegen_lib)
        add_library(qrcodegen_lib STATIC
            ${qrcodegen_SOURCE_DIR}/cpp/qrcodegen.cpp
        )
        target_include_directories(qrcodegen_lib PUBLIC ${qrcodegen_SOURCE_DIR}/cpp)
    endif()

    add_subdirectory(server)
endif()
