file(GLOB SRC_FILES
  *.cpp
  *.h
  *.ui
)


add_library(aux_widgets STATIC
    ${SRC_FILES}
)

target_link_libraries(aux_widgets
    PUBLIC
    Qt::Core
    Qt::Widgets
    Qt::Network
    AuxiliarCustomWidgets
    git
    big_widget
    cache
)
target_include_directories(aux_widgets PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
