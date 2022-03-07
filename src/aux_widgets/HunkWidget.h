#pragma once

#include <QFrame>

class FileDiffView;
class GitBase;
class GitCache;

class HunkWidget : public QFrame
{
   Q_OBJECT

signals:
   void hunkStaged();

public:
   explicit HunkWidget(QSharedPointer<GitBase> git, QSharedPointer<GitCache> cache, const QString &fileName,
                       const QString &header, const QString &hunk, QWidget *parent = nullptr);

private:
   QSharedPointer<GitBase> mGit;
   QSharedPointer<GitCache> mCache;
   QString mFileName;
   QString mHeader;
   QString mHunk;
   FileDiffView *mHunkView = nullptr;

   void discardHunk();
   void stageHunk();
};
