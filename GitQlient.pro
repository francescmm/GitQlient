# General stuff
TEMPLATE = app
CONFIG += qt warn_on c++17
QMAKE_CXXFLAGS += -Werror
TARGET = GitQlient
QT += widgets

# project files
include(app/GitQlient.pri)

INCLUDEPATH += QLogger \
    app

include(QLogger/QLogger.pri)

OTHER_FILES += $$PWD/Tasks.txt \
    $$PWD/LICENSE \
    $$PWD/README.md \
    $$PWD/CONTRIBUTING.md \
    $$PWD/SETUP_BUILD.md
