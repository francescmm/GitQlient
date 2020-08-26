#pragma once

#include <QFrame>

namespace DiffHelper
{
struct DiffChange;
}

class LineNumberArea;
class FileDiffView;

class PrChangeListItem : public QFrame
{
public:
   explicit PrChangeListItem(DiffHelper::DiffChange change, QWidget *parent = nullptr);

   void setBookmarks(const QMap<int, int> &bookmarks);
   int getStartingLine() const { return mNewFileStartingLine; }
   QString getFileName() const { return mNewFileName; }

private:
   int mNewFileStartingLine;
   QString mNewFileName;
   FileDiffView *mNewFileDiff = nullptr;
   LineNumberArea *mNewNumberArea = nullptr;
};
