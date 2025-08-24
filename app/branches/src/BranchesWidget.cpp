#include <AddSubtreeDlg.h>
#include <BranchesWidget.h>
#include <BranchesWidgetMinimal.h>
#include <GitBase.h>
#include <GitCache.h>
#include <GitConfig.h>
#include <GitQlientSettings.h>
#include <GitStashes.h>
#include <GitSubmodules.h>
#include <GitSubtree.h>
#include <GitTags.h>
#include <RefListWidget.h>
#include <RefTreeWidget.h>
#include <StashesContextMenu.h>
#include <SubmodulesContextMenu.h>

#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include <QLogger.h>
#include <qlistwidget.h>

using namespace QLogger;

BranchesWidget::BranchesWidget(const QSharedPointer<GitCache> &cache,
                               const QSharedPointer<GitBase> &git,
                               QWidget *parent)
   : QFrame(parent)
   , mCache(cache)
   , mGit(git)
   , mGitTags(new GitTags(mGit))
   , mMinimize(new QPushButton())
   , mMinimal(new BranchesWidgetMinimal(mCache, mGit))
{
   mLocalBranches = new RefTreeWidget(tr("Local"), "LocalHeader",
                                      RefTreeWidget::LocalBranches, mCache, mGit);
   mRemoteBranches = new RefTreeWidget(tr("Remote"), "RemoteHeader",
                                       RefTreeWidget::RemoteBranches, mCache, mGit);
   mTags = new RefTreeWidget(tr("Tags"), "TagsHeader",
                             RefTreeWidget::Tags, mCache, mGit);

   mStashes = new RefListWidget(tr("Stashes"), "StashesHeader", mCache, mGit);
   mSubmodules = new RefListWidget(tr("Submodules"), "SubmodulesHeader", mCache, mGit);
   mSubtrees = new RefListWidget(tr("Subtrees"), "SubtreeHeader", mCache, mGit);

   setupUI();
   setupConnections();
}

void BranchesWidget::setupUI()
{
   mMinimize->setIcon(QIcon(":/icons/ahead"));
   mMinimize->setToolTip(tr("Show minimalist view"));
   mMinimize->setObjectName("BranchesWidgetOptionsButton");
   mMinimize->setShortcut(Qt::CTRL | Qt::Key_B);

   const auto separator = new QFrame();
   separator->setObjectName("separator");

   const auto panelsLayout = new QVBoxLayout();
   panelsLayout->setContentsMargins(QMargins());
   panelsLayout->setSpacing(0);
   panelsLayout->setAlignment(Qt::AlignTop);
   panelsLayout->addWidget(mLocalBranches);
   panelsLayout->addWidget(mRemoteBranches);
   panelsLayout->addWidget(mTags);
   panelsLayout->addWidget(mStashes);
   panelsLayout->addWidget(mSubmodules);
   panelsLayout->addWidget(mSubtrees);
   panelsLayout->addWidget(separator);

   mFullBranchFrame = new QFrame();
   mFullBranchFrame->setObjectName("panelsFrame");
   mFullBranchFrame->setLayout(panelsLayout);

   const auto vLayout = new QVBoxLayout();
   vLayout->setContentsMargins(0, 0, 0, 0);
   vLayout->setSpacing(0);
   vLayout->setAlignment(Qt::AlignTop);
   vLayout->addWidget(mMinimize);
   vLayout->addSpacing(5);
   vLayout->addWidget(mFullBranchFrame);

   const auto mainLayout = new QGridLayout(this);
   mainLayout->setContentsMargins(QMargins());
   mainLayout->setSpacing(0);
   mainLayout->addLayout(vLayout, 0, 0, 3, 1);
   mainLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding), 0, 1);
   mainLayout->addWidget(mMinimal, 1, 1);
   mainLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding), 2, 1);

   GitQlientSettings settings(mGit->getGitDir());
   const auto isMinimalVisible = settings.localValue("MinimalBranchesView", false).toBool();
   mFullBranchFrame->setVisible(!isMinimalVisible);
   mMinimal->setVisible(isMinimalVisible);
}

