# General stuff
TEMPLATE = app
CONFIG += qt warn_on c++17
QMAKE_CXXFLAGS += -Werror
TARGET = GitQlient
QT += widgets

win32:VERSION = 0.13.0.0
else:VERSION = 0.13.0

DEFINES += \
    VER=\\\"$$VERSION\\\" \
    APP_NAME=\\\"$$TARGET\\\"

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
