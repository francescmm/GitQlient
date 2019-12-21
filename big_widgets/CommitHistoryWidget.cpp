#include "CommitHistoryWidget.h"

#include <CommitHistoryModel.h>
#include <CommitHistoryView.h>
#include <RepositoryViewDelegate.h>

#include <QVBoxLayout>
#include <QLineEdit>

CommitHistoryWidget::CommitHistoryWidget(const QSharedPointer<Git> git, QWidget *parent)
   : QFrame(parent)
   , mRepositoryModel(new CommitHistoryModel(git))
   , mRepositoryView(new CommitHistoryView(git))
{
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

   mGoToSha = new QLineEdit();
   mGoToSha->setPlaceholderText(tr("Press Enter to focus on SHA..."));
   connect(mGoToSha, &QLineEdit::returnPressed, this, &CommitHistoryWidget::goToSha);

   const auto layout = new QVBoxLayout();
   layout->setContentsMargins(QMargins());
   layout->addWidget(mGoToSha);
   layout->addWidget(mRepositoryView);

   setLayout(layout);
}

void CommitHistoryWidget::clear()
{
   mRepositoryView->clear();
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
