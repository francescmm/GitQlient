#include "GitQlient.h"

#include <QProcess>
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
   QFile styles(":/stylesheet.css");

   if (styles.open(QIODevice::ReadOnly))
   {
      setStyleSheet(QString::fromUtf8(styles.readAll()));
      styles.close();
   }

   const auto newRepo = new QPushButton();
   newRepo->setObjectName("openGitAdmin");
   newRepo->setIcon(QIcon(":/icons/git_orange"));
   newRepo->setToolTip(tr("Open new repository"));
   // connect(newRepo, &QPushButton::clicked, this, &GitQlient::openRepo);

   const auto addTab = new QPushButton();
   addTab->setObjectName("openNewRepo");
   addTab->setIcon(QIcon(":/icons/open_repo"));
   addTab->setToolTip(tr("Open new repository"));
   connect(addTab, &QPushButton::clicked, this, &GitQlient::openRepo);

   mRepos->setCornerWidget(newRepo, Qt::TopLeftCorner);
   mRepos->setCornerWidget(addTab, Qt::TopRightCorner);
   mRepos->setTabsClosable(true);
   connect(mRepos, &QTabWidget::tabCloseRequested, this, &GitQlient::repoClosed);

   const auto vLayout = new QVBoxLayout(this);
   vLayout->setContentsMargins(QMargins());
   vLayout->addWidget(mRepos);

   addRepoTab();
}

void GitQlient::setRepositories(const QStringList repositories)
{
   if (!mFirstRepoInitialized)
   {
      closeTab(0);
      mFirstRepoInitialized = true;
   }

   for (auto repo : repositories)
      addRepoTab(repo);
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
   if (!mCurrentRepos.contains(repoPath))
   {
      const auto newRepo = new GitQlientRepo(repoPath);
      connect(newRepo, &GitQlientRepo::signalOpenSubmodule, this, [this](const QString &repoName) {
         if (!mFirstRepoInitialized)
         {
            closeTab(0);
            mFirstRepoInitialized = true;
         }

         auto submoduleDir = dynamic_cast<GitQlientRepo *>(sender())->currentDir() + "/" + repoName;
         addRepoTab(submoduleDir);
      });

      const auto repoName = repoPath.contains("/") ? repoPath.split("/").last() : "No repo";
      const auto index = mRepos->addTab(newRepo, repoName);

      if (!repoPath.isEmpty())
      {
         QProcess p;
         p.setWorkingDirectory(repoPath);
         p.start("git rev-parse --show-superproject-working-tree");
         p.waitForFinished(5000);

         const auto output = p.readAll().trimmed();
         const auto isSubmodule = !output.isEmpty();

         mRepos->setTabIcon(index, QIcon(isSubmodule ? QString(":/icons/submodules") : QString(":/icons/local")));

         if (isSubmodule)
         {
            const auto parentRepo = output.split('/').last();
            mRepos->setTabText(index, QString("%1 \u2192 %2").arg(parentRepo, repoName));
         }
      }

      mRepos->setCurrentIndex(index);

      mCurrentRepos.insert(repoPath);
   }
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
   auto repoToRemove = dynamic_cast<GitQlientRepo *>(mRepos->widget(tabIndex));
   mCurrentRepos.remove(repoToRemove->currentDir());
   mRepos->removeTab(tabIndex);
   repoToRemove->close();
}
