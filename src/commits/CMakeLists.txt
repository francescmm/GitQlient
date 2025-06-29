file(GLOB SRC_FILES
  *.cpp
  *.h
  *.ui
)


add_library(commits STATIC
    ${SRC_FILES}
)

target_link_libraries(commits
    PUBLIC
    Qt::Core
    Qt::Widgets
    cache
)
target_include_directories(commits PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
