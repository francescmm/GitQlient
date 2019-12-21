#include "CommitHistoryWidget.h"

#include <CommitHistoryModel.h>
#include <CommitHistoryView.h>
#include <RepositoryViewDelegate.h>
#include <BranchesWidget.h>

#include <QGridLayout>
#include <QLineEdit>

CommitHistoryWidget::CommitHistoryWidget(const QSharedPointer<Git> git, QWidget *parent)
   : QFrame(parent)
   , mRepositoryModel(new CommitHistoryModel(git))
   , mRepositoryView(new CommitHistoryView(git))
   , mBranchesWidget(new BranchesWidget(git))
   , mGoToSha(new QLineEdit())
{
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
   connect(mRepositoryView, &CommitHistoryView::signalAmendCommit, this, &CommitHistoryWidget::signalAmendCommit);

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
   layout->addLayout(viewLayout);
   layout->addWidget(mBranchesWidget);

   setLayout(layout);
}

void CommitHistoryWidget::clear()
{
   mRepositoryView->clear();
   mBranchesWidget->clear();
}

void CommitHistoryWidget::reload()
{
   mBranchesWidget->showBranches();
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

   emit signalGoToSha(sha);
}

void CommitHistoryWidget::commitSelected(const QModelIndex &index)
{
   const auto sha = mRepositoryModel->sha(index.row());

   emit signalGoToSha(sha);
}

void CommitHistoryWidget::openDiff(const QModelIndex &index)
{
   const auto sha = mRepositoryModel->sha(index.row());

   emit signalOpenDiff(sha);
}
