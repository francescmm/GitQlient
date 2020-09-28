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
   Q_OBJECT

signals:
   void gotoReview(int linkId);

public:
   explicit PrChangeListItem(DiffHelper::DiffChange change, QWidget *parent = nullptr);

   void setBookmarks(const QMap<int, int> &bookmarks);
   int getStartingLine() const { return mNewFileStartingLine; }
   int getEndingLine() const { return mNewFileEndingLine; }
   QString getFileName() const { return mNewFileName; }

private:
   int mNewFileStartingLine = 0;
   int mNewFileEndingLine = 0;
   QString mNewFileName;
   FileDiffView *mNewFileDiff = nullptr;
   LineNumberArea *mNewNumberArea = nullptr;
};
