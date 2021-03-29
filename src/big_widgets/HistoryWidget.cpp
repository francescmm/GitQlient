#include "HistoryWidget.h"

#include <AmendWidget.h>
#include <BranchesWidget.h>
#include <CheckBox.h>
#include <CommitHistoryModel.h>
#include <CommitHistoryView.h>
#include <CommitInfo.h>
#include <CommitInfoWidget.h>
#include <FileDiffWidget.h>
#include <FileEditor.h>
#include <FullDiffWidget.h>
#include <GitBase.h>
#include <GitBranches.h>
#include <GitCache.h>
#include <GitConfig.h>
#include <GitHistory.h>
#include <GitLocal.h>
#include <GitMerge.h>
#include <GitQlientSettings.h>
#include <GitQlientStyles.h>
#include <GitRemote.h>
#include <GitRepoLoader.h>
#include <GitWip.h>
#include <RepositoryViewDelegate.h>
#include <WipWidget.h>

#include <QLogger.h>

#include <QApplication>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QSplitter>
#include <QStackedWidget>

using namespace QLogger;

HistoryWidget::HistoryWidget(const QSharedPointer<GitCache> &cache, const QSharedPointer<GitBase> git,
                             const QSharedPointer<GitServerCache> &gitServerCache,
                             const QSharedPointer<GitQlientSettings> &settings, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mCache(cache)
   , mGitServerCache(gitServerCache)
   , mSettings(settings)
   , mWipWidget(new WipWidget(mCache, mGit))
   , mAmendWidget(new AmendWidget(mCache, mGit))
   , mCommitInfoWidget(new CommitInfoWidget(mCache, mGit))
   , mReturnFromFull(new QPushButton())
   , mUserName(new QLabel())
   , mUserEmail(new QLabel())
   , mSplitter(new QSplitter())
{
   setAttribute(Qt::WA_DeleteOnClose);

   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
   const auto localUserInfo = gitConfig->getLocalUserInfo();
   const auto globalUserInfo = gitConfig->getGlobalUserInfo();

   mUserName->setText(localUserInfo.mUserName.isEmpty() ? globalUserInfo.mUserName : localUserInfo.mUserName);
   mUserEmail->setText(localUserInfo.mUserEmail.isEmpty() ? globalUserInfo.mUserEmail : localUserInfo.mUserEmail);

   const auto wipInfoFrame = new QFrame();
   wipInfoFrame->setObjectName("wipInfoFrame");
   const auto wipInfoLayout = new QVBoxLayout(wipInfoFrame);
   wipInfoLayout->setContentsMargins(QMargins());
   wipInfoLayout->setSpacing(10);
   wipInfoLayout->addWidget(mUserName);
   wipInfoLayout->addWidget(mUserEmail);

   mCommitStackedWidget = new QStackedWidget();
   mCommitStackedWidget->setCurrentIndex(0);
   mCommitStackedWidget->addWidget(mCommitInfoWidget);
   mCommitStackedWidget->addWidget(mWipWidget);
   mCommitStackedWidget->addWidget(mAmendWidget);

   const auto wipLayout = new QVBoxLayout();
   wipLayout->setContentsMargins(QMargins());
   wipLayout->setSpacing(10);
   wipLayout->addWidget(wipInfoFrame);
   wipLayout->addWidget(mCommitStackedWidget);

   const auto wipFrame = new QFrame();
   wipFrame->setLayout(wipLayout);
   wipFrame->setMinimumWidth(250);
   wipFrame->setMaximumWidth(500);

   connect(mWipWidget, &WipWidget::signalShowDiff, this, &HistoryWidget::showFileDiff);
   connect(mWipWidget, &WipWidget::signalChangesCommitted, this, &HistoryWidget::returnToView);
   connect(mWipWidget, &WipWidget::signalChangesCommitted, this, &HistoryWidget::signalChangesCommitted);
   connect(mWipWidget, &WipWidget::signalCheckoutPerformed, this, &HistoryWidget::signalUpdateUi);
   connect(mWipWidget, &WipWidget::signalShowFileHistory, this, &HistoryWidget::signalShowFileHistory);
   connect(mWipWidget, &WipWidget::signalUpdateWip, this, &HistoryWidget::signalUpdateWip);
   connect(mWipWidget, &WipWidget::changeReverted, this, [this](const QString &revertedFile) {
      if (mFileDiff->getCurrentFile().contains(revertedFile))
         returnToView();

      emit signalUpdateUi();
   });

   connect(mAmendWidget, &AmendWidget::signalShowDiff, this, &HistoryWidget::showFileDiff);
   connect(mAmendWidget, &AmendWidget::signalChangesCommitted, this, &HistoryWidget::returnToView);
   connect(mAmendWidget, &AmendWidget::signalChangesCommitted, this, &HistoryWidget::signalChangesCommitted);
   connect(mAmendWidget, &AmendWidget::signalCheckoutPerformed, this, &HistoryWidget::signalUpdateUi);
   connect(mAmendWidget, &AmendWidget::signalShowFileHistory, this, &HistoryWidget::signalShowFileHistory);
   connect(mAmendWidget, &AmendWidget::signalUpdateWip, this, &HistoryWidget::signalUpdateWip);
   connect(mAmendWidget, &AmendWidget::signalCancelAmend, this, &HistoryWidget::onCommitSelected);

   connect(mCommitInfoWidget, &CommitInfoWidget::signalOpenFileCommit, this, &HistoryWidget::showFileDiff);
   connect(mCommitInfoWidget, &CommitInfoWidget::signalShowFileHistory, this, &HistoryWidget::signalShowFileHistory);

   mSearchInput = new QLineEdit();
   mSearchInput->setObjectName("SearchInput");
   mSearchInput->setPlaceholderText(tr("Press Enter to search by SHA or log message..."));
   connect(mSearchInput, &QLineEdit::returnPressed, this, &HistoryWidget::search);

   mRepositoryModel = new CommitHistoryModel(mCache, mGit, mGitServerCache);
   mRepositoryView = new CommitHistoryView(mCache, mGit, mSettings, mGitServerCache);

   connect(mRepositoryView, &CommitHistoryView::requestReload, this, &HistoryWidget::requestReload);
   connect(mRepositoryView, &CommitHistoryView::signalOpenDiff, this, [this](const QString &sha) {
      if (sha == CommitInfo::ZERO_SHA)
         showFullDiff();
      else
         emit signalOpenDiff(sha);
   });
   connect(mRepositoryView, &CommitHistoryView::signalOpenCompareDiff, this, &HistoryWidget::signalOpenCompareDiff);
   connect(mRepositoryView, &CommitHistoryView::clicked, this, &HistoryWidget::commitSelected);
   connect(mRepositoryView, &CommitHistoryView::customContextMenuRequested, this, [this](const QPoint &pos) {
      const auto rowIndex = mRepositoryView->indexAt(pos);
      commitSelected(rowIndex);
   });
   connect(mRepositoryView, &CommitHistoryView::signalAmendCommit, this, &HistoryWidget::onAmendCommit);
   connect(mRepositoryView, &CommitHistoryView::signalMergeRequired, this, &HistoryWidget::mergeBranch);
   connect(mRepositoryView, &CommitHistoryView::signalCherryPickConflict, this,
           &HistoryWidget::signalCherryPickConflict);
   connect(mRepositoryView, &CommitHistoryView::signalPullConflict, this, &HistoryWidget::signalPullConflict);
   connect(mRepositoryView, &CommitHistoryView::showPrDetailedView, this, &HistoryWidget::showPrDetailedView);

   mRepositoryView->setObjectName("historyGraphView");
   mRepositoryView->setModel(mRepositoryModel);
   mRepositoryView->setItemDelegate(mItemDelegate
                                    = new RepositoryViewDelegate(cache, mGit, mGitServerCache, mRepositoryView));
   mRepositoryView->setEnabled(true);

   mBranchesWidget = new BranchesWidget(mCache, mGit);

   connect(mBranchesWidget, &BranchesWidget::signalBranchesUpdated, this, &HistoryWidget::signalUpdateCache);
   connect(mBranchesWidget, &BranchesWidget::signalBranchCheckedOut, this, &HistoryWidget::onBranchCheckout);
   connect(mBranchesWidget, &BranchesWidget::signalSelectCommit, mRepositoryView, &CommitHistoryView::focusOnCommit);
   connect(mBranchesWidget, &BranchesWidget::signalSelectCommit, this, &HistoryWidget::goToSha);
   connect(mBranchesWidget, &BranchesWidget::signalOpenSubmodule, this, &HistoryWidget::signalOpenSubmodule);
   connect(mBranchesWidget, &BranchesWidget::signalMergeRequired, this, &HistoryWidget::mergeBranch);
   connect(mBranchesWidget, &BranchesWidget::signalPullConflict, this, &HistoryWidget::signalPullConflict);
   connect(mBranchesWidget, &BranchesWidget::panelsVisibilityChanged, this, &HistoryWidget::panelsVisibilityChanged);

   const auto cherryPickBtn = new QPushButton(tr("Cherry-pick"));
   cherryPickBtn->setEnabled(false);
   cherryPickBtn->setObjectName("applyActionBtn");
   connect(cherryPickBtn, &QPushButton::clicked, this, &HistoryWidget::cherryPickCommit);
   connect(mSearchInput, &QLineEdit::textChanged, this,
           [cherryPickBtn](const QString &text) { cherryPickBtn->setEnabled(!text.isEmpty()); });

   mChShowAllBranches = new CheckBox(tr("Show all branches"));
   mChShowAllBranches->setChecked(mSettings->localValue("ShowAllBranches", true).toBool());
   connect(mChShowAllBranches, &CheckBox::toggled, this, &HistoryWidget::onShowAllUpdated);

   const auto graphOptionsLayout = new QHBoxLayout();
   graphOptionsLayout->setContentsMargins(QMargins());
   graphOptionsLayout->setSpacing(10);
   graphOptionsLayout->addWidget(mSearchInput);
   graphOptionsLayout->addWidget(cherryPickBtn);
   graphOptionsLayout->addWidget(mChShowAllBranches);

   const auto viewLayout = new QVBoxLayout();
   viewLayout->setContentsMargins(QMargins());
   viewLayout->setSpacing(5);
   viewLayout->addLayout(graphOptionsLayout);
   viewLayout->addWidget(mRepositoryView);

   mGraphFrame = new QFrame();
   mGraphFrame->setLayout(viewLayout);

   mFileDiff = new FileDiffWidget(mGit, mCache);

   mReturnFromFull->setIcon(QIcon(":/icons/back"));
   connect(mReturnFromFull, &QPushButton::clicked, this, &HistoryWidget::returnToView);
   mFullDiffWidget = new FullDiffWidget(mGit, mCache);

   const auto fullFrame = new QFrame();
   const auto fullLayout = new QGridLayout(fullFrame);
   fullLayout->setSpacing(10);
   fullLayout->setContentsMargins(QMargins());
   fullLayout->addWidget(mReturnFromFull, 0, 0);
   fullLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 1);
   fullLayout->addWidget(mFullDiffWidget, 1, 0, 1, 2);

   mCenterStackedWidget = new QStackedWidget();
   mCenterStackedWidget->insertWidget(static_cast<int>(Pages::Graph), mGraphFrame);
   mCenterStackedWidget->insertWidget(static_cast<int>(Pages::FileDiff), mFileDiff);
   mCenterStackedWidget->insertWidget(static_cast<int>(Pages::FullDiff), fullFrame);

   connect(mFileDiff, &FileDiffWidget::exitRequested, this, &HistoryWidget::returnToView);
   connect(mFileDiff, &FileDiffWidget::fileStaged, this, &HistoryWidget::signalUpdateWip);
   connect(mFileDiff, &FileDiffWidget::fileReverted, this, &HistoryWidget::signalUpdateWip);

   connect(mWipWidget, &WipWidget::signalEditFile, mFileDiff, [this](const QString &fileName) {
      showFileDiffEdition(CommitInfo::ZERO_SHA, mCache->commitInfo(CommitInfo::ZERO_SHA).parent(0), fileName);
   });

   mSplitter->insertWidget(0, wipFrame);
   mSplitter->insertWidget(1, mCenterStackedWidget);
   mSplitter->setCollapsible(1, false);
   mSplitter->insertWidget(2, mBranchesWidget);

   const auto minimalActive = mBranchesWidget->isMinimalViewActive();
   const auto branchesWidth = minimalActive ? 50 : 250;

   rearrangeSplittrer(minimalActive);

   connect(mBranchesWidget, &BranchesWidget::minimalViewStateChanged, this, &HistoryWidget::rearrangeSplittrer);

   const auto splitterSate = mSettings->localValue("HistoryWidgetState", QByteArray()).toByteArray();

   if (splitterSate.isEmpty())
      mSplitter->setSizes({ 250, QApplication::primaryScreen()->geometry().width() - 500, branchesWidth });
   else
      mSplitter->restoreState(splitterSate);

   const auto layout = new QHBoxLayout(this);
   layout->addWidget(mSplitter);
   /*
   layout->setContentsMargins(QMargins());
   layout->setSpacing(10);
   layout->addWidget(wipFrame);
   layout->addWidget(mCenterStackedWidget);
   layout->addWidget(mBranchesWidget);
   */

   mCenterStackedWidget->setCurrentIndex(static_cast<int>(Pages::Graph));
   mCenterStackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
   mBranchesWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

HistoryWidget::~HistoryWidget()
{
   mSettings->setLocalValue("HistoryWidgetState", mSplitter->saveState());

   delete mItemDelegate;
   delete mRepositoryModel;
}

void HistoryWidget::clear()
{
   mRepositoryView->clear();
   resetWip();
   mBranchesWidget->clear();
   mCommitInfoWidget->clear();
   mAmendWidget->clear();

   mCommitStackedWidget->setCurrentIndex(mCommitStackedWidget->currentIndex());
}

void HistoryWidget::resetWip()
{
   mWipWidget->clear();
}

void HistoryWidget::loadBranches(bool fullReload)
{
   if (fullReload)
      mBranchesWidget->showBranches();
   else
      mBranchesWidget->refreshCurrentBranchLink();
}

void HistoryWidget::updateUiFromWatcher()
{
   const auto commitStackedIndex = mCommitStackedWidget->currentIndex();

   if (commitStackedIndex == 1)
      mWipWidget->configure(CommitInfo::ZERO_SHA);
   else if (commitStackedIndex == 2)
      mAmendWidget->reload();

   const auto viewIndex = mCenterStackedWidget->currentIndex();

   if (viewIndex == static_cast<int>(Pages::FileDiff))
      mFileDiff->reload();
   else if (viewIndex == static_cast<int>(Pages::FullDiff))
      mFullDiffWidget->reload();
}

void HistoryWidget::focusOnCommit(const QString &sha)
{
   mRepositoryView->focusOnCommit(sha);
}

QString HistoryWidget::getCurrentSha() const
{
   return mRepositoryView->getCurrentSha();
}

void HistoryWidget::onNewRevisions(int totalCommits)
{
   mRepositoryModel->onNewRevisions(totalCommits);

   onCommitSelected(CommitInfo::ZERO_SHA);

   mRepositoryView->selectionModel()->select(
       QItemSelection(mRepositoryModel->index(0, 0), mRepositoryModel->index(0, mRepositoryModel->columnCount() - 1)),
       QItemSelectionModel::Select);
}

void HistoryWidget::updateConfig()
{
   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
   const auto localUserInfo = gitConfig->getLocalUserInfo();
   const auto globalUserInfo = gitConfig->getGlobalUserInfo();

   mUserName->setText(localUserInfo.mUserName.isEmpty() ? globalUserInfo.mUserName : localUserInfo.mUserName);
   mUserEmail->setText(localUserInfo.mUserEmail.isEmpty() ? globalUserInfo.mUserEmail : localUserInfo.mUserEmail);
}

void HistoryWidget::keyPressEvent(QKeyEvent *event)
{
   if (event->key() == Qt::Key_Shift)
      mReverseSearch = true;

   QFrame::keyPressEvent(event);
}

void HistoryWidget::keyReleaseEvent(QKeyEvent *event)
{
   if (event->key() == Qt::Key_Shift)
      mReverseSearch = false;

   QFrame::keyReleaseEvent(event);
}

void HistoryWidget::showFullDiff()
{
   const auto commit = mCache->commitInfo(CommitInfo::ZERO_SHA);
   QScopedPointer<GitHistory> git(new GitHistory(mGit));
   const auto ret = git->getCommitDiff(CommitInfo::ZERO_SHA, commit.parent(0));

   if (ret.success && !ret.output.toString().isEmpty())
   {
      mFullDiffWidget->loadDiff(CommitInfo::ZERO_SHA, commit.parent(0), ret.output.toString());
      mCenterStackedWidget->setCurrentIndex(static_cast<int>(Pages::FullDiff));
   }
   else
      QMessageBox::warning(this, tr("No diff available!"), tr("There is no diff to show."));
}

void HistoryWidget::rearrangeSplittrer(bool minimalActive)
{
   if (minimalActive)
   {
      mBranchesWidget->setFixedWidth(50);
      mSplitter->setCollapsible(2, false);
   }
   else
   {
      mBranchesWidget->setMinimumWidth(250);
      mBranchesWidget->setMaximumWidth(500);
      mSplitter->setCollapsible(2, true);
   }
}

void HistoryWidget::onCommitTitleMaxLenghtChanged()
{
   mWipWidget->setCommitTitleMaxLength();
   mAmendWidget->setCommitTitleMaxLength();
}

void HistoryWidget::onPanelsVisibilityChanged()
{
   mBranchesWidget->onPanelsVisibilityChaned();
}

void HistoryWidget::search()
{
   if (const auto text = mSearchInput->text(); !text.isEmpty())
   {
      auto commitInfo = mCache->commitInfo(text);

      if (commitInfo.isValid())
         goToSha(text);
      else
      {
         auto selectedItems = mRepositoryView->selectedIndexes();
         auto startingRow = 0;

         if (!selectedItems.isEmpty())
         {
            std::sort(selectedItems.begin(), selectedItems.end(),
                      [](const QModelIndex index1, const QModelIndex index2) { return index1.row() <= index2.row(); });
            startingRow = selectedItems.constFirst().row();
         }

         commitInfo = mCache->searchCommitInfo(text, startingRow + 1, mReverseSearch);

         if (commitInfo.isValid())
            goToSha(commitInfo.sha());
         else
            QMessageBox::information(this, tr("Not found!"), tr("No commits where found based on the search text."));
      }
   }
}

void HistoryWidget::goToSha(const QString &sha)
{
   mRepositoryView->focusOnCommit(sha);

   onCommitSelected(sha);
}

void HistoryWidget::commitSelected(const QModelIndex &index)
{
   const auto sha = mRepositoryModel->sha(index.row());

   onCommitSelected(sha);
}

void HistoryWidget::onShowAllUpdated(bool showAll)
{
   GitQlientSettings settings(mGit->getGitDir());
   settings.setLocalValue("ShowAllBranches", showAll);

   emit signalAllBranchesActive(showAll);
}

void HistoryWidget::onBranchCheckout()
{
   QScopedPointer<GitBranches> gitBranches(new GitBranches(mGit));
   const auto ret = gitBranches->getLastCommitOfBranch(mGit->getCurrentBranch());

   if (mChShowAllBranches->isChecked())
      mRepositoryView->focusOnCommit(ret.output.toString());

   emit signalUpdateCache();
}

void HistoryWidget::mergeBranch(const QString &current, const QString &branchToMerge)
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QScopedPointer<GitMerge> git(new GitMerge(mGit, mCache));
   const auto ret = git->merge(current, { branchToMerge });

   QScopedPointer<GitWip> gitWip(new GitWip(mGit, mCache));
   gitWip->updateWip();

   QApplication::restoreOverrideCursor();

   if (!ret.success)
   {
      QMessageBox msgBox(
          QMessageBox::Critical, tr("Merge failed"),
          QString(tr("There were problems during the merge. Please, see the detailed description for more "
                     "information.<br><br>GitQlient will show the merge helper tool.")),
          QMessageBox::Ok, this);
      msgBox.setDetailedText(ret.output.toString());
      msgBox.setStyleSheet(GitQlientStyles::getStyles());
      msgBox.exec();

      emit signalMergeConflicts();
   }
   else
   {
      const auto outputStr = ret.output.toString();

      if (!outputStr.isEmpty())
      {
         if (outputStr.contains("error: could not apply", Qt::CaseInsensitive)
             || outputStr.contains(" conflict", Qt::CaseInsensitive))
         {
            QMessageBox msgBox(
                QMessageBox::Warning, tr("Merge status"),
                tr("There were problems during the merge. Please, see the detailed description for more information."),
                QMessageBox::Ok, this);
            msgBox.setDetailedText(ret.output.toString());
            msgBox.setStyleSheet(GitQlientStyles::getStyles());
            msgBox.exec();

            emit signalMergeConflicts();
         }
         else
         {
            emit signalUpdateCache();

            QMessageBox msgBox(
                QMessageBox::Information, tr("Merge successful"),
                tr("The merge was successfully done. See the detailed description for more information."),
                QMessageBox::Ok, this);
            msgBox.setDetailedText(ret.output.toString());
            msgBox.setStyleSheet(GitQlientStyles::getStyles());
            msgBox.exec();
         }
      }
   }
}

