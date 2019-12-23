#pragma once

#include <QFrame>

class Git;
class QLabel;
class FileListWidget;

class CommitDiffWidget : public QFrame
{
   Q_OBJECT

signals:
   void signalOpenFileCommit(const QString &currentSha, const QString &previousSha, const QString &file);

public:
   explicit CommitDiffWidget(QSharedPointer<Git> git, QWidget *parent = nullptr);

   void configure(const QString &firstSha, const QString &secondSha);

private:
   QSharedPointer<Git> mGit;
   QLabel *mFirstSha = nullptr;
   QLabel *mSecondSha = nullptr;
   FileListWidget *fileListWidget = nullptr;
   QString mFirstShaStr;
   QString mSecondShaStr;
};
