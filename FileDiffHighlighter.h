#ifndef FILEDIFFHIGHLIGHTER_H
#define FILEDIFFHIGHLIGHTER_H

#include <QSyntaxHighlighter>

class FileDiffHighlighter : public QSyntaxHighlighter
{
   Q_OBJECT

public:
   explicit FileDiffHighlighter(QTextDocument *document);

   void highlightBlock(const QString &text) override;
   QVector<int> getModifiedRows() const { return mModifiedRows; }
   void resetState();

private:
   bool mFirstModificationFound = false;
   QVector<int> mModifiedRows;
};
#endif // FILEDIFFHIGHLIGHTER_H
