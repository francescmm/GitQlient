file(GLOB SRC_FILES
  *.cpp
  *.h
  *.ui
)


add_library(history STATIC
    ${SRC_FILES}
)

target_link_libraries(history
    PUBLIC
    Qt::Core
    Qt::Widgets
    PRIVATE
    aux_widgets
    AuxiliarCustomWidgets
    cache
    BigWidgets
    branches
    cache
    git
    QLogger
)
target_include_directories(history PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
