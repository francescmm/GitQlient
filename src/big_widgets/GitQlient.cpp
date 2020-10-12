#include "GitQlient.h"

#include <ConfigWidget.h>
#include <GitQlientStyles.h>
#include <GitQlientSettings.h>
#include <QPinnableTabWidget.h>
#include <InitialRepoConfig.h>

#include <QProcess>
#include <QTabBar>
#include <GitQlientRepo.h>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <GitBase.h>

#include <QLogger.h>

using namespace QLogger;

GitQlient::GitQlient(QWidget *parent)
   : GitQlient(QStringList(), parent)
{
}

GitQlient::GitQlient(const QStringList &arguments, QWidget *parent)
   : QWidget(parent)
   , mRepos(new QPinnableTabWidget())
   , mConfigWidget(new ConfigWidget())
{

   auto repos = parseArguments(arguments);

   QLog_Info("UI", "*******************************************");
   QLog_Info("UI", "*          GitQlient has started          *");
   QLog_Info("UI", QString("*                  %1                  *").arg(VER));
   QLog_Info("UI", "*******************************************");

   setStyleSheet(GitQlientStyles::getStyles());

   const auto addTab = new QPushButton();
   addTab->setObjectName("openNewRepo");
   addTab->setIcon(QIcon(":/icons/open_repo"));
   addTab->setToolTip(tr("Open new repository"));
   connect(addTab, &QPushButton::clicked, this, &GitQlient::openRepo);

   mRepos->setStyleSheet(GitQlientStyles::getStyles());
   mRepos->setCornerWidget(addTab, Qt::TopRightCorner);
   connect(mRepos, &QTabWidget::tabCloseRequested, this, &GitQlient::closeTab);

   const auto vLayout = new QVBoxLayout(this);
   vLayout->setContentsMargins(QMargins());
   vLayout->addWidget(mRepos);
   mRepos->addPinnedTab(mConfigWidget, QIcon(":/icons/config"), QString());

   mConfigWidget->onRepoOpened();

   connect(mConfigWidget, &ConfigWidget::signalOpenRepo, this, &GitQlient::addRepoTab);

   setRepositories(repos);

   GitQlientSettings settings;
   const auto geometry = settings.globalValue("GitQlientGeometry", saveGeometry()).toByteArray();

   if (!geometry.isNull())
      restoreGeometry(geometry);
}

GitQlient::~GitQlient()
{
   GitQlientSettings settings;
   QStringList pinnedRepos;
   const auto totalTabs = mRepos->count();

   for (auto i = 1; i < totalTabs; ++i)
   {
      if (mRepos->isPinned(i))
      {
         auto repoToRemove = dynamic_cast<GitQlientRepo *>(mRepos->widget(i));
         pinnedRepos.append(repoToRemove->currentDir());
      }
   }

   settings.setGlobalValue(GitQlientSettings::PinnedRepos, pinnedRepos);
   settings.setGlobalValue("GitQlientGeometry", saveGeometry());

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

   LogLevel logLevel;
   GitQlientSettings settings;

#ifdef DEBUG
   logLevel = LogLevel::Trace;
#else
   logLevel = static_cast<LogLevel>(settings.globalValue("logsLevel", static_cast<int>(LogLevel::Warning)).toInt());
#endif

   if (arguments.contains("-noLog") || settings.globalValue("logsDisabled", true).toBool())
      QLoggerManager::getInstance()->pause();
   else
      QLoggerManager::getInstance()->overwriteLogLevel(logLevel);

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
      {
         if (arguments.at(i) == "-logLevel")
         {
            logLevel = static_cast<LogLevel>(arguments.at(++i).toInt());

            if (logLevel >= QLogger::LogLevel::Trace && logLevel <= QLogger::LogLevel::Fatal)
            {
               const auto logger = QLoggerManager::getInstance();
               logger->overwriteLogLevel(logLevel);

               settings.setGlobalValue("logsLevel", static_cast<int>(logLevel));
            }
         }

         ++i;
      }
   }

   const auto manager = QLoggerManager::getInstance();
   manager->addDestination("GitQlient.log", { "UI", "Git" }, logLevel);

   return repos;
}

void GitQlient::addRepoTab(const QString &repoPath)
{
   addNewRepoTab(repoPath, false);
}

void GitQlient::addNewRepoTab(const QString &repoPath, bool pinned)
{
   if (!mCurrentRepos.contains(repoPath))
   {
      QFileInfo info(QString("%1/.git").arg(repoPath));

      if (info.isFile() || info.isDir())
      {
         conditionallyOpenPreConfigDlg(repoPath);

         const auto repoName = repoPath.contains("/") ? repoPath.split("/").last() : "No repo";
         const auto repo = new GitQlientRepo(repoPath);
         const auto index = pinned ? mRepos->addPinnedTab(repo, repoName) : mRepos->addTab(repo, repoName);

         connect(repo, &GitQlientRepo::signalEditFile, this, &GitQlient::signalEditDocument);
         connect(repo, &GitQlientRepo::signalOpenSubmodule, this, &GitQlient::addRepoTab);
         connect(repo, &GitQlientRepo::repoOpened, this, &GitQlient::onSuccessOpen);

         repo->setRepository(repoName);

         if (!repoPath.isEmpty())
         {
            QProcess p;
            p.setWorkingDirectory(repoPath);
            p.start("git rev-parse", { "--show-superproject-working-tree" });
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
      {
         QLog_Info("UI", "Trying to open a directory that is not a Git repository.");
         QMessageBox::information(
             this, tr("Not a Git repository"),
             tr("The selected path is not a Git repository. Please make sure you opened the correct directory."));
      }
   }
   else
      QLog_Warning("UI", QString("Repository at {%1} already opened. Skip adding it again.").arg(repoPath));
}

void GitQlient::closeTab(int tabIndex)
{
   auto repoToRemove = dynamic_cast<GitQlientRepo *>(mRepos->widget(tabIndex));

   QLog_Info("UI", QString("Removing repository {%1}").arg(repoToRemove->currentDir()));

   mCurrentRepos.remove(repoToRemove->currentDir());
   // mRepos->removeTab(tabIndex);
   repoToRemove->close();
}

void GitQlient::restorePinnedRepos()
{
   GitQlientSettings settings;
   const auto pinnedRepos = settings.globalValue(GitQlientSettings::PinnedRepos, QStringList()).toStringList();

   for (auto &repo : pinnedRepos)
      addNewRepoTab(repo, true);
}

void GitQlient::onSuccessOpen(const QString &fullPath)
{
   GitQlientSettings settings;
   settings.setProjectOpened(fullPath);

   mConfigWidget->onRepoOpened();
}

void GitQlient::conditionallyOpenPreConfigDlg(const QString &repoPath)
{
   QSharedPointer<GitBase> git(new GitBase(repoPath));

   GitQlientSettings settings;
   auto maxCommits = settings.localValue(git->getGitQlientSettingsDir(), "MaxCommits", -1).toInt();

   if (maxCommits == -1)
   {
      const auto preConfig = new InitialRepoConfig(git, this);
      preConfig->exec();
   }
}
