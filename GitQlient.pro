# General stuff
CONFIG += qt warn_on c++17

greaterThan(QT_MINOR_VERSION, 12) {
!msvc:QMAKE_CXXFLAGS += -Werror
}

TARGET = GitQlient
QT += widgets core
DEFINES += QT_DEPRECATED_WARNINGS
QMAKE_LFLAGS += -no-pie

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# project files
SOURCES += main.cpp

include(App.pri)
include(QLogger/QLogger.pri)

INCLUDEPATH += QLogger

OTHER_FILES += \
    $$PWD/LICENSE \
    $$PWD/README.md \
    $$PWD/docs/CONTRIBUTING.md \
    $$PWD/docs/SETUP_BUILD.md \
    $$PWD/.travis.yml

VERSION = 1.0.0

DEFINES += \
    VER=\\\"$$VERSION\\\"

debug {
   DEFINES += DEBUG
}