void BranchesWidget::setupConnections()
{
   connect(mCache.get(), &GitCache::signalCacheUpdated, this, &BranchesWidget::showBranches);

   connect(mMinimize, &QPushButton::clicked, this, &BranchesWidget::minimalView);
   connect(mMinimal, &BranchesWidgetMinimal::showFullBranchesView, this, &BranchesWidget::fullView);
   connect(mMinimal, &BranchesWidgetMinimal::commitSelected, this, &BranchesWidget::signalSelectCommit);
   connect(mMinimal, &BranchesWidgetMinimal::stashSelected, this,
           [this](const QString &stashId) {
              QScopedPointer<GitTags> git(new GitTags(mGit));
              const auto sha = git->getTagCommit(stashId).output;
              emit signalSelectCommit(sha);
           });

   auto connectRefWidget = [this](RefTreeWidget *widget, RefTreeWidget *other) {
      connect(widget, &RefTreeWidget::signalSelectCommit, this, &BranchesWidget::signalSelectCommit);
      connect(widget, &RefTreeWidget::signalSelectCommit, other, &RefTreeWidget::clearSelection);
      connect(widget, &RefTreeWidget::fullReload, this, &BranchesWidget::fullReload);
      connect(widget, &RefTreeWidget::logReload, this, &BranchesWidget::logReload);
      connect(widget, &RefTreeWidget::signalMergeRequired, this, &BranchesWidget::signalMergeRequired);
      connect(widget, &RefTreeWidget::mergeSqushRequested, this, &BranchesWidget::mergeSqushRequested);
      connect(widget, &RefTreeWidget::signalPullConflict, this, &BranchesWidget::signalPullConflict);
   };

   connectRefWidget(mLocalBranches, mRemoteBranches);
   connectRefWidget(mRemoteBranches, mLocalBranches);

   connect(mTags, &RefTreeWidget::signalSelectCommit, this, &BranchesWidget::signalSelectCommit);
   connect(mTags, &RefTreeWidget::signalSelectCommit, mLocalBranches, &RefTreeWidget::clearSelection);
   connect(mTags, &RefTreeWidget::signalSelectCommit, mRemoteBranches, &RefTreeWidget::clearSelection);

   connect(mStashes, &RefListWidget::itemSelected, this,
           [this](const QString &stashId) {
              QScopedPointer<GitTags> git(new GitTags(mGit));
              const auto sha = git->getTagCommit(stashId).output;
              emit signalSelectCommit(sha);
           });

   connect(mStashes, &RefListWidget::contextMenuRequested, this, &BranchesWidget::handleStashesContextMenu);

   connect(mSubmodules, &RefListWidget::itemDoubleClicked, this,
           [this](const QString &name) {
              emit signalOpenSubmodule(mGit->getWorkingDir().append("/").append(name));
           });

   connect(mSubmodules, &RefListWidget::contextMenuRequested, this, &BranchesWidget::handleSubmodulesContextMenu);
   connect(mSubtrees, &RefListWidget::contextMenuRequested, this, &BranchesWidget::handleSubtreesContextMenu);
}

void BranchesWidget::showBranches()
{
   QLog_Info("UI", QString("Loading branches data"));

   clear();
   mMinimal->clearActions();

   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

   processBranches();
   processTags();
   processListItems();

   QApplication::restoreOverrideCursor();

   mLocalBranches->adjustBranchesTree();
   mRemoteBranches->adjustBranchesTree();
}

void BranchesWidget::processBranches()
{
   auto branches = mCache->getBranches(References::Type::LocalBranch);
   int localCount = 0;

   for (const auto &[sha, branchList] : qAsConst(branches))
   {
      for (const auto &branch : branchList)
      {
         if (!branch.contains("HEAD->"))
         {
            const bool isCurrentBranch = (branch == mGit->getCurrentBranch());
            mLocalBranches->addItems(isCurrentBranch, branch, sha);
            mMinimal->configureLocalMenu(sha, branch);
            localCount++;
         }
      }
   }

   mLocalBranches->setCount(localCount);

   branches = mCache->getBranches(References::Type::RemoteBranche);
   int remoteCount = 0;

   for (const auto &[sha, branchList] : qAsConst(branches))
   {
      for (const auto &branch : branchList)
      {
         if (!branch.contains("HEAD->"))
         {
            mRemoteBranches->addItems(false, branch, sha);
            mMinimal->configureRemoteMenu(sha, branch);
            remoteCount++;
         }
      }
   }

   mRemoteBranches->setCount(remoteCount);
}

