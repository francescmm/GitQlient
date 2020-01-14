#include "GitQlient.h"

#include <ConfigWidget.h>
#include <GitQlientStyles.h>

#include <QProcess>
#include <QTabWidget>
#include <QTabBar>
#include <GitQlientRepo.h>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFile>
#include <QFileDialog>

#include <QLogger.h>

using namespace QLogger;

GitQlient::GitQlient(QWidget *parent)
   : GitQlient(QStringList(), parent)
{
}

GitQlient::GitQlient(const QStringList &arguments, QWidget *parent)
   : QWidget(parent)
   , mRepos(new QTabWidget())
   , mConfigWidget(new ConfigWidget())
{
   const auto repos = parseArguments(arguments);

   QLog_Info("UI", "*******************************************");
   QLog_Info("UI", "*          GitQlient has started          *");
   QLog_Info("UI", QString("*                 %1                 *").arg(VER));
   QLog_Info("UI", "*******************************************");

   QFile styles(":/stylesheet");

   setStyleSheet(GitQlientStyles::getStyles());

   const auto addTab = new QPushButton();
   addTab->setObjectName("openNewRepo");
   addTab->setIcon(QIcon(":/icons/open_repo"));
   addTab->setToolTip(tr("Open new repository"));
   connect(addTab, &QPushButton::clicked, this, &GitQlient::openRepo);

   mRepos->setCornerWidget(addTab, Qt::TopRightCorner);
   mRepos->setTabsClosable(true);
   connect(mRepos, &QTabWidget::tabCloseRequested, this, &GitQlient::closeTab);

   const auto vLayout = new QVBoxLayout(this);
   vLayout->setContentsMargins(QMargins());
   vLayout->addWidget(mRepos);

   auto configIndex = mRepos->addTab(mConfigWidget, QIcon(":/icons/config"), QString());
   mRepos->tabBar()->setTabButton(configIndex, QTabBar::RightSide, nullptr);

   connect(mConfigWidget, &ConfigWidget::signalOpenRepo, this, &GitQlient::addRepoTab);

   setRepositories(repos);
}

GitQlient::~GitQlient()
{
   QLog_Info("UI", "*            Closing GitQlient            *\n\n");
}

void GitQlient::openRepo()
{
   const QString dirName(QFileDialog::getExistingDirectory(this, "Choose the directory of a Git project"));

   if (!dirName.isEmpty())
   {
      QDir d(dirName);
      addRepoTab(d.absolutePath());
   }
}

void GitQlient::setRepositories(const QStringList &repositories)
{
   QLog_Info("UI", QString("Adding {%1} repositories").arg(repositories.count()));

   for (const auto &repo : repositories)
      addRepoTab(repo);
}

void GitQlient::setArgumentsPostInit(const QStringList &arguments)
{
   QLog_Info("UI", QString("External call with the params {%1}").arg(arguments.join(",")));

   const auto repos = parseArguments(arguments);

   setRepositories(repos);
}

QStringList GitQlient::parseArguments(const QStringList &arguments)
{
   const auto manager = QLoggerManager::getInstance();
   manager->addDestination("GitQlient.log", { "UI", "Git" }, LogLevel::Debug);

   if (arguments.contains("-noLog"))
      QLoggerManager::getInstance()->pause();

   QLog_Info("UI", QString("Getting arguments {%1}").arg(arguments.join(", ")));

   QStringList repos;
   const auto argSize = arguments.count();

   for (auto i = 0; i < argSize;)
   {
      if (arguments.at(i) == "-repos")
      {
         while (++i < argSize && !arguments.at(i).startsWith("-"))
            repos.append(arguments.at(i));
      }
      else
         ++i;
   }

   return repos;
}

void GitQlient::addRepoTab(const QString &repoPath)
{
   if (!mCurrentRepos.contains(repoPath))
   {
      const auto newRepo = new GitQlientRepo(repoPath);
      connect(newRepo, &GitQlientRepo::signalRepoOpened, mConfigWidget, &ConfigWidget::updateRecentProjectsList);
      connect(newRepo, &GitQlientRepo::signalOpenSubmodule, this, [this](const QString &repoName) {
         const auto currentDir = dynamic_cast<GitQlientRepo *>(sender())->currentDir();

         auto submoduleDir = QString("%1/%2").arg(currentDir, repoName);

         QLog_Info("UI", QString("Adding a new tab for the submodule {%1} in {%2}").arg(repoName, currentDir));

         addRepoTab(submoduleDir);
      });

      const auto repoName = newRepo->currentDir().contains("/") ? newRepo->currentDir().split("/").last() : "No repo";
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

         QLog_Info("UI", "Attaching repository to a new tab");

         if (isSubmodule)
         {
            const auto parentRepo = QString::fromUtf8(output.split('/').last());

            mRepos->setTabText(index, QString("%1 \u2192 %2").arg(parentRepo, repoName));

            QLog_Info("UI",
                      QString("Opening the submodule {%1} from the repo {%2} on tab index {%3}")
                          .arg(repoName, parentRepo)
                          .arg(index));
         }
      }

      mRepos->setCurrentIndex(index);

      mCurrentRepos.insert(repoPath);
   }
   else
      QLog_Warning("UI", QString("Repository at {%1} already opened. Skip adding it again.").arg(repoPath));
}

void GitQlient::closeTab(int tabIndex)
{
   auto repoToRemove = dynamic_cast<GitQlientRepo *>(mRepos->widget(tabIndex));

   QLog_Info("UI", QString("Removing repository {%1}").arg(repoToRemove->currentDir()));

   mCurrentRepos.remove(repoToRemove->currentDir());
   mRepos->removeTab(tabIndex);
   repoToRemove->close();
}
