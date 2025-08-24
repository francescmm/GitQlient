file(GLOB SRC_FILES
    include/*.h
    src/*.cpp
    src/*.ui
)

add_library(branches STATIC
    ${SRC_FILES}
)

target_link_libraries(branches
    PUBLIC
    Qt::Core
    Qt::Widgets
    aux_widgets
)

target_include_directories(branches PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
