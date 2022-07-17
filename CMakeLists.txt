cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

set(APP_NAME "GitQlient")

project(${APP_NAME} LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(SHA_VER="000000")
add_compile_definitions(VER="1.5.0")
add_compile_definitions(QT_DEPRECATED_WARNINGS)

#list(APPEND CPP_FLAGS "-Wall")
#
#if (NOT WIN32)
#   list(APPEND CPP_FLAGS "-Wextra" "-Wpedantic")
#endif()

find_package(Qt5 COMPONENTS Core REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5 COMPONENTS Network REQUIRED)

add_executable(${APP_NAME} "${PROJECT_SOURCE_DIR}/src/main.cpp")

file(GLOB SRC_FILES
   ${PROJECT_SOURCE_DIR}/src/GitQlient.qrc
   ${PROJECT_SOURCE_DIR}/src/AuxiliarCustomWidgets/*.cpp
   ${PROJECT_SOURCE_DIR}/src/AuxiliarCustomWidgets/*.h
   ${PROJECT_SOURCE_DIR}/src/AuxiliarCustomWidgets/*.ui
   ${PROJECT_SOURCE_DIR}/src/AuxiliarCustomWidgets/*.qrc
   ${PROJECT_SOURCE_DIR}/src/aux_widgets/*.cpp
   ${PROJECT_SOURCE_DIR}/src/aux_widgets/*.h
   ${PROJECT_SOURCE_DIR}/src/aux_widgets/*.ui
   ${PROJECT_SOURCE_DIR}/src/big_widgets/*.cpp
   ${PROJECT_SOURCE_DIR}/src/big_widgets/*.h
   ${PROJECT_SOURCE_DIR}/src/big_widgets/*.ui
   ${PROJECT_SOURCE_DIR}/src/branches/*.cpp
   ${PROJECT_SOURCE_DIR}/src/branches/*.h
   ${PROJECT_SOURCE_DIR}/src/branches/*.ui
   ${PROJECT_SOURCE_DIR}/src/cache/*.cpp
   ${PROJECT_SOURCE_DIR}/src/cache/*.h
   ${PROJECT_SOURCE_DIR}/src/cache/*.ui
   ${PROJECT_SOURCE_DIR}/src/commits/*.cpp
   ${PROJECT_SOURCE_DIR}/src/commits/*.h
   ${PROJECT_SOURCE_DIR}/src/commits/*.ui
   ${PROJECT_SOURCE_DIR}/src/config/*.cpp
   ${PROJECT_SOURCE_DIR}/src/config/*.h
   ${PROJECT_SOURCE_DIR}/src/config/*.ui
   ${PROJECT_SOURCE_DIR}/src/diff/*.cpp
   ${PROJECT_SOURCE_DIR}/src/diff/*.h
   ${PROJECT_SOURCE_DIR}/src/diff/*.ui
   ${PROJECT_SOURCE_DIR}/src/git/*.cpp
   ${PROJECT_SOURCE_DIR}/src/git/*.h
   ${PROJECT_SOURCE_DIR}/src/git/*.ui
   ${PROJECT_SOURCE_DIR}/src/git_server/*.cpp
   ${PROJECT_SOURCE_DIR}/src/git_server/*.h
   ${PROJECT_SOURCE_DIR}/src/git_server/*.ui
   ${PROJECT_SOURCE_DIR}/src/history/*.cpp
   ${PROJECT_SOURCE_DIR}/src/history/*.h
   ${PROJECT_SOURCE_DIR}/src/history/*.ui
   ${PROJECT_SOURCE_DIR}/src/jenkins/*.cpp
   ${PROJECT_SOURCE_DIR}/src/jenkins/*.h
   ${PROJECT_SOURCE_DIR}/src/jenkins/*.ui
   ${PROJECT_SOURCE_DIR}/src/QLogger/*.cpp
   ${PROJECT_SOURCE_DIR}/src/QLogger/*.h
   ${PROJECT_SOURCE_DIR}/src/QLogger/*.ui
   ${PROJECT_SOURCE_DIR}/src/QPinnableTabWidget/*.cpp
   ${PROJECT_SOURCE_DIR}/src/QPinnableTabWidget/*.h
   ${PROJECT_SOURCE_DIR}/src/QPinnableTabWidget/*.ui
   ${PROJECT_SOURCE_DIR}/src/terminal/*.cpp
   ${PROJECT_SOURCE_DIR}/src/terminal/*.h
   ${PROJECT_SOURCE_DIR}/src/terminal/*.ui
)

target_sources(${APP_NAME} PRIVATE ${SRC_FILES} ${RC_FILES})

#set_target_properties(${APP_NAME} PROPERTIES
#    WIN32_EXECUTABLE TRUE
#    MACOSX_BUNDLE TRUE
#)

#target_compile_options(${APP_NAME}
#   PRIVATE
#   ${CPP_FLAGS}
#)

target_include_directories(${APP_NAME}
   PRIVATE
   ${PROJECT_SOURCE_DIR}/src
   ${PROJECT_SOURCE_DIR}/src/AuxiliarCustomWidgets
   ${PROJECT_SOURCE_DIR}/src/aux_widgets
   ${PROJECT_SOURCE_DIR}/src/big_widgets
   ${PROJECT_SOURCE_DIR}/src/branches
   ${PROJECT_SOURCE_DIR}/src/cache
   ${PROJECT_SOURCE_DIR}/src/commits
   ${PROJECT_SOURCE_DIR}/src/config
   ${PROJECT_SOURCE_DIR}/src/diff
   ${PROJECT_SOURCE_DIR}/src/git
   ${PROJECT_SOURCE_DIR}/src/git_server
   ${PROJECT_SOURCE_DIR}/src/history
   ${PROJECT_SOURCE_DIR}/src/jenkins
   ${PROJECT_SOURCE_DIR}/src/QLogger
   ${PROJECT_SOURCE_DIR}/src/QPinnableTabWidget
   ${PROJECT_SOURCE_DIR}/src/terminal
)

target_link_libraries(${APP_NAME}
    PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
    Qt::Network
)


if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
   target_link_options(${APP_NAME} PUBLIC "-no-pie")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
endif()

install(TARGETS ${APP_NAME}
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
