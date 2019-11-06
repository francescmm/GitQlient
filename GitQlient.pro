# General stuff
TEMPLATE = app
CONFIG += qt warn_on c++17
QMAKE_CXXFLAGS += -Werror
TARGET = GitQlient
QT += widgets

SOURCES += \
    $$PWD/main.cpp

# project files
include(app/GitQlient.pri)
include($$PWD/qtsingleapplication/src/qtsingleapplication.pri)

INCLUDEPATH += QLogger \
    app \
    qtsingleapplication/src

include(QLogger/QLogger.pri)

OTHER_FILES += $$PWD/Tasks.txt \
    $$PWD/LICENSE \
    $$PWD/README.md \
    $$PWD/CONTRIBUTING.md \
    $$PWD/SETUP_BUILD.md
