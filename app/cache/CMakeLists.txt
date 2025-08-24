file(GLOB SRC_FILES
    include/*.h
    src/*.cpp
)

add_library(
    cache
    STATIC
    ${SRC_FILES}
)

target_link_libraries(cache
    PUBLIC
    Qt::Core
    AuxiliarCustomWidgets
    big_widgets
    git
)

target_include_directories(cache PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
