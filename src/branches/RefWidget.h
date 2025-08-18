#pragma once

#include <ClickableFrame.h>
#include <GitQlientSettings.h>

namespace Ui
{
class RefWidget;
}

class BranchesViewDelegate;
class GitCache;
class GitBase;

class RefWidget : public ClickableFrame
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
   explicit RefWidget(const QString &header, const QString& settingsKey, const QSharedPointer<GitCache> &cache, const QSharedPointer<GitBase> &git, QWidget *parent = nullptr);
   ~RefWidget();

   void setCount(const QString &count);

   void adjustBranchesTree();
   void reloadCurrentBranchLink();
   void clear();
   void reloadVisibility();
   void addItems(bool isCurrentBranch, const QString& fullBranchName, const QString& sha);

private:
   Ui::RefWidget *ui;
   QString mSettingsKey;
   GitQlientSettings mSettings;
   BranchesViewDelegate* mLocalDelegate = nullptr;

   void onHeaderClicked();
};
