win32:VERSION = 0.13.0.0
else:VERSION = 0.13.0

DEFINES += \
    VER=\\\"$$VERSION\\\" \
    APP_NAME=\\\"$$TARGET\\\"

RESOURCES += \
    $$PWD/resources.qrc

FORMS += \
    $$PWD/AddSubmoduleDlg.ui \
    $$PWD/CommitWidget.ui \
    $$PWD/BranchDlg.ui \
    $$PWD/CreateRepoDlg.ui \
    $$PWD/TagDlg.ui

HEADERS += \
    $$PWD/AGitProcess.h \
    $$PWD/AddSubmoduleDlg.h \
    $$PWD/BranchContextMenu.h \
    $$PWD/BranchTreeWidget.h \
    $$PWD/BranchesViewDelegate.h \
    $$PWD/BranchesWidget.h \
    $$PWD/ClickableFrame.h \
    $$PWD/CommitHistoryColumns.h \
    $$PWD/CommitHistoryModel.h \
    $$PWD/CommitWidget.h \
    $$PWD/ConfigWidget.h \
    $$PWD/Controls.h \
    $$PWD/CreateRepoDlg.h \
    $$PWD/FileBlameWidget.h \
    $$PWD/FileContextMenu.h \
    $$PWD/FileDiffHighlighter.h \
    $$PWD/FileDiffView.h \
    $$PWD/FileDiffWidget.h \
    $$PWD/FileHistoryWidget.h \
    $$PWD/FileListDelegate.h \
    $$PWD/FileListWidget.h \
    $$PWD/FullDiffWidget.h \
    $$PWD/GeneralConfigPage.h \
    $$PWD/GitAsyncProcess.h \
    $$PWD/GitQlient.h \
    $$PWD/GitQlientRepo.h \
    $$PWD/GitQlientSettings.h \
    $$PWD/GitRequestorProcess.h \
    $$PWD/GitSyncProcess.h \
    $$PWD/RepositoryContextMenu.h \
    $$PWD/RepositoryModel.h \
    $$PWD/RepositoryView.h \
    $$PWD/RepositoryViewDelegate.h \
    $$PWD/Revision.h \
    $$PWD/RevisionFile.h \
    $$PWD/RevisionWidget.h \
    $$PWD/RevisionsCache.h \
    $$PWD/ShaFilterProxyModel.h \
    $$PWD/TagDlg.h \
    $$PWD/Terminal.h \
    $$PWD/UnstagedFilesContextMenu.h \
    $$PWD/git.h \
    $$PWD/lanes.h \
    $$PWD/BranchDlg.h

SOURCES += \
    $$PWD/AGitProcess.cpp \
    $$PWD/AddSubmoduleDlg.cpp \
    $$PWD/BranchContextMenu.cpp \
    $$PWD/BranchTreeWidget.cpp \
    $$PWD/BranchesViewDelegate.cpp \
    $$PWD/BranchesWidget.cpp \
    $$PWD/ClickableFrame.cpp \
    $$PWD/CommitHistoryModel.cpp \
    $$PWD/CommitWidget.cpp \
    $$PWD/ConfigWidget.cpp \
    $$PWD/Controls.cpp \
    $$PWD/CreateRepoDlg.cpp \
    $$PWD/FileBlameWidget.cpp \
    $$PWD/FileContextMenu.cpp \
    $$PWD/FileDiffHighlighter.cpp \
    $$PWD/FileDiffView.cpp \
    $$PWD/FileDiffWidget.cpp \
    $$PWD/FileHistoryWidget.cpp \
    $$PWD/FileListDelegate.cpp \
    $$PWD/FileListWidget.cpp \
    $$PWD/FullDiffWidget.cpp \
    $$PWD/GeneralConfigPage.cpp \
    $$PWD/GitAsyncProcess.cpp \
    $$PWD/GitQlient.cpp \
    $$PWD/GitQlientRepo.cpp \
    $$PWD/GitQlientSettings.cpp \
    $$PWD/GitRequestorProcess.cpp \
    $$PWD/GitSyncProcess.cpp \
    $$PWD/RepositoryContextMenu.cpp \
    $$PWD/RepositoryModel.cpp \
    $$PWD/RepositoryView.cpp \
    $$PWD/RepositoryViewDelegate.cpp \
    $$PWD/Revision.cpp \
    $$PWD/RevisionFile.cpp \
    $$PWD/RevisionWidget.cpp \
    $$PWD/RevisionsCache.cpp \
    $$PWD/ShaFilterProxyModel.cpp \
    $$PWD/TagDlg.cpp \
    $$PWD/Terminal.cpp \
    $$PWD/UnstagedFilesContextMenu.cpp \
    $$PWD/git.cpp \
    $$PWD/lanes.cpp \
    $$PWD/BranchDlg.cpp
