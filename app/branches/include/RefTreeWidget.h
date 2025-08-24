#pragma once

#include <QWidget>
#include <memory>

class ClickableFrame;
class BranchTreeWidget;
class GitCache;
class GitBase;
class GitQlientSettings;
class BranchesViewDelegate;

class RefTreeWidget : public QWidget
{
   Q_OBJECT

signals:
   void fullReload();
   void logReload();
   void signalSelectCommit(const QString &sha);
   void signalMergeRequired(const QString &currentBranch, const QString &fromBranch);
   void mergeSqushRequested(const QString &origin, const QString &destination);
   void signalPullConflict();
   void clearSelection();

public:
   enum RefType
   {
      LocalBranches,
      RemoteBranches,
      Tags
   };

   explicit RefTreeWidget(const QString &title,
                          const QString &settingsKey,
                          RefType type,
                          const QSharedPointer<GitCache> &cache,
                          const QSharedPointer<GitBase> &git,
                          QWidget *parent = nullptr);
   ~RefTreeWidget();

   void setCount(int count);
   void adjustBranchesTree();
   void reloadCurrentBranchLink();
   void clear();
   void reloadVisibility();
   void addItems(bool isCurrentBranch, const QString &fullBranchName, const QString &sha);

private:
   ClickableFrame *mFrame = nullptr;
   BranchTreeWidget *mTreeWidget = nullptr;
   RefType mRefType;
   QString mSettingsKey;
   std::unique_ptr<GitQlientSettings> mSettings;
   BranchesViewDelegate *mDelegate = nullptr;

   void saveExpansionState(bool expanded);
};
