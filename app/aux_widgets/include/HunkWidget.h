#pragma once

#include <QFrame>

class FileDiffView;
class GitBase;
class GitCache;
class QTemporaryFile;

class HunkWidget : public QFrame
{
   Q_OBJECT

signals:
   void hunkStaged();

public:
   explicit HunkWidget(QSharedPointer<GitBase> git, QSharedPointer<GitCache> cache, const QString &fileName,
                       const QString &header, const QString &hunk, bool isCached = false, bool isEditable = false,
                       QWidget *parent = nullptr);

private:
   QSharedPointer<GitBase> mGit;
   QSharedPointer<GitCache> mCache;
   QString mFileName;
   QString mHeader;
   QString mHunk;
   bool mIsCached = false;
   FileDiffView *mHunkView = nullptr;
   int mLineToDiscard {};

   QTemporaryFile *createPatchFile();

   void discardHunk();
   void stageHunk();
   void stageLine();
   void discardLine();
   void revertLine();

   void showContextMenu(const QPoint &pos);
};
