file(GLOB SRC_FILES
  *.cpp
  *.h
  *.ui
)


add_library(diff STATIC
    ${SRC_FILES}
)

target_link_libraries(diff
    PUBLIC
    Qt::Core
    Qt::Widgets
    AuxiliarCustomWidgets
    cache
)
target_include_directories(diff PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
