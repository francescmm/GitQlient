/*
        Description: qgit revision list view

Author: Marco Costalba (C) 2005-2007

             Copyright: See COPYING file that comes with this distribution

                             */
#include "CommitHistoryView.h"

#include <CommitHistoryModel.h>
#include <CommitHistoryColumns.h>
#include <CommitHistoryContextMenu.h>
#include <RepositoryViewDelegate.h>
#include <ShaFilterProxyModel.h>
#include <git.h>
#include <CommitInfo.h>

#include <QHeaderView>
#include <QSettings>
#include <QDateTime>

#include <QLogger.h>
using namespace QLogger;

CommitHistoryView::CommitHistoryView(QSharedPointer<Git> git, QWidget *parent)
   : QTreeView(parent)
   , mGit(git)
{
   setEnabled(false);
   setContextMenuPolicy(Qt::CustomContextMenu);
   setItemsExpandable(false);
   setMouseTracking(true);
   setSelectionMode(QAbstractItemView::ExtendedSelection);
   header()->setSortIndicatorShown(false);

   connect(header(), &QHeaderView::sectionResized, this, &CommitHistoryView::saveHeaderState);
   connect(this, &CommitHistoryView::customContextMenuRequested, this, &CommitHistoryView::showContextMenu);
}

void CommitHistoryView::setModel(QAbstractItemModel *model)
{
   mCommitHistoryModel = dynamic_cast<CommitHistoryModel *>(model);
   QTreeView::setModel(model);
   setupGeometry();
}

void CommitHistoryView::filterBySha(const QStringList &shaList)
{
   mIsFiltering = true;

   if (mProxyModel)
   {
      mProxyModel->beginResetModel();
      mProxyModel->setAcceptedSha(shaList);
      mProxyModel->endResetModel();
   }
   else
   {
      mProxyModel = new ShaFilterProxyModel(this);
      mProxyModel->setSourceModel(mCommitHistoryModel);
      mProxyModel->setAcceptedSha(shaList);
      setModel(mProxyModel);
   }

   setupGeometry();
}

CommitHistoryView::~CommitHistoryView()
{
   QSettings s;
   s.setValue(QString("RepositoryView::%1").arg(objectName()), header()->saveState());
}

void CommitHistoryView::setupGeometry()
{
   QSettings s;
   const auto previousState = s.value(QString("RepositoryView::%1").arg(objectName()), QByteArray()).toByteArray();

   if (previousState.isEmpty())
   {
      const auto hv = header();
      hv->setCascadingSectionResizes(true);
      hv->resizeSection(static_cast<int>(CommitHistoryColumns::GRAPH), 120);
      hv->resizeSection(static_cast<int>(CommitHistoryColumns::AUTHOR), 140);
      hv->resizeSection(static_cast<int>(CommitHistoryColumns::DATE), 120);
      hv->resizeSection(static_cast<int>(CommitHistoryColumns::SHA), 100);
      hv->setSectionResizeMode(static_cast<int>(CommitHistoryColumns::LOG), QHeaderView::Stretch);
      hv->setStretchLastSection(false);

      // hideColumn(static_cast<int>(CommitHistoryColumns::SHA));
      // hideColumn(static_cast<int>(CommitHistoryColumns::DATE));
      // hideColumn(static_cast<int>(CommitHistoryColumns::AUTHOR));
      hideColumn(static_cast<int>(CommitHistoryColumns::ID));
   }
   else
   {
      header()->restoreState(previousState);
      header()->setSectionResizeMode(static_cast<int>(CommitHistoryColumns::LOG), QHeaderView::Stretch);
   }
}

void CommitHistoryView::currentChanged(const QModelIndex &index, const QModelIndex &)
{
   mCurrentSha = model()->index(index.row(), static_cast<int>(CommitHistoryColumns::SHA)).data().toString();

   emit clicked(index);
}

void CommitHistoryView::clear()
{
   mCommitHistoryModel->clear();
}

void CommitHistoryView::focusOnCommit(const QString &goToSha)
{
   mCurrentSha = mGit->getRefSha(goToSha);

   QLog_Info("UI", QString("Setting the focus on the commit {%1}").arg(mCurrentSha));

   QModelIndex index;
   auto row = mGit->getCommitInfo(mCurrentSha).orderIdx;

   if (mIsFiltering)
   {
      const auto sourceIndex = mProxyModel->sourceModel()->index(row, 0);
      row = mProxyModel->mapFromSource(sourceIndex).row();
   }
   else
      index = mCommitHistoryModel->index(row, 0);

   clearSelection();

   QModelIndex newIndex = model()->index(row, 0);

   if (newIndex.isValid())
   {
      setCurrentIndex(newIndex);
      scrollTo(newIndex);
   }
}

void CommitHistoryView::showContextMenu(const QPoint &pos)
{
   if (!mIsFiltering)
   {
      const auto shas = getSelectedShaList();

      if (!shas.isEmpty())
      {
         const auto menu = new CommitHistoryContextMenu(mGit, shas, this);
         connect(menu, &CommitHistoryContextMenu::signalRepositoryUpdated, this, &CommitHistoryView::signalViewUpdated);
         connect(menu, &CommitHistoryContextMenu::signalOpenDiff, this, &CommitHistoryView::signalOpenDiff);
         connect(menu, &CommitHistoryContextMenu::signalOpenCompareDiff, this,
                 &CommitHistoryView::signalOpenCompareDiff);
         connect(menu, &CommitHistoryContextMenu::signalAmendCommit, this, &CommitHistoryView::signalAmendCommit);
         menu->exec(viewport()->mapToGlobal(pos));
      }
      else
         QLog_Warning("UI", "SHAs selected belong to different branches. They need to share at least one branch.");
   }
}

void CommitHistoryView::saveHeaderState()
{
   QSettings s;
   s.setValue(QString("RepositoryView::%1").arg(objectName()), header()->saveState());
}

QList<QString> CommitHistoryView::getSelectedShaList() const
{
   const auto indexes = selectedIndexes();
   QMap<QDateTime, QString> shas;
   QVector<QVector<QString>> godVector;

   for (auto index : indexes)
   {
      const auto sha = mCommitHistoryModel->sha(index.row());
      const auto dtStr
          = mCommitHistoryModel->index(index.row(), static_cast<int>(CommitHistoryColumns::DATE)).data().toString();
      const auto dt = QDateTime::fromString(dtStr, "dd/MM/yyyy hh:mm");

      shas.insert(dt, sha);
      auto ret = mGit->getBranchesOfCommit(sha);
      auto branches = ret.output.toString().trimmed().split("\n ");
      std::sort(branches.begin(), branches.end());
      godVector.append(branches.toVector());
   }

   auto commitsInSameBranch = false;

   if (godVector.count() > 1)
   {
      QVector<QString> common;

      for (auto i = 0; i < godVector.count() - 1; ++i)
      {
         QVector<QString> aux;
         std::set_intersection(godVector.at(i).constBegin(), godVector.at(i).constEnd(),
                               godVector.at(i + 1).constBegin(), godVector.at(i + 1).constEnd(),
                               std::back_inserter(aux));
         common = aux;
      }

      commitsInSameBranch = !common.isEmpty();
   }

   return commitsInSameBranch || shas.count() == 1 ? shas.values() : QList<QString>();
}
