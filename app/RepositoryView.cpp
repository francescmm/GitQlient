/*
        Description: qgit revision list view

Author: Marco Costalba (C) 2005-2007

             Copyright: See COPYING file that comes with this distribution

                             */
#include "RepositoryView.h"

#include <RevisionsCache.h>
#include <Revision.h>
#include <RepositoryModel.h>
#include <RepositoryModelColumns.h>
#include <domain.h>
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
   , mRepositoryModel(new RepositoryModel(mRevCache, mGit))
   , d(new Domain(mRevCache))
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
   connect(mGit.get(), &Git::newRevsAdded, this, [this]() {
      if (!d->st.sha().isEmpty() || mRepositoryModel->rowCount() == 0)
         return;

      d->st.setSha(mRepositoryModel->index(0, static_cast<int>(RepositoryModelColumns::SHA)).data().toString());
      d->st.setSelectItem(true);
      update();
   });
   connect(mGit.get(), &Git::loadCompleted, this, [this]() { d->update(false); });
}

void RepositoryView::setup()
{
   st = &(d->st);
   setModel(mRepositoryModel);

   setupGeometry(); // after setting delegate
}

void RepositoryView::filterBySha(const QStringList &shaList)
{
   mIsFiltering = true;

   delete mProxyModel;
   setModel(mRepositoryModel);

   mProxyModel = new ShaFilterProxyModel(this);
   mProxyModel->setAcceptedSha(shaList);
   mProxyModel->setSourceModel(mRepositoryModel);
   setModel(mProxyModel);
}

RepositoryView::~RepositoryView()
{
   mGit->cancelDataLoading(); // non blocking

   QSettings s;
   s.setValue(QString("RepositoryView::%1").arg(objectName()), header()->saveState());
}

void RepositoryView::setupGeometry()
{
   QSettings s;
   const auto previousState = s.value(QString("RepositoryView::%1").arg(objectName()), QByteArray()).toByteArray();

   if (previousState.isEmpty())
   {
      QHeaderView *hv = header();
      hv->setCascadingSectionResizes(true);
      hv->resizeSection(static_cast<int>(RepositoryModelColumns::GRAPH), 80);
      hv->resizeSection(static_cast<int>(RepositoryModelColumns::SHA), 60);
      hv->resizeSection(static_cast<int>(RepositoryModelColumns::AUTHOR), 200);
      hv->resizeSection(static_cast<int>(RepositoryModelColumns::DATE), 115);
      hv->setSectionResizeMode(static_cast<int>(RepositoryModelColumns::LOG), QHeaderView::Stretch);
      hv->setStretchLastSection(false);

      hideColumn(static_cast<int>(RepositoryModelColumns::SHA));
      hideColumn(static_cast<int>(RepositoryModelColumns::DATE));
      hideColumn(static_cast<int>(RepositoryModelColumns::AUTHOR));
      hideColumn(static_cast<int>(RepositoryModelColumns::ID));
   }
   else
   {
      header()->restoreState(previousState);
   }
}

int RepositoryView::getLaneType(const QString &sha, int pos) const
{

   const auto r = mRevCache->getRevLookup(sha);
   return pos < r.lanes.count() && pos >= 0 ? static_cast<int>(r.lanes.at(pos)) : -1;
}

bool RepositoryView::update()
{
   if (!st)
      return false;

   auto stRow = mRevCache ? mRevCache->row(st->sha()) : -1;

   if (mIsFiltering)
      stRow = mProxyModel->mapFromSource(mRepositoryModel->index(stRow, 0)).row();

   if (stRow == -1)
      return false; // main/tree view asked us a sha not in history

   QModelIndex index = currentIndex();
   QItemSelectionModel *sel = selectionModel();

   if (index.isValid() && (index.row() == stRow))
   {

      if (sel->isSelected(index) != st->selectItem())
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

         // emits QItemSelectionModel::currentChanged()
         setCurrentIndex(newIndex);
         scrollTo(newIndex);
         if (!st->selectItem())
            sel->select(newIndex, QItemSelectionModel::Deselect);
      }
   }

   emit diffTargetChanged(mRevCache ? mRevCache->row(st->sha()) : -1);

   setupGeometry();

   return currentIndex().isValid();
}

void RepositoryView::currentChanged(const QModelIndex &index, const QModelIndex &)
{

   const auto &selRev = model()->index(index.row(), static_cast<int>(RepositoryModelColumns::SHA)).data().toString();
   if (st->sha() != selRev)
   { // to avoid looping
      st->setSha(selRev);
      st->setSelectItem(true);
      d->update(false);

      emit clicked(index);
   }
}

void RepositoryView::clear(bool complete)
{
   d->clear(complete);
   mRepositoryModel->clear();
}

Domain *RepositoryView::domain()
{
   return d;
}

void RepositoryView::focusOnCommit(const QString &goToSha)
{
   const auto sha = mGit->getRefSha(goToSha);

   QLog_Info("UI", QString("Setting the focus on the commit {%1}").arg(sha));

   if (!sha.isEmpty())
   {
      d->st.setSha(sha);
      d->update(false);
      update();
   }
}

QVariant RepositoryView::data(int row, RepositoryModelColumns column) const
{
   return model()->index(row, static_cast<int>(column)).data();
}

bool RepositoryView::filterRightButtonPressed(QMouseEvent *e)
{

   QModelIndex index = indexAt(e->pos());
   const auto &selSha = model()->index(index.row(), static_cast<int>(RepositoryModelColumns::SHA)).data().toString();

   if (selSha.isEmpty())
      return false;

   if (e->modifiers() == Qt::ControlModifier)
   { // check for 'diff to' function
      if (selSha != ZERO_SHA)
      {
         st->setDiffToSha(selSha != st->diffToSha() ? selSha : QString());
         d->update(false);
         return true; // filter event out
      }
   }
   return false;
}

void RepositoryView::showContextMenu(const QPoint &pos)
{
   const auto shas = getSelectedSha();

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

QList<QString> RepositoryView::getSelectedSha() const
{
   const auto indexes = selectedIndexes();
   QMap<QDateTime, QString> shas;
   QVector<QVector<QString>> godVector;

   for (auto index : indexes)
   {
      const auto sha = mRepositoryModel->sha(index.row());
      const auto dtStr
          = mRepositoryModel->index(index.row(), static_cast<int>(RepositoryModelColumns::DATE)).data().toString();
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

bool RepositoryView::getLaneParentsChildren(const QString &sha, int x, QStringList &p, QStringList &c)
{
   uint lane = x / LANE_WIDTH;
   int t = getLaneType(sha, lane);
   if (t == static_cast<int>(LaneType::EMPTY) || t == -1)
      return false;

   // first find the parents
   p.clear();
   QString root;
   if (!isFreeLane(static_cast<LaneType>(t)))
   {
      p = mRevCache->getRevLookup(sha).parents(); // pointer cannot be nullptr
      root = sha;
   }
   else
   {
      const QString &par(mRevCache->getLaneParent(sha, lane));
      if (par.isEmpty())
         return false;

      p.append(par);
      root = p.first();
   }
   // then find children
   c = mGit->getChildren(root);
   return true;
}
