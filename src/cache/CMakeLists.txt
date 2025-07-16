file(GLOB SRC_FILES
  *.cpp
  *.h
  *.ui
)


add_library(cache STATIC
    ${SRC_FILES}
)

target_link_libraries(cache
    PUBLIC
    Qt::Core
    PRIVATE
    BigWidgets
    git
    QLogger
)
target_include_directories(cache PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
