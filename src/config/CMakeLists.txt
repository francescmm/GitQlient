file(GLOB SRC_FILES
  *.cpp
  *.h
  *.ui
)


add_library(config STATIC
    ${SRC_FILES}
)

target_link_libraries(config
    PUBLIC
    Qt::Core
    Qt::Widgets
    AuxiliarCustomWidgets
    aux_widgets
)
target_include_directories(config PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
