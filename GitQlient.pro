# General stuff
TEMPLATE = app
CONFIG += qt warn_on c++17
QMAKE_CXXFLAGS += -Werror
TARGET = GitQlient
QT += widgets

# project files
include(GitQlient.pri)

OTHER_FILES += Tasks.txt
