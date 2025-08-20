file(GLOB SRC_FILES
  *.cpp
  *.h
  *.ui
  core/*.cpp
  core/*.h
  graph/*.cpp
  graph/*.h
  storage/*.cpp
  storage/*.h
)


add_library(cache STATIC
    ${SRC_FILES}
)

target_link_libraries(cache
    PUBLIC
    Qt::Core
    git
    big_widget
)
target_include_directories(cache PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
