#include "HistoryWidget.h"

#include <CommitHistoryModel.h>
#include <CommitHistoryView.h>
#include <RepositoryViewDelegate.h>
#include <BranchesWidget.h>
#include <WorkInProgressWidget.h>
#include <CommitInfoWidget.h>
#include "git.h"

#include <QLogger.h>

#include <QGridLayout>
#include <QLineEdit>
#include <QStackedWidget>

using namespace QLogger;

HistoryWidget::HistoryWidget(const QSharedPointer<Git> git, QWidget *parent)
   : QFrame(parent)
   , mRepositoryModel(new CommitHistoryModel(git))
   , mRepositoryView(new CommitHistoryView(git))
   , mBranchesWidget(new BranchesWidget(git))
   , mGoToSha(new QLineEdit())
   , mCommitStackedWidget(new QStackedWidget())
   , mCommitWidget(new WorkInProgressWidget(git))
   , mRevisionWidget(new CommitInfoWidget(git))
{
   mCommitStackedWidget->setCurrentIndex(0);
   mCommitStackedWidget->addWidget(mRevisionWidget);
   mCommitStackedWidget->addWidget(mCommitWidget);
   mCommitStackedWidget->setFixedWidth(310);

   connect(mCommitWidget, &WorkInProgressWidget::signalShowDiff, this, &HistoryWidget::signalShowDiff);
   connect(mCommitWidget, &WorkInProgressWidget::signalChangesCommitted, this, &HistoryWidget::signalChangesCommitted);
   connect(mCommitWidget, &WorkInProgressWidget::signalCheckoutPerformed, this, &HistoryWidget::signalUpdateUi);
   connect(mCommitWidget, &WorkInProgressWidget::signalShowFileHistory, this, &HistoryWidget::signalShowFileHistory);
   connect(mRevisionWidget, &CommitInfoWidget::signalOpenFileCommit, this, &HistoryWidget::signalOpenFileCommit);
   connect(mRevisionWidget, &CommitInfoWidget::signalShowFileHistory, this, &HistoryWidget::signalShowFileHistory);

   mGoToSha->setPlaceholderText(tr("Press Enter to focus on SHA..."));
   connect(mGoToSha, &QLineEdit::returnPressed, this, &HistoryWidget::goToSha);

   mRepositoryView->setModel(mRepositoryModel);
   mRepositoryView->setItemDelegate(new RepositoryViewDelegate(git, mRepositoryView));
   mRepositoryView->setEnabled(true);

   connect(mRepositoryView, &CommitHistoryView::signalViewUpdated, this, &HistoryWidget::signalViewUpdated);
   connect(mRepositoryView, &CommitHistoryView::signalOpenDiff, this, &HistoryWidget::signalOpenDiff);
   connect(mRepositoryView, &CommitHistoryView::signalOpenCompareDiff, this, &HistoryWidget::signalOpenCompareDiff);
   connect(mRepositoryView, &CommitHistoryView::clicked, this, &HistoryWidget::commitSelected);
   connect(mRepositoryView, &CommitHistoryView::doubleClicked, this, &HistoryWidget::openDiff);
   connect(mRepositoryView, &CommitHistoryView::signalAmendCommit, this, &HistoryWidget::onAmendCommit);

   connect(mBranchesWidget, &BranchesWidget::signalBranchesUpdated, this, &HistoryWidget::signalUpdateCache);
   connect(mBranchesWidget, &BranchesWidget::signalBranchCheckedOut, this, &HistoryWidget::signalUpdateCache);
   connect(mBranchesWidget, &BranchesWidget::signalSelectCommit, mRepositoryView, &CommitHistoryView::focusOnCommit);
   connect(mBranchesWidget, &BranchesWidget::signalSelectCommit, this, &HistoryWidget::signalGoToSha);
   connect(mBranchesWidget, &BranchesWidget::signalOpenSubmodule, this, &HistoryWidget::signalOpenSubmodule);

   const auto viewLayout = new QVBoxLayout();
   viewLayout->setContentsMargins(QMargins());
   viewLayout->setSpacing(5);
   viewLayout->addWidget(mGoToSha);
   viewLayout->addWidget(mRepositoryView);

   const auto layout = new QHBoxLayout();
   layout->setContentsMargins(QMargins());
   layout->setSpacing(15);
   layout->addWidget(mCommitStackedWidget);
   layout->addLayout(viewLayout);
   layout->addWidget(mBranchesWidget);

   setLayout(layout);
}

void HistoryWidget::clear()
{
   mRepositoryView->clear();
   mBranchesWidget->clear();
   mCommitWidget->clear();
   mRevisionWidget->clear();

   mCommitStackedWidget->setCurrentIndex(mCommitStackedWidget->currentIndex());
}

void HistoryWidget::reload()
{
   mBranchesWidget->showBranches();

   const auto commitStackedIndex = mCommitStackedWidget->currentIndex();
   const auto currentSha = commitStackedIndex == 0 ? mRevisionWidget->getCurrentCommitSha() : ZERO_SHA;

   focusOnCommit(currentSha);

   if (commitStackedIndex == 1)
      mCommitWidget->configure(currentSha);
}

void HistoryWidget::updateUiFromWatcher()
{
   const auto commitStackedIndex = mCommitStackedWidget->currentIndex();

   if (commitStackedIndex == 1 && !mCommitWidget->isAmendActive())
      mCommitWidget->configure(ZERO_SHA);
}

void HistoryWidget::focusOnCommit(const QString &sha)
{
   mRepositoryView->focusOnCommit(sha);
}

QString HistoryWidget::getCurrentSha() const
{
   return mRepositoryView->getCurrentSha();
}

void HistoryWidget::goToSha()
{
   const auto sha = mGoToSha->text();
   mRepositoryView->focusOnCommit(sha);

   mGoToSha->clear();

   onCommitSelected(sha);
}

void HistoryWidget::commitSelected(const QModelIndex &index)
{
   const auto sha = mRepositoryModel->sha(index.row());

   onCommitSelected(sha);
}

void HistoryWidget::openDiff(const QModelIndex &index)
{
   const auto sha = mRepositoryModel->sha(index.row());

   emit signalOpenDiff(sha);
}

void HistoryWidget::onCommitSelected(const QString &goToSha)
{
   const auto isWip = goToSha == ZERO_SHA;
   mCommitStackedWidget->setCurrentIndex(isWip);

   QLog_Info("UI", QString("Selected commit {%1}").arg(goToSha));

   if (isWip)
      mCommitWidget->configure(goToSha);
   else
      mRevisionWidget->configure(goToSha);
}

void HistoryWidget::onAmendCommit(const QString &sha)
{
   mCommitStackedWidget->setCurrentIndex(1);
   mCommitWidget->configure(sha);
}
