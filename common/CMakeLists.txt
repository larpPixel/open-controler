add_library(qrc_common STATIC
    protocol.h
    protocol.cpp
)

target_include_directories(qrc_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(qrc_common PUBLIC Qt6::Core)
