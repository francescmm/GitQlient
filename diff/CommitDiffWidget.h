#pragma once

#include <QFrame>

class GitBase;
class QLabel;
class FileListWidget;
class RevisionsCache;

class CommitDiffWidget : public QFrame
{
   Q_OBJECT

signals:
   void signalOpenFileCommit(const QString &currentSha, const QString &previousSha, const QString &file);

public:
   explicit CommitDiffWidget(QSharedPointer<GitBase> git, const QSharedPointer<RevisionsCache> &cache,
                             QWidget *parent = nullptr);

   void configure(const QString &firstSha, const QString &secondSha);

private:
   QSharedPointer<GitBase> mGit;
   QLabel *mFirstSha = nullptr;
   QLabel *mSecondSha = nullptr;
   FileListWidget *fileListWidget = nullptr;
   QString mFirstShaStr;
   QString mSecondShaStr;
};
