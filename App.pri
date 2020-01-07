include($$PWD/aux_widgets/AuxiliarWidgets.pri)
include($$PWD/big_widgets/BigWidgets.pri)
include($$PWD/branches/Branches.pri)
include($$PWD/commits/Commits.pri)
include($$PWD/config/Config.pri)
include($$PWD/diff/Diff.pri)
include($$PWD/git/Git.pri)
include($$PWD/history/History.pri)

DEFINES += \
    VER=\\\"$$VERSION\\\" \
    APP_NAME=\\\"$$TARGET\\\"

RESOURCES += \
    $$PWD/resources.qrc
