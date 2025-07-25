file(GLOB SRC_FILES
  *.cpp
  *.h
  *.ui
)


add_library(big_widget STATIC
    ${SRC_FILES}
)

target_link_libraries(big_widget
    PUBLIC
    Qt::Core
    Qt::Widgets
    aux_widgets
    branches
    diff
    commits
    history
    config
    QPinnableTabWidget
)
target_include_directories(big_widget PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
