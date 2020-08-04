#General stuff
CONFIG += qt warn_on c++ 17 c++1z

greaterThan(QT_MINOR_VERSION, 12) {
!msvc:QMAKE_CXXFLAGS += -Werror
}

TARGET = GitQlient
QT += widgets core network svg
DEFINES += QT_DEPRECATED_WARNINGS
QMAKE_LFLAGS += -no-pie

#Default rules for deployment.
qnx: target.path = $$(HOME)/$${TARGET}/bin
else: unix:!android: target.path = /$$(HOME)/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

#project files
SOURCES += src/main.cpp

include(src/App.pri)
include(QLogger/QLogger.pri)

INCLUDEPATH += QLogger

OTHER_FILES += \
    $$PWD/LICENSE

VERSION = 1.2.0

GQ_SHA = $$system(git rev-parse HEAD)

DEFINES += \
    VER=\\\"$$VERSION\\\" \
    SHA_VER=\\\"$$GQ_SHA\\\"

debug {
   DEFINES += DEBUG
}

macos{
   BUNDLE_FILENAME = $${TARGET}.app
   DMG_FILENAME = "GitQlient-$$(VERSION).dmg"
#Target for pretty DMG generation
   dmg.commands += echo "Generate DMG";
   dmg.commands += macdeployqt $$BUNDLE_FILENAME &&
   dmg.commands += create-dmg \
         --volname $${TARGET} \
         --background $${PWD}/src/resources/dmg_bg.png \
         --icon $${BUNDLE_FILENAME} 150 218 \
         --window-pos 200 120 \
         --window-size 600 450 \
         --icon-size 100 \
         --hdiutil-quiet \
         --app-drop-link 450 218 \
         $${DMG_FILENAME} \
         $${BUNDLE_FILENAME}

   QMAKE_EXTRA_TARGETS += dmg
}
