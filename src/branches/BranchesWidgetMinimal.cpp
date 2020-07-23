#include "BranchesWidgetMinimal.h"

#include <RevisionsCache.h>
#include <GitSubmodules.h>
#include <GitStashes.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QToolButton>
#include <QPushButton>
#include <QEvent>

BranchesWidgetMinimal::BranchesWidgetMinimal(const QSharedPointer<RevisionsCache> &cache,
                                             const QSharedPointer<GitBase> git, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mCache(cache)
   , mBack(new QPushButton())
   , mLocal(new QToolButton())
   , mRemote(new QToolButton())
   , mTags(new QToolButton())
   , mStashes(new QToolButton())
   , mSubmodules(new QToolButton())
{
   mBack->setIcon(QIcon(":/icons/back"));
   connect(mBack, &QPushButton::clicked, this, &BranchesWidgetMinimal::showFullBranchesView);

   const auto layout = new QGridLayout(this);
   layout->setContentsMargins(QMargins());
   layout->setSpacing(0);
   layout->addWidget(mBack, 0, 0);
   layout->addWidget(mLocal, 1, 0);
   layout->addWidget(mRemote, 2, 0);
   layout->addWidget(mTags, 3, 0);
   layout->addWidget(mStashes, 4, 0);
   layout->addWidget(mSubmodules, 5, 0);
}

void BranchesWidgetMinimal::configure()
{
   configureLocalMenu();
   configureRemoteMenu();
   configureTagsMenu();
   configureStashesMenu();
   configureSubmodulesMenu();
}

bool BranchesWidgetMinimal::eventFilter(QObject *obj, QEvent *event)
{
   if (const auto menu = qobject_cast<QMenu *>(obj); menu && event->type() == QEvent::Show)
   {
      auto localPos = menu->parentWidget()->pos();
      localPos.setX(localPos.x());
      auto pos = mapToGlobal(localPos);
      menu->show();
      pos.setX(pos.x() - menu->width());
      menu->move(pos);
      return true;
   }
   return false;
}

void BranchesWidgetMinimal::configureLocalMenu()
{
   auto branches = mCache->getBranches(References::Type::LocalBranch);
   auto count = 0;

   if (!branches.isEmpty())
   {
      const auto menu = new QMenu(mLocal);
      menu->installEventFilter(this);

      for (const auto &pair : branches)
      {
         for (const auto &branch : pair.second)
         {
            if (!branch.contains("HEAD->"))
            {
               ++count;
               const auto action = new QAction(branch);
               action->setData(pair.first);
               menu->addAction(action);
            }
         }
      }

      mLocal->setMenu(menu);
   }

   mLocal->setIcon(QIcon(":/icons/local"));
   mLocal->setText("   " + QString::number(count));
   mLocal->setPopupMode(QToolButton::InstantPopup);
   mLocal->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
}

void BranchesWidgetMinimal::configureRemoteMenu()
{
   auto branches = mCache->getBranches(References::Type::RemoteBranches);
   auto count = 0;

   if (!branches.isEmpty())
   {
      const auto menu = new QMenu(mRemote);
      menu->installEventFilter(this);

      for (const auto &pair : branches)
      {
         for (const auto &branch : pair.second)
         {
            if (!branch.contains("HEAD->"))
            {
               ++count;
               const auto action = new QAction(branch);
               action->setData(pair.first);
               menu->addAction(action);
            }
         }
      }

      mRemote->setMenu(menu);
   }

   mRemote->setIcon(QIcon(":/icons/server"));
   mRemote->setText("   " + QString::number(count));
   mRemote->setPopupMode(QToolButton::InstantPopup);
   mRemote->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
}

void BranchesWidgetMinimal::configureTagsMenu()
{
   const auto localTags = mCache->getTags(References::Type::LocalTag);
   const auto remoteTags = mCache->getTags(References::Type::RemoteTag);

   if (!localTags.isEmpty())
   {
      const auto menu = new QMenu(mTags);
      menu->installEventFilter(this);

      for (const auto &tag : localTags.toStdMap())
      {
         const auto action = new QAction(tag.first);
         action->setData(tag.second);
         menu->addAction(action);
      }

      mTags->setMenu(menu);
   }

   mTags->setIcon(QIcon(":/icons/tags"));
   mTags->setText("   " + QString::number(localTags.count()));
   mTags->setPopupMode(QToolButton::InstantPopup);
   mTags->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
}

void BranchesWidgetMinimal::configureStashesMenu()
{
   QScopedPointer<GitStashes> git(new GitStashes(mGit));
   const auto stashes = git->getStashes();

   if (!stashes.isEmpty())
   {
      const auto menu = new QMenu(mStashes);
      menu->installEventFilter(this);

      for (const auto &stash : stashes)
      {
         const auto stashId = stash.split(":").first();
         const auto stashDesc = stash.split("}: ").last();
         const auto action = new QAction(stashDesc);
         action->setData(stashId);
         menu->addAction(action);
      }

      mStashes->setMenu(menu);
   }

   mStashes->setIcon(QIcon(":/icons/stashes"));
   mStashes->setText("   " + QString::number(stashes.count()));
   mStashes->setPopupMode(QToolButton::InstantPopup);
   mStashes->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
}

void BranchesWidgetMinimal::configureSubmodulesMenu()
{
   QScopedPointer<GitSubmodules> gitSM(new GitSubmodules(mGit));
   const auto submodules = gitSM->getSubmodules();
   const auto menu = new QMenu(mSubmodules);
   menu->installEventFilter(this);

   for (const auto &submodule : submodules)
      menu->addAction(new QAction(submodule));

   mSubmodules->setIcon(QIcon(":/icons/submodules"));
   mSubmodules->setText("   " + QString::number(submodules.count()));
   mSubmodules->setPopupMode(QToolButton::InstantPopup);
   mSubmodules->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
   mSubmodules->setMenu(menu);
}
