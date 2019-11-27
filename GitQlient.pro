# General stuff
CONFIG += qt warn_on c++17
QMAKE_CXXFLAGS += -Werror
TARGET = GitQlient
QT += widgets core gui
DEFINES += QT_DEPRECATED_WARNINGS
QMAKE_LFLAGS += -no-pie

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# project files
SOURCES += main.cpp

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
    $$PWD/SETUP_BUILD.md \
    $$PWD/.travis.yml
