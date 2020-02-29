#pragma once

#include <QFrame>
#include <QMap>

class GitBase;
class QStackedWidget;
class DiffButton;
class QVBoxLayout;
class CommitDiffWidget;
class RevisionsCache;

class DiffWidget : public QFrame
{
   Q_OBJECT

signals:
   void signalShowFileHistory(const QString &fileName);
   void signalDiffEmpty();

public:
   explicit DiffWidget(const QSharedPointer<GitBase> git, QSharedPointer<RevisionsCache> cache,
                       QWidget *parent = nullptr);
   ~DiffWidget() override;

   void reload();

   void clear() const;
   bool loadFileDiff(const QString &sha, const QString &previousSha, const QString &file);
   void loadCommitDiff(const QString &sha, const QString &parentSha);

private:
   QSharedPointer<GitBase> mGit;
   QStackedWidget *centerStackedWidget = nullptr;
   QMap<QString, QPair<QFrame *, DiffButton *>> mDiffButtons;
   QVBoxLayout *mDiffButtonsContainer = nullptr;
   CommitDiffWidget *mCommitDiffWidget = nullptr;

   void changeSelection(int index);
};