void HistoryWidget::onCommitSelected(const QString &goToSha)
{
   const auto isWip = goToSha == CommitInfo::ZERO_SHA;
   mCommitStackedWidget->setCurrentIndex(isWip);

   QLog_Info("UI", QString("Selected commit {%1}").arg(goToSha));

   if (isWip)
      mWipWidget->configure(CommitInfo::ZERO_SHA);
   else
      mCommitInfoWidget->configure(goToSha);
}

void HistoryWidget::onAmendCommit(const QString &sha)
{
   mCommitStackedWidget->setCurrentIndex(2);
   mAmendWidget->configure(sha);
}

void HistoryWidget::returnToView()
{
   mCenterStackedWidget->setCurrentIndex(static_cast<int>(Pages::Graph));
   mBranchesWidget->returnToSavedView();
}

void HistoryWidget::cherryPickCommit()
{
   if (const auto commit = mCache->commitInfo(mSearchInput->text()); commit.isValid())
   {
      const auto git = QScopedPointer<GitLocal>(new GitLocal(mGit));
      const auto ret = git->cherryPickCommit(commit.sha());

      if (ret.success)
      {
         mSearchInput->clear();
         emit requestReload(false);
      }
      else
      {
         const auto errorMsg = ret.output.toString();

         if (errorMsg.contains("error: could not apply", Qt::CaseInsensitive)
             || errorMsg.contains(" conflict", Qt::CaseInsensitive))
         {
            emit signalCherryPickConflict(QStringList());
         }
         else
         {
            QMessageBox msgBox(QMessageBox::Critical, tr("Error while cherry-pick"),
                               tr("There were problems during the cherry-pick operation. Please, see the detailed "
                                  "description for more information."),
                               QMessageBox::Ok, this);
            msgBox.setDetailedText(errorMsg);
            msgBox.setStyleSheet(GitQlientStyles::getStyles());
            msgBox.exec();
         }
      }
   }
}

void HistoryWidget::showFileDiff(const QString &sha, const QString &parentSha, const QString &fileName, bool isCached)
{
   if (sha == CommitInfo::ZERO_SHA)
   {
      mFileDiff->configure(sha, parentSha, fileName, isCached);
      mCenterStackedWidget->setCurrentIndex(static_cast<int>(Pages::FileDiff));
      mBranchesWidget->forceMinimalView();
   }
   else
      emit signalShowDiff(sha, parentSha, fileName, isCached);
}

void HistoryWidget::showFileDiffEdition(const QString &sha, const QString &parentSha, const QString &fileName)
{
   if (sha == CommitInfo::ZERO_SHA)
   {
      mFileDiff->configure(sha, parentSha, fileName, true);
      mCenterStackedWidget->setCurrentIndex(static_cast<int>(Pages::FileDiff));
      mBranchesWidget->forceMinimalView();
   }
   else
      emit signalShowDiff(sha, parentSha, fileName, false);
}
