#include "CommitHistoryWidget.h"

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

CommitHistoryWidget::CommitHistoryWidget(const QSharedPointer<Git> git, QWidget *parent)
   : QFrame(parent)
   , mRepositoryModel(new CommitHistoryModel(git))
   , mRepositoryView(new CommitHistoryView(git))
   , mBranchesWidget(new BranchesWidget(git))
   , mGoToSha(new QLineEdit())
   , commitStackedWidget(new QStackedWidget())
   , mCommitWidget(new WorkInProgressWidget(git))
   , mRevisionWidget(new CommitInfoWidget(git))
{
   commitStackedWidget->setCurrentIndex(0);
   commitStackedWidget->addWidget(mRevisionWidget);
   commitStackedWidget->addWidget(mCommitWidget);
   commitStackedWidget->setFixedWidth(310);

   connect(mCommitWidget, &WorkInProgressWidget::signalShowDiff, this, &CommitHistoryWidget::signalShowDiff);
   connect(mCommitWidget, &WorkInProgressWidget::signalChangesCommitted, this,
           &CommitHistoryWidget::signalChangesCommitted);
   connect(mCommitWidget, &WorkInProgressWidget::signalCheckoutPerformed, this, &CommitHistoryWidget::signalUpdateUi);
   connect(mCommitWidget, &WorkInProgressWidget::signalShowFileHistory, this,
           &CommitHistoryWidget::signalShowFileHistory);
   connect(mRevisionWidget, &CommitInfoWidget::signalOpenFileCommit, this, &CommitHistoryWidget::signalOpenFileCommit);
   connect(mRevisionWidget, &CommitInfoWidget::signalShowFileHistory, this,
           &CommitHistoryWidget::signalShowFileHistory);

   mGoToSha->setPlaceholderText(tr("Press Enter to focus on SHA..."));
   connect(mGoToSha, &QLineEdit::returnPressed, this, &CommitHistoryWidget::goToSha);

   mRepositoryView->setModel(mRepositoryModel);
   mRepositoryView->setItemDelegate(new RepositoryViewDelegate(git, mRepositoryView));
   mRepositoryView->setEnabled(true);

   connect(mRepositoryView, &CommitHistoryView::signalViewUpdated, this, &CommitHistoryWidget::signalViewUpdated);
   connect(mRepositoryView, &CommitHistoryView::signalOpenDiff, this, &CommitHistoryWidget::signalOpenDiff);
   connect(mRepositoryView, &CommitHistoryView::signalOpenCompareDiff, this,
           &CommitHistoryWidget::signalOpenCompareDiff);
   connect(mRepositoryView, &CommitHistoryView::clicked, this, &CommitHistoryWidget::commitSelected);
   connect(mRepositoryView, &CommitHistoryView::doubleClicked, this, &CommitHistoryWidget::openDiff);
   connect(mRepositoryView, &CommitHistoryView::signalAmendCommit, this, &CommitHistoryWidget::onAmendCommit);

   connect(mBranchesWidget, &BranchesWidget::signalBranchesUpdated, this, &CommitHistoryWidget::signalUpdateCache);
   connect(mBranchesWidget, &BranchesWidget::signalBranchCheckedOut, this, &CommitHistoryWidget::signalUpdateCache);
   connect(mBranchesWidget, &BranchesWidget::signalSelectCommit, mRepositoryView, &CommitHistoryView::focusOnCommit);
   connect(mBranchesWidget, &BranchesWidget::signalSelectCommit, this, &CommitHistoryWidget::signalGoToSha);
   connect(mBranchesWidget, &BranchesWidget::signalOpenSubmodule, this, &CommitHistoryWidget::signalOpenSubmodule);

   const auto viewLayout = new QVBoxLayout();
   viewLayout->setContentsMargins(QMargins());
   viewLayout->setSpacing(5);
   viewLayout->addWidget(mGoToSha);
   viewLayout->addWidget(mRepositoryView);

   const auto layout = new QHBoxLayout();
   layout->setContentsMargins(QMargins());
   layout->setSpacing(15);
   layout->addWidget(commitStackedWidget);
   layout->addLayout(viewLayout);
   layout->addWidget(mBranchesWidget);

   setLayout(layout);
}

void CommitHistoryWidget::clear()
{
   mRepositoryView->clear();
   mBranchesWidget->clear();
   mCommitWidget->clear();
   mRevisionWidget->clear();

   commitStackedWidget->setCurrentIndex(commitStackedWidget->currentIndex());
}

void CommitHistoryWidget::reload()
{
   mBranchesWidget->showBranches();

   const auto commitStackedIndex = commitStackedWidget->currentIndex();
   const auto currentSha = commitStackedIndex == 0 ? mRevisionWidget->getCurrentCommitSha() : ZERO_SHA;

   focusOnCommit(currentSha);

   if (commitStackedIndex == 1)
      mCommitWidget->configure(currentSha);
}

void CommitHistoryWidget::updateUiFromWatcher()
{
   const auto commitStackedIndex = commitStackedWidget->currentIndex();

   if (commitStackedIndex == 1 && !mCommitWidget->isAmendActive())
      mCommitWidget->configure(ZERO_SHA);
}

void CommitHistoryWidget::focusOnCommit(const QString &sha)
{
   mRepositoryView->focusOnCommit(sha);
}

QString CommitHistoryWidget::getCurrentSha() const
{
   return mRepositoryView->getCurrentSha();
}

void CommitHistoryWidget::goToSha()
{
   const auto sha = mGoToSha->text();
   mRepositoryView->focusOnCommit(sha);

   mGoToSha->clear();

   onCommitSelected(sha);
}

void CommitHistoryWidget::commitSelected(const QModelIndex &index)
{
   const auto sha = mRepositoryModel->sha(index.row());

   onCommitSelected(sha);
}

void CommitHistoryWidget::openDiff(const QModelIndex &index)
{
   const auto sha = mRepositoryModel->sha(index.row());

   emit signalOpenDiff(sha);
}

void CommitHistoryWidget::onCommitSelected(const QString &goToSha)
{
   const auto isWip = goToSha == ZERO_SHA;
   commitStackedWidget->setCurrentIndex(isWip);

   QLog_Info("UI", QString("Selected commit {%1}").arg(goToSha));

   if (isWip)
      mCommitWidget->configure(goToSha);
   else
      mRevisionWidget->setCurrentCommitSha(goToSha);
}

void CommitHistoryWidget::onAmendCommit(const QString &sha)
{
   commitStackedWidget->setCurrentIndex(1);
   mCommitWidget->configure(sha);
}
