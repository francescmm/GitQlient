#ifndef DIFFWIDGET_H
#define DIFFWIDGET_H

#include <QFrame>

class Git;
class QStackedWidget;
class FullDiffWidget;
class FileDiffWidget;

class DiffWidget : public QFrame
{
   Q_OBJECT

public:
   explicit DiffWidget(const QSharedPointer<Git> git, QWidget *parent = nullptr);

   void clear() const;
   void loadFileDiff(const QString &sha, const QString &previousSha, const QString &file);
   void loadCommitDiff(const QString &sha, const QString &parentSha);

private:
   QStackedWidget *centerStackedWidget = nullptr;
   FullDiffWidget *mFullDiffWidget = nullptr;
   FileDiffWidget *mFileDiffWidget = nullptr;
};

#endif // DIFFWIDGET_H
