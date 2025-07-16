file(GLOB SRC_FILES
  *.cpp
  *.h
  *.ui
)

add_library(BigWidgets STATIC
    ${SRC_FILES}
)

target_include_directories(BigWidgets PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(BigWidgets
    PUBLIC
    Qt::Core
    Qt::Widgets
    PRIVATE
    aux_widgets
    AuxiliarCustomWidgets
    branches
    cache
    commits
    diff
    git
    history
    config
    QLogger
    QPinnableTabWidget
)
