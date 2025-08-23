file(GLOB SRC_FILES
    include/*.h
    src/*.cpp
    src/*.ui
)

add_library(big_widgets STATIC
    ${SRC_FILES}
)

target_include_directories(big_widgets PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(big_widgets
    PUBLIC
    Qt::Core
    Qt::Widgets
    AuxiliarCustomWidgets
    aux_widgets
    branches
    diff
    commits
    history
    config
    QPinnableTabWidget
)
