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
#include "domain.h"
#include "git.h"
#include <RepositoryContextMenu.h>
#include <RepositoryViewDelegate.h>
#include <QLogger.h>
#include <lanes.h>

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

using namespace QLogger;

uint refTypeFromName(const QString &name);

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
   header()->setSortIndicatorShown(false);

   const auto lvd = new RepositoryViewDelegate(mGit, revCache);
   setItemDelegate(lvd);

   connect(lvd, &RepositoryViewDelegate::updateView, viewport(), qOverload<>(&QWidget::update));
   connect(this, &RepositoryView::diffTargetChanged, lvd, &RepositoryViewDelegate::diffTargetChanged);
   connect(this, &RepositoryView::customContextMenuRequested, this, &RepositoryView::showContextMenu);
   connect(mGit.get(), &Git::newRevsAdded, this, [this]() {
      if (!d->st.sha().isEmpty() || model()->rowCount() == 0)
         return;

      d->st.setSha(sha(0));
      d->st.setSelectItem(true);
      update();
   });
   connect(mGit.get(), &Git::loadCompleted, this, [this]() { d->update(false); });
}

void RepositoryView::setup()
{
   st = &(d->st);
   filterNextContextMenuRequest = false;
   setModel(mRepositoryModel);

   setupGeometry(); // after setting delegate
}

RepositoryView::~RepositoryView()
{
   mGit->cancelDataLoading(); // non blocking
}

const QString RepositoryView::sha(int row) const
{
   return model() ? model()->index(row, static_cast<int>(RepositoryModelColumns::SHA)).data().toString() : QString();
}

int RepositoryView::row(const QString &sha) const
{
   return mRevCache->row(sha);
}

void RepositoryView::setupGeometry()
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

void RepositoryView::scrollToNext(int direction)
{
   // Depending on the value of direction, scroll to:
   // -1 = the next child in history
   //  1 = the previous parent in history
   const QString &s = sha(currentIndex().row());
   const auto r = mRevCache->revLookup(s);

   if (r)
   {
      const QStringList &next = direction < 0 ? mGit->getChildren(s) : r->parents();
      if (next.size() >= 1)
         setCurrentIndex(model()->index(row(next.first()), 0));
   }
}

void RepositoryView::scrollToCurrent(ScrollHint hint)
{

   if (currentIndex().isValid())
      scrollTo(currentIndex(), hint);
}

int RepositoryView::getLaneType(const QString &sha, int pos) const
{

   const auto r = mRevCache->revLookup(sha);
   return (r && pos < r->lanes.count() && pos >= 0 ? r->lanes.at(pos) : -1);
}

void RepositoryView::getSelectedItems(QStringList &selectedItems)
{

   selectedItems.clear();
   const auto ml = selectionModel()->selectedRows();
   for (auto &it : ml)
      selectedItems.append(sha(it.row()));

   // selectedRows() returns the items in an unspecified order,
   // so be sure rows are ordered from newest to oldest.
   selectedItems = mGit->sortShaListByIndex(selectedItems);
}

const QString RepositoryView::shaFromAnnId(int id)
{
   return sha(model()->rowCount() - id);
}

bool RepositoryView::update()
{

   int stRow = row(st->sha());
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

   emit diffTargetChanged(row(st->diffToSha()));

   setupGeometry();

   return currentIndex().isValid();
}

void RepositoryView::currentChanged(const QModelIndex &index, const QModelIndex &)
{

   const QString &selRev = sha(index.row());
   if (st->sha() != selRev)
   { // to avoid looping
      st->setSha(selRev);
      st->setSelectItem(true);
      d->update(false);

      emit clicked(index);
   }
}

void RepositoryView::markDiffToSha(const QString &sha)
{
   st->setDiffToSha(sha != st->diffToSha() ? sha : QString());
   d->update(false);
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
   QLog_Info("UI", QString("Setting the focus on the commit {%1}").arg(goToSha));

   const auto sha = mGit->getRefSha(goToSha);

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
   const QString &selSha = sha(index.row());
   if (selSha.isEmpty())
      return false;

   if (e->modifiers() == Qt::ControlModifier)
   { // check for 'diff to' function
      if (selSha != ZERO_SHA)
      {

         filterNextContextMenuRequest = true;
         markDiffToSha(selSha);
         return true; // filter event out
      }
   }
   return false;
}

uint refTypeFromName(const QString &name)
{
   if (name.startsWith("tags/"))
      return Git::TAG;
   if (name.startsWith("remotes/"))
      return Git::RMT_BRANCH;
   if (!name.isEmpty())
      return Git::BRANCH;
   return 0;
}

void RepositoryView::showContextMenu(const QPoint &pos)
{
   QModelIndex index = indexAt(pos);

   if (!index.isValid())
      return;

   if (filterNextContextMenuRequest)
   {
      // event filter does not work on them
      filterNextContextMenuRequest = false;
      return;
   }

   const auto sha = mRepositoryModel->sha(index.row());
   const auto menu = new RepositoryContextMenu(mGit, sha, this);
   connect(menu, &RepositoryContextMenu::signalRepositoryUpdated, this, &RepositoryView::signalViewUpdated);
   connect(menu, &RepositoryContextMenu::signalOpenDiff, this, &RepositoryView::signalOpenDiff);
   connect(menu, &RepositoryContextMenu::signalAmmendCommit, this, [this, sha]() { emit signalAmmendCommit(sha); });
   menu->exec(viewport()->mapToGlobal(pos));
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
      p = mRevCache->revLookup(sha)->parents(); // pointer cannot be nullptr
      root = sha;
   }
   else
   {
      const QString &par(mGit->getLaneParent(sha, lane));
      if (par.isEmpty())
         return false;

      p.append(par);
      root = p.first();
   }
   // then find children
   c = mGit->getChildren(root);
   return true;
}
