/*
        Description: qgit revision list view

Author: Marco Costalba (C) 2005-2007

             Copyright: See COPYING file that comes with this distribution

                             */
#include "revsview.h"

#include <RepositoryView.h>
#include <common.h>
#include <domain.h>
#include <FileListWidget.h>
#include <RepositoryModel.h>
#include <git.h>
#include <MainWindow.h>
#include <DiffWidget.h>

#include <QMenu>
#include <QStackedWidget>

RevsView::RevsView(bool isMain)
   : Domain(isMain)
   , listViewLog(new RepositoryView())
{
   listViewLog->setup(this);

   QWidget::connect(Git::getInstance(), &Git::newRevsAdded, this, &RevsView::on_newRevsAdded);
   QWidget::connect(Git::getInstance(), &Git::loadCompleted, this, &RevsView::on_loadCompleted);

   QWidget::connect(listViewLog, &RepositoryView::signalViewUpdated, this, &RevsView::signalViewUpdated);
   QWidget::connect(listViewLog, &RepositoryView::rebase, this, &RevsView::rebase);
   QWidget::connect(listViewLog, &RepositoryView::merge, this, &RevsView::merge);
   QWidget::connect(listViewLog, &RepositoryView::moveRef, this, &RevsView::moveRef);
   QWidget::connect(listViewLog, &RepositoryView::contextMenu, this, &RevsView::on_contextMenu);
   QWidget::connect(listViewLog, &RepositoryView::signalOpenDiff, this, &RevsView::signalOpenDiff);
}

RevsView::~RevsView()
{
   delete listViewLog;
}

void RevsView::clear(bool complete)
{
   Domain::clear(complete);
}

void RevsView::setEnabled(bool enabled)
{
   listViewLog->setEnabled(enabled);
}

void RevsView::on_newRevsAdded(const RepositoryModel *fh, const QVector<QString> &)
{

   if (!Git::getInstance()->isMainHistory(fh) || !st.sha().isEmpty())
      return;

   if (listViewLog->model()->rowCount() == 0)
      return;

   st.setSha(listViewLog->sha(0));
   st.setSelectItem(true);
   doUpdate(false);
}

void RevsView::on_loadCompleted(const RepositoryModel *fh, const QString &)
{

   if (!Git::getInstance()->isMainHistory(fh))
      return;

   Domain::update(false, false);
}

bool RevsView::doUpdate(bool)
{
   return listViewLog->update() || st.sha().isEmpty();
}
