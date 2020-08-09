INCLUDEPATH += $$PWD

FORMS += \
   $$PWD/CreateIssueDlg.ui \
   $$PWD/CreatePullRequestDlg.ui \
   $$PWD/MergePullRequestDlg.ui \
   $$PWD/ServerConfigDlg.ui

HEADERS += \
   $$PWD/Assignee.h \
   $$PWD/CreateIssueDlg.h \
   $$PWD/CreatePullRequestDlg.h \
   $$PWD/GitHubRestApi.h \
   $$PWD/GitLabRestApi.h \
   $$PWD/IRestApi.h \
   $$PWD/IssueItem.h \
   $$PWD/MergePullRequestDlg.h \
   $$PWD/ServerConfigDlg.h \
   $$PWD/ServerIssue.h \
   $$PWD/ServerLabel.h \
   $$PWD/ServerMilestone.h \
   $$PWD/ServerPullRequest.h

SOURCES += \
   $$PWD/CreateIssueDlg.cpp \
   $$PWD/CreatePullRequestDlg.cpp \
   $$PWD/GitHubRestApi.cpp \
   $$PWD/GitLabRestApi.cpp \
   $$PWD/IRestApi.cpp \
   $$PWD/IssueItem.cpp \
   $$PWD/MergePullRequestDlg.cpp \
   $$PWD/ServerConfigDlg.cpp
