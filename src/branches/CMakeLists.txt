file(GLOB SRC_FILES
  *.cpp
  *.h
  *.ui
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
target_include_directories(branches PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