void BranchesWidget::processTags()
{
   mTags->clear();

   const auto localTags = mCache->getTags(References::Type::LocalTag);
   const auto remoteTags = mCache->getTags(References::Type::RemoteTag);
   int tagsCount = 0;

   for (auto iter = localTags.cbegin(); iter != localTags.cend(); ++iter)
   {
      const auto fullTagName = iter.key();
      const auto sha = iter.value();
      const bool isRemote = remoteTags.contains(fullTagName);

      QString displayName = fullTagName;
      if (!isRemote)
         displayName += " (local)";

      mTags->addItems(false, displayName, sha);
      tagsCount++;
   }

   for (auto iter = remoteTags.cbegin(); iter != remoteTags.cend(); ++iter)
   {
      if (!localTags.contains(iter.key()))
      {
         mTags->addItems(false, iter.key(), iter.value());
         tagsCount++;
      }
   }

   mTags->setCount(tagsCount);
}

void BranchesWidget::processListItems()
{
   processGenericList(mStashes, [this]() {
      QScopedPointer<GitStashes> git(new GitStashes(mGit));
      const auto stashes = git->getStashes();

      for (const auto &stash : stashes)
      {
         const auto stashId = stash.split(":").first();
         const auto stashDesc = stash.split("}: ").last();
         mStashes->addItem(stashDesc, stashId);
         mMinimal->configureStashesMenu(stashId, stashDesc);
      }

      return stashes.count();
   });

   processGenericList(mSubmodules, [this]() {
      QScopedPointer<GitSubmodules> git(new GitSubmodules(mGit));
      const auto submodules = git->getSubmodules();

      for (const auto &submodule : submodules)
      {
         mSubmodules->addItem(submodule, submodule);
         mMinimal->configureSubmodulesMenu(submodule);
      }

      return submodules.count();
   });

   processGenericList(mSubtrees, [this]() {
      QScopedPointer<GitSubtree> git(new GitSubtree(mGit));
      const auto ret = git->list();
      int count = 0;

      if (ret.success)
      {
         const auto commits = ret.output.split("\n\n");

         for (const auto &subtreeRawData : commits)
         {
            if (!subtreeRawData.isEmpty())
            {
               QString name;
               const auto fields = subtreeRawData.split("\n");

               for (const auto &field : fields)
               {
                  if (field.contains("git-subtree-dir:"))
                  {
                     name = field.mid(field.indexOf(":") + 1).trimmed();
                     break;
                  }
               }

               if (!name.isEmpty())
               {
                  mSubtrees->addItem(name, name);
                  count++;
               }
            }
         }
      }

      return count;
   });
}

template<typename Processor>
void BranchesWidget::processGenericList(RefListWidget *widget, Processor processor)
{
   widget->clear();
   const int count = processor();
   widget->setCount(count);

   QLog_Info("UI", QString("Processed %1 items for %2").arg(count).arg(widget->objectName()));
}

void BranchesWidget::clear()
{
   blockSignals(true);
   mLocalBranches->clear();
   mRemoteBranches->clear();
   mTags->clear();
   mStashes->clear();
   mSubmodules->clear();
   mSubtrees->clear();
   blockSignals(false);
}

void BranchesWidget::refreshCurrentBranchLink()
{
   mLocalBranches->reloadCurrentBranchLink();
}

void BranchesWidget::onPanelsVisibilityChaned()
{
   mLocalBranches->reloadVisibility();
   mRemoteBranches->reloadVisibility();
   mTags->reloadVisibility();
   mStashes->reloadVisibility();
   mSubmodules->reloadVisibility();
   mSubtrees->reloadVisibility();
}

