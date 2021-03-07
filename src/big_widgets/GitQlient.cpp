#include "GitQlient.h"

#include <InitScreen.h>
#include <GitQlientStyles.h>
#include <GitQlientSettings.h>
#include <QPinnableTabWidget.h>
#include <InitialRepoConfig.h>

#include <QCommandLineParser>
#include <QProcess>
#include <QTabBar>
#include <GitQlientRepo.h>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <GitBase.h>

#include <QLogger.h>

using namespace QLogger;

GitQlient::GitQlient(QWidget *parent)
   : QWidget(parent)
   , mRepos(new QPinnableTabWidget())
   , mConfigWidget(new InitScreen())
{
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

   mRepos->setObjectName("GitQlientTab");
   mRepos->setStyleSheet(GitQlientStyles::getStyles());
   mRepos->setCornerWidget(addTab, Qt::TopRightCorner);
   connect(mRepos, &QTabWidget::tabCloseRequested, this, &GitQlient::closeTab);

   const auto vLayout = new QVBoxLayout(this);
   vLayout->setContentsMargins(QMargins());
   vLayout->addWidget(mRepos);
   mRepos->addPinnedTab(mConfigWidget, QIcon(":/icons/home"), QString());

   mConfigWidget->onRepoOpened();

   connect(mConfigWidget, &InitScreen::signalOpenRepo, this, &GitQlient::addRepoTab);

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

bool GitQlient::setArgumentsPostInit(const QStringList &arguments)
{
   QLog_Info("UI", QString("External call with the params {%1}").arg(arguments.join(",")));

   QStringList repos;
   bool ret = parseArguments(arguments, &repos);
   if (ret) {
      setRepositories(repos);
   }
   return ret;
}

bool GitQlient::parseArguments(const QStringList &arguments, QStringList *repos)
{
   bool ret = true;
   GitQlientSettings settings;
#ifdef DEBUG
   LogLevel logLevel = LogLevel::Trace;
#else
   LogLevel logLevel = static_cast<LogLevel>(settings.globalValue("logsLevel", static_cast<int>(LogLevel::Warning)).toInt());
#endif
   bool areLogsDisabled = settings.globalValue("logsDisabled", true).toBool();

   QCommandLineParser parser;
   parser.setApplicationDescription(tr("Graphical Git client"));
   parser.addPositionalArgument("repos", tr("Git repositories to open"), tr("[repos...]"));

   const QCommandLineOption helpOption = parser.addHelpOption();
   // We don't use parser.addVersionOption() because then it is handled by Qt and we want to show Git SHA also
   const QCommandLineOption versionOption(QStringList() << "v" << "version", tr("Displays version information."));
   parser.addOption(versionOption);

   const QCommandLineOption noLogOption("no-log", tr("Disables logs."));
   parser.addOption(noLogOption);

   const QCommandLineOption logLevelOption("log-level", tr("Sets log level."), tr("level"));
   parser.addOption(logLevelOption);

   parser.process(arguments);

   *repos = parser.positionalArguments();
   if (parser.isSet(noLogOption))
      areLogsDisabled = true;

   if (!areLogsDisabled)
   {
      if (parser.isSet(logLevelOption))
      {
         LogLevel level = static_cast<LogLevel>(parser.value(logLevelOption).toInt());
         if (level >= QLogger::LogLevel::Trace && level <= QLogger::LogLevel::Fatal)
         {
            logLevel = level;
            settings.setGlobalValue("logsLevel", static_cast<int>(level));
         }
      }

      QLoggerManager::getInstance()->overwriteLogLevel(logLevel);
      QLog_Info("UI", QString("Getting arguments {%1}").arg(arguments.join(", ")));
   }
   else
      QLoggerManager::getInstance()->pause();

   if (parser.isSet(versionOption))
   {
      QTextStream out(stdout);
      out << QCoreApplication::applicationName() << ' ' << tr("version") << ' ' << QCoreApplication::applicationVersion()
          << " (" << tr("Git SHA ") << SHA_VER << ")\n";
      ret = false;
   }
   if (parser.isSet(helpOption))
      ret = false;

   const auto manager = QLoggerManager::getInstance();
   manager->addDestination("GitQlient.log", { "UI", "Git", "Cache" }, logLevel);

   return ret;
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
