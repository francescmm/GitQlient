file(GLOB SRC_FILES
    include/*.h
    src/*.cpp
    src/*.ui
)

add_library(history STATIC
    ${SRC_FILES}
)

target_link_libraries(history
    PUBLIC
    Qt::Core
    Qt::Widgets
    cache
)

target_include_directories(history PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
