INCLUDEPATH += $$PWD

FORMS += \
   $$PWD/CreateIssueDlg.ui \
   $$PWD/ServerConfigDlg.ui

HEADERS += \
   $$PWD/CreateIssueDlg.h \
   $$PWD/GitHubRestApi.h \
   $$PWD/ServerConfigDlg.h \
   $$PWD/ServerIssue.h \
   $$PWD/ServerLabel.h \
   $$PWD/ServerMilestone.h

SOURCES += \
   $$PWD/CreateIssueDlg.cpp \
   $$PWD/GitHubRestApi.cpp \
   $$PWD/ServerConfigDlg.cpp
