INCLUDEPATH += $$PWD

FORMS += \
   $$PWD/CreateIssueDlg.ui \
   $$PWD/CreatePullRequestDlg.ui \
   $$PWD/ServerConfigDlg.ui

HEADERS += \
   $$PWD/CreateIssueDlg.h \
   $$PWD/CreatePullRequestDlg.h \
   $$PWD/GitHubRestApi.h \
   $$PWD/ServerConfigDlg.h \
   $$PWD/ServerIssue.h \
   $$PWD/ServerLabel.h \
   $$PWD/ServerMilestone.h \
   $$PWD/ServerPullRequest.h

SOURCES += \
   $$PWD/CreateIssueDlg.cpp \
   $$PWD/CreatePullRequestDlg.cpp \
   $$PWD/GitHubRestApi.cpp \
   $$PWD/ServerConfigDlg.cpp