void BranchesWidget::handleTagsContextMenu(const QPoint &pos, const QString &tagName)
{
   if (tagName.isEmpty())
      return;

   const auto menu = new QMenu(this);
   const auto isRemote = !tagName.endsWith(" (local)");
   const auto cleanTagName = tagName.endsWith(" (local)")
       ? tagName.left(tagName.lastIndexOf(" (local)"))
       : tagName;

   const auto removeTagAction = menu->addAction(tr("Remove tag"));
   connect(removeTagAction, &QAction::triggered, this, [this, cleanTagName, isRemote]() {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      QScopedPointer<GitTags> git(new GitTags(mGit));
      const auto ret = git->removeTag(cleanTagName, isRemote);
      QApplication::restoreOverrideCursor();

      if (ret.success)
         mGitTags->getRemoteTags();
   });

   const auto pushTagAction = menu->addAction(tr("Push tag"));
   pushTagAction->setEnabled(!isRemote);
   connect(pushTagAction, &QAction::triggered, this, [this, cleanTagName]() {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      QScopedPointer<GitTags> git(new GitTags(mGit));
      const auto ret = git->pushTag(cleanTagName);
      QApplication::restoreOverrideCursor();

      if (ret.success)
         mGitTags->getRemoteTags();
   });

   menu->exec(pos);
}

void BranchesWidget::handleStashesContextMenu(const QPoint &pos, const QString &stashId)
{
   if (stashId.isEmpty())
      return;

   const auto menu = new StashesContextMenu(mGit, stashId, this);
   connect(menu, &StashesContextMenu::signalUpdateView, this, &BranchesWidget::fullReload);
   connect(menu, &StashesContextMenu::signalContentRemoved, this, &BranchesWidget::fullReload);
   menu->exec(pos);
}

void BranchesWidget::handleSubmodulesContextMenu(const QPoint &pos, const QString &submoduleName)
{
   const auto index = mSubmodules->listWidget()->indexAt(pos);
   const auto menu = new SubmodulesContextMenu(mGit, index, mSubmodules->listWidget()->count(), this);
   connect(menu, &SubmodulesContextMenu::openSubmodule, this, &BranchesWidget::signalOpenSubmodule);
   connect(menu, &SubmodulesContextMenu::infoUpdated, this, &BranchesWidget::fullReload);
   menu->exec(pos);
}

void BranchesWidget::handleSubtreesContextMenu(const QPoint &pos, const QString &subtreeName)
{
   const auto menu = new QMenu(this);

   if (!subtreeName.isEmpty())
   {
      connect(menu->addAction(tr("Pull")), &QAction::triggered, this, [this, subtreeName]() {
         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
         const auto subtreeData = getSubtreeData(subtreeName);
         QScopedPointer<GitSubtree> git(new GitSubtree(mGit));
         const auto ret = git->pull(subtreeData.first, subtreeData.second, subtreeName);
         QApplication::restoreOverrideCursor();

         if (ret.success)
            emit fullReload();
         else
            QMessageBox::warning(this, tr("Error when pulling"), ret.output);
      });

      connect(menu->addAction(tr("Push")), &QAction::triggered, this, [this, subtreeName]() {
         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
         const auto subtreeData = getSubtreeData(subtreeName);
         QScopedPointer<GitSubtree> git(new GitSubtree(mGit));
         const auto ret = git->push(subtreeData.first, subtreeData.second, subtreeName);
         QApplication::restoreOverrideCursor();

         if (ret.success)
            emit fullReload();
         else
            QMessageBox::warning(this, tr("Error when pushing"), ret.output);
      });

      connect(menu->addAction(tr("Configure")), &QAction::triggered, this, [this, subtreeName]() {
         const auto subtreeData = getSubtreeData(subtreeName);
         AddSubtreeDlg addDlg(subtreeName, subtreeData.first, subtreeData.second, mGit);
         if (addDlg.exec() == QDialog::Accepted)
            emit fullReload();
      });
   }
   else
   {
      connect(menu->addAction(tr("Add subtree")), &QAction::triggered, this, [this]() {
         AddSubtreeDlg addDlg(mGit);
         if (addDlg.exec() == QDialog::Accepted)
            emit fullReload();
      });
   }

   menu->exec(pos);
}

void BranchesWidget::fullView()
{
   mFullBranchFrame->setVisible(true);
   mMinimal->setVisible(false);
   emit minimalViewStateChanged(false);

   GitQlientSettings settings(mGit->getGitDir());
   settings.setLocalValue("MinimalBranchesView", false);
}

