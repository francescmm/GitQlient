#include "DiffWidget.h"

#include <GitCache.h>
#include <CommitInfoPanel.h>
#include <FileDiffWidget.h>
#include <FullDiffWidget.h>
#include <CommitDiffWidget.h>
#include <GitQlientSettings.h>
#include <GitHistory.h>

#include <QPinnableTabWidget.h>
#include <QLogger.h>

#include <QMessageBox>
#include <QHBoxLayout>

using namespace QLogger;

DiffWidget::DiffWidget(const QSharedPointer<GitBase> git, QSharedPointer<GitCache> cache, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mCache(cache)
   , mInfoPanelBase(new CommitInfoPanel())
   , mInfoPanelParent(new CommitInfoPanel())
   , mCenterStackedWidget(new QPinnableTabWidget())
   , mCommitDiffWidget(new CommitDiffWidget(mGit, mCache))
{
   setAttribute(Qt::WA_DeleteOnClose);

   mCenterStackedWidget->setCurrentIndex(0);
   mCenterStackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
   connect(mCenterStackedWidget, &QTabWidget::currentChanged, this, &DiffWidget::changeSelection);
   connect(mCenterStackedWidget, &QTabWidget::tabCloseRequested, this, &DiffWidget::onTabClosed);

   const auto diffsLayout = new QVBoxLayout();
   diffsLayout->setContentsMargins(QMargins());
   diffsLayout->setSpacing(0);
   diffsLayout->addWidget(mInfoPanelBase);
   diffsLayout->addWidget(mCommitDiffWidget);
   diffsLayout->addItem(new QSpacerItem(300, 1, QSizePolicy::Fixed, QSizePolicy::Fixed));
   diffsLayout->addStretch();
   diffsLayout->addWidget(mInfoPanelParent);

   const auto layout = new QHBoxLayout();
   layout->setContentsMargins(QMargins());
   layout->addLayout(diffsLayout);
   layout->setSpacing(10);
   layout->addWidget(mCenterStackedWidget);

   setLayout(layout);

   connect(mCommitDiffWidget, &CommitDiffWidget::signalOpenFileCommit, this, &DiffWidget::loadFileDiff);
   connect(mCommitDiffWidget, &CommitDiffWidget::signalShowFileHistory, this, &DiffWidget::signalShowFileHistory);

   mCommitDiffWidget->setVisible(false);
}

DiffWidget::~DiffWidget()
{
   mDiffWidgets.clear();
   blockSignals(true);
}

void DiffWidget::reload()
{
   if (mCenterStackedWidget->count() > 0)
   {
      if (const auto fileDiff = dynamic_cast<FileDiffWidget *>(mCenterStackedWidget->currentWidget()))
         fileDiff->reload();
      else if (const auto fullDiff = dynamic_cast<FullDiffWidget *>(mCenterStackedWidget->currentWidget()))
         fullDiff->reload();
   }
}

void DiffWidget::clear() const
{
   mCenterStackedWidget->setCurrentIndex(0);
}

bool DiffWidget::loadFileDiff(const QString &currentSha, const QString &previousSha, const QString &file, bool isCached)
{
   const auto id = QString("%1 (%2 \u2194 %3)").arg(file.split("/").last(), currentSha.left(6), previousSha.left(6));

   if (!mDiffWidgets.contains(id))
   {
      QLog_Info(
          "UI",
          QString("Requested diff for file {%1} on between commits {%2} and {%3}").arg(file, currentSha, previousSha));

      const auto fileDiffWidget = new FileDiffWidget(mGit, mCache);
      const auto fileWithModifications = fileDiffWidget->configure(currentSha, previousSha, file, isCached);

      if (fileWithModifications)
      {
         mInfoPanelBase->configure(mCache->getCommitInfo(currentSha));
         mInfoPanelParent->configure(mCache->getCommitInfo(previousSha));

         mDiffWidgets.insert(id, fileDiffWidget);

         const auto index = mCenterStackedWidget->addTab(fileDiffWidget, file.split("/").last());
         mCenterStackedWidget->setCurrentIndex(index);

         mCommitDiffWidget->configure(currentSha, previousSha);
         mCommitDiffWidget->setVisible(true);

         return true;
      }
      else
      {
         QMessageBox::information(this, tr("No modifications"), tr("There are no content modifications for this file"));
         delete fileDiffWidget;

         return false;
      }
   }
   else
   {
      const auto diffWidget = mDiffWidgets.value(id);
      const auto diff = dynamic_cast<FileDiffWidget *>(diffWidget);
      diff->reload();

      mCenterStackedWidget->setCurrentWidget(diff);

      return true;
   }
}

bool DiffWidget::loadCommitDiff(const QString &sha, const QString &parentSha)
{
   const auto id = QString("Commit diff (%1 \u2194 %2)").arg(sha.left(6), parentSha.left(6));

   if (!mDiffWidgets.contains(id))
   {
      QScopedPointer<GitHistory> git(new GitHistory(mGit));
      const auto ret = git->getCommitDiff(sha, parentSha);

      if (ret.success && !ret.output.toString().isEmpty())
      {
         const auto fullDiffWidget = new FullDiffWidget(mGit, mCache);
         fullDiffWidget->loadDiff(sha, parentSha, ret.output.toString());

         mInfoPanelBase->configure(mCache->getCommitInfo(sha));
         mInfoPanelParent->configure(mCache->getCommitInfo(parentSha));

         mDiffWidgets.insert(id, fullDiffWidget);

         const auto index = mCenterStackedWidget->addTab(fullDiffWidget,
                                                         QString("(%1 \u2194 %2)").arg(sha.left(6), parentSha.left(6)));
         mCenterStackedWidget->setCurrentIndex(index);

         mCommitDiffWidget->configure(sha, parentSha);
         mCommitDiffWidget->setVisible(true);

         return true;
      }
      else
         QMessageBox::information(this, tr("No diff to show!"),
                                  tr("There is no diff to show between commit SHAs {%1} and {%2}").arg(sha, parentSha));

      return false;
   }
   else
   {
      const auto diffWidget = mDiffWidgets.value(id);
      const auto diff = dynamic_cast<FullDiffWidget *>(diffWidget);
      diff->reload();
      mCenterStackedWidget->setCurrentWidget(diff);
   }

   return true;
}

void DiffWidget::changeSelection(int index)
{
   const auto widget = qobject_cast<IDiffWidget *>(mCenterStackedWidget->widget(index));

   if (widget)
   {
      mInfoPanelBase->configure(mCache->getCommitInfo(widget->getCurrentSha()));
      mInfoPanelParent->configure(mCache->getCommitInfo(widget->getPreviousSha()));
   }
   else
      emit signalDiffEmpty();
}

void DiffWidget::onTabClosed(int index)
{
   const auto widget = qobject_cast<IDiffWidget *>(mCenterStackedWidget->widget(index));

   if (widget)
   {
      const auto key = mDiffWidgets.key(widget);
      mDiffWidgets.remove(key);
   }
}
