#include "GitQlient.h"

#include <QTabWidget>
#include <GitQlientRepo.h>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QDir>

GitQlient::GitQlient(QWidget *parent)
   : QWidget(parent)
   , mRepos(new QTabWidget())
{
   const auto newRepo = new QPushButton(tr("Open repo"));
   newRepo->setIcon(QIcon(":/icons/open_repo"));

   connect(newRepo, &QPushButton::clicked, this, &GitQlient::openRepo);

   mRepos->setCornerWidget(newRepo);
   mRepos->setTabsClosable(true);
   connect(mRepos, &QTabWidget::tabCloseRequested, this, &GitQlient::repoClosed);

   const auto vLayout = new QVBoxLayout(this);
   vLayout->setContentsMargins(QMargins());
   vLayout->addWidget(mRepos);

   addRepoTab();
}

void GitQlient::openRepo()
{
   const QString dirName(QFileDialog::getExistingDirectory(this, "Choose the directory of a Git project"));

   if (!dirName.isEmpty())
   {
      QDir d(dirName);

      if (!mFirstRepoInitialized)
      {
         closeTab(0);
         mFirstRepoInitialized = true;
      }

      addRepoTab(d.absolutePath());
   }
}

void GitQlient::addRepoTab(const QString &repoPath)
{
   const auto blankRepo = new GitQlientRepo(repoPath);
   const auto repoName = repoPath.contains("/") ? repoPath.split("/").last() : "No repo";
   const auto index = mRepos->addTab(blankRepo, repoName);

   mRepos->setCurrentIndex(index);

   mCurrentRepos.insert(blankRepo, repoName);
}

void GitQlient::repoClosed(int tabIndex)
{
   closeTab(tabIndex);

   if (mRepos->count() == 0)
   {
      addRepoTab();
      mFirstRepoInitialized = false;
   }
}

void GitQlient::closeTab(int tabIndex)
{
   const auto repoName = mRepos->tabText(tabIndex);
   auto repoToRemove = dynamic_cast<GitQlientRepo *>(mRepos->widget(tabIndex));
   mCurrentRepos.remove(repoToRemove);
   mRepos->removeTab(tabIndex);
   repoToRemove->close();
}
