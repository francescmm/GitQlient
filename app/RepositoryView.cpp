/*
        Description: qgit revision list view

Author: Marco Costalba (C) 2005-2007

             Copyright: See COPYING file that comes with this distribution

                             */
#include "RepositoryView.h"

#include <RevisionsCache.h>
#include <Revision.h>
#include <CommitHistoryModel.h>
#include <CommitHistoryColumns.h>
#include <git.h>
#include <ShaFilterProxyModel.h>
#include <RepositoryContextMenu.h>
#include <RepositoryViewDelegate.h>
#include <QLogger.h>

#include <QApplication>
#include <QClipboard>
#include <QDrag>
#include <QHeaderView>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QSettings>
#include <QShortcut>
#include <QUrl>
#include <QMenu>
#include <QByteArray>
#include <QDateTime>

using namespace QLogger;

RepositoryView::RepositoryView(QSharedPointer<RevisionsCache> revCache, QSharedPointer<Git> git, QWidget *parent)
   : QTreeView(parent)
   , mRevCache(revCache)
   , mGit(git)
   , mCommitHistoryModel(new CommitHistoryModel(mRevCache, mGit))
{
   setEnabled(false);
   setContextMenuPolicy(Qt::CustomContextMenu);
   setItemsExpandable(false);
   setMouseTracking(true);
   setSelectionMode(QAbstractItemView::ExtendedSelection);
   header()->setSortIndicatorShown(false);

   connect(header(), &QHeaderView::sectionResized, this, &RepositoryView::saveHeaderState);

   const auto lvd = new RepositoryViewDelegate(mGit, revCache, this);
   setItemDelegate(lvd);

   connect(lvd, &RepositoryViewDelegate::updateView, viewport(), qOverload<>(&QWidget::update));
   connect(this, &RepositoryView::diffTargetChanged, lvd, &RepositoryViewDelegate::diffTargetChanged);
   connect(this, &RepositoryView::customContextMenuRequested, this, &RepositoryView::showContextMenu);
   connect(mRevCache.get(), &RevisionsCache::signalCacheUpdated, this, &RepositoryView::update);

   setModel(mCommitHistoryModel);
   setupGeometry();
}

void RepositoryView::filterBySha(const QStringList &shaList)
{
   mIsFiltering = true;

   delete mProxyModel;
   setModel(mCommitHistoryModel);

   mProxyModel = new ShaFilterProxyModel(this);
   mProxyModel->setAcceptedSha(shaList);
   mProxyModel->setSourceModel(mCommitHistoryModel);
   setModel(mProxyModel);

   setupGeometry();
}

RepositoryView::~RepositoryView()
{
   QSettings s;
   s.setValue(QString("RepositoryView::%1").arg(objectName()), header()->saveState());
}

void RepositoryView::setupGeometry()
{
   QSettings s;
   const auto previousState = s.value(QString("RepositoryView::%1").arg(objectName()), QByteArray()).toByteArray();

   if (previousState.isEmpty())
   {
      const auto hv = header();
      hv->setCascadingSectionResizes(true);
      hv->resizeSection(static_cast<int>(CommitHistoryColumns::GRAPH), 120);
      hv->setSectionResizeMode(static_cast<int>(CommitHistoryColumns::LOG), QHeaderView::Stretch);
      hv->setStretchLastSection(false);

      hideColumn(static_cast<int>(CommitHistoryColumns::SHA));
      hideColumn(static_cast<int>(CommitHistoryColumns::DATE));
      hideColumn(static_cast<int>(CommitHistoryColumns::AUTHOR));
      hideColumn(static_cast<int>(CommitHistoryColumns::ID));
   }
   else
   {
      header()->restoreState(previousState);
      header()->setSectionResizeMode(static_cast<int>(CommitHistoryColumns::LOG), QHeaderView::Stretch);
   }
}

bool RepositoryView::update()
{
   auto stRow = mRevCache ? mRevCache->row(mCurrentSha) : -1;

   if (mIsFiltering)
      stRow = mProxyModel->mapFromSource(mCommitHistoryModel->index(stRow, 0)).row();

   if (stRow == -1)
      return false;

   QModelIndex index = currentIndex();
   QItemSelectionModel *sel = selectionModel();

   if (index.isValid() && (index.row() == stRow))
   {

      sel->select(index, QItemSelectionModel::Toggle);
      scrollTo(index);
   }
   else
   {
      // setCurrentIndex() does not clear previous
      // selections in a multi selection QListView
      clearSelection();

      QModelIndex newIndex = model()->index(stRow, 0);
      if (newIndex.isValid())
      {
         setCurrentIndex(newIndex);
         scrollTo(newIndex);
      }
   }

   emit diffTargetChanged(mRevCache ? mRevCache->row(mCurrentSha) : -1);

   setupGeometry();

   return currentIndex().isValid();
}

void RepositoryView::currentChanged(const QModelIndex &index, const QModelIndex &)
{
   mCurrentSha = mRevCache->revLookup(index.row()).sha();

   emit clicked(index);
}

void RepositoryView::clear()
{
   mCommitHistoryModel->clear();
}

void RepositoryView::focusOnCommit(const QString &goToSha)
{
   mCurrentSha = mGit->getRefSha(goToSha);

   QLog_Info("UI", QString("Setting the focus on the commit {%1}").arg(mCurrentSha));

   update();
}

void RepositoryView::showContextMenu(const QPoint &pos)
{
   const auto shas = getSelectedShaList();

   if (!shas.isEmpty())
   {
      const auto menu = new RepositoryContextMenu(mGit, shas, this);
      connect(menu, &RepositoryContextMenu::signalRepositoryUpdated, this, &RepositoryView::signalViewUpdated);
      connect(menu, &RepositoryContextMenu::signalOpenDiff, this, &RepositoryView::signalOpenDiff);
      connect(menu, &RepositoryContextMenu::signalOpenCompareDiff, this, &RepositoryView::signalOpenCompareDiff);
      connect(menu, &RepositoryContextMenu::signalAmendCommit, this, &RepositoryView::signalAmendCommit);
      menu->exec(viewport()->mapToGlobal(pos));
   }
   else
      QLog_Warning("UI", "SHAs selected belong to different branches. They need to share at least one branch.");
}

void RepositoryView::saveHeaderState()
{
   QSettings s;
   s.setValue(QString("RepositoryView::%1").arg(objectName()), header()->saveState());
}

QList<QString> RepositoryView::getSelectedShaList() const
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