void BranchesWidget::minimalView()
{
   forceMinimalView();
   GitQlientSettings settings(mGit->getGitDir());
   settings.setLocalValue("MinimalBranchesView", true);
}

void BranchesWidget::forceMinimalView()
{
   mFullBranchFrame->setVisible(false);
   mMinimal->setVisible(true);
   emit minimalViewStateChanged(true);
}

void BranchesWidget::returnToSavedView()
{
   GitQlientSettings settings(mGit->getGitDir());
   const auto savedState = settings.localValue("MinimalBranchesView", false).toBool();

   if (savedState != mMinimal->isVisible())
   {
      mFullBranchFrame->setVisible(!savedState);
      mMinimal->setVisible(savedState);
      emit minimalViewStateChanged(savedState);
   }
}

bool BranchesWidget::isMinimalViewActive() const
{
   GitQlientSettings settings(mGit->getGitDir());
   return settings.localValue("MinimalBranchesView", false).toBool();
}

QPair<QString, QString> BranchesWidget::getSubtreeData(const QString &prefix)
{
   GitQlientSettings settings(mGit->getGitDir());
   bool end = false;
   QString url;
   QString ref;

   for (auto i = 0; !end; ++i)
   {
      const auto repo = settings.localValue(QString("Subtrees/%1.prefix").arg(i), "");

      if (repo.toString() == prefix)
      {
         auto tmpUrl = settings.localValue(QString("Subtrees/%1.url").arg(i)).toString();
         auto tmpRef = settings.localValue(QString("Subtrees/%1.ref").arg(i)).toString();

         if (tmpUrl.isEmpty() || tmpRef.isEmpty())
         {
            const auto resp
                = QMessageBox::question(this, tr("Subtree configuration not found!"),
                                        tr("The subtree configuration was not found. It could be that it was created "
                                           "outside GitQlient.<br>To operate with this subtree, it needs to be "
                                           "configured.<br><br><b>Do you want to configure it now?<b>"));

            if (resp == QMessageBox::Yes)
            {
               AddSubtreeDlg stDlg(prefix, mGit, this);
               const auto ret = stDlg.exec();

               if (ret == QDialog::Accepted)
               {
                  tmpUrl = settings.localValue(QString("Subtrees/%1.url").arg(i)).toString();
                  tmpRef = settings.localValue(QString("Subtrees/%1.ref").arg(i)).toString();

                  if (tmpUrl.isEmpty() || tmpRef.isEmpty())
                     QMessageBox::critical(this, tr("Unexpected error!"),
                                           tr("An unidentified error happened while using subtrees. Please contact the "
                                              "creator of GitQlient for support."));
                  else
                  {
                     url = tmpUrl;
                     ref = tmpRef;
                  }
               }
            }

            end = true;
         }
         else
         {
            url = tmpUrl;
            ref = tmpRef;
            end = true;
         }
      }
      else if (repo.toString().isEmpty())
      {
         const auto resp
             = QMessageBox::question(this, tr("Subtree configuration not found!"),
                                     tr("The subtree configuration was not found. It could be that it was created "
                                        "outside GitQlient.<br>To operate with this subtree, it needs to be "
                                        "configured.<br><br><b>Do you want to configure it now?<b>"));

         if (resp == QMessageBox::Yes)
         {
            AddSubtreeDlg stDlg(prefix, mGit, this);
            const auto ret = stDlg.exec();

            if (ret == QDialog::Accepted)
            {
               const auto tmpUrl = settings.localValue(QString("Subtrees/%1.url").arg(i)).toString();
               const auto tmpRef = settings.localValue(QString("Subtrees/%1.ref").arg(i)).toString();

               if (tmpUrl.isEmpty() || tmpRef.isEmpty())
                  QMessageBox::critical(this, tr("Unexpected error!"),
                                        tr("An unidentified error happened while using subtrees. Please contact the "
                                           "creator of GitQlient for support."));
               else
               {
                  url = tmpUrl;
                  ref = tmpRef;
               }
            }
         }

         end = true;
      }
   }

   return qMakePair(url, ref);
}
