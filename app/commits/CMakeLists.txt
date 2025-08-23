file(GLOB SRC_FILES
    include/*.h
    src/*.cpp
    src/*.ui
)


add_library(commits STATIC
    ${SRC_FILES}
)

target_include_directories(commits PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(commits
    PUBLIC
    Qt::Core
    Qt::Widgets
    cache
)
