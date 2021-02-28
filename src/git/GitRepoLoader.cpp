#include "GitRepoLoader.h"

#include <GitBase.h>
#include <GitBranches.h>
#include <GitCache.h>
#include <GitConfig.h>
#include <GitLocal.h>
#include <GitQlientSettings.h>
#include <GitRequestorProcess.h>
#include <GitWip.h>

#include <QLogger.h>

#include <QDir>

using namespace QLogger;

static const char *GIT_LOG_FORMAT("%m%HX%P%n%cn<%ce>%n%an<%ae>%n%at%n%s%n%b ");

GitRepoLoader::GitRepoLoader(QSharedPointer<GitBase> gitBase, QSharedPointer<GitCache> cache,
                             const QSharedPointer<GitQlientSettings> &settings, QObject *parent)
   : QObject(parent)
   , mGitBase(gitBase)
   , mRevCache(std::move(cache))
   , mSettings(settings)
{
}

void GitRepoLoader::cancelAll()
{
   emit cancelAllProcesses(QPrivateSignal());
}

bool GitRepoLoader::load()
{
   return load(true);
}

bool GitRepoLoader::load(bool refreshReferences)
{
   if (mLocked)
      QLog_Warning("Git", "Git is currently loading data.");
   else
   {
      mRefreshReferences = refreshReferences;

      if (mGitBase->getWorkingDir().isEmpty())
         QLog_Error("Git", "No working directory set.");
      else
      {
         QLog_Info("Git", "Initializing Git...");

         mLocked = true;

         if (configureRepoDirectory())
         {
            mGitBase->updateCurrentBranch();

            QLog_Info("Git", "... Git initialization finished.");

            QLog_Info("Git", "Requesting revisions...");

            requestRevisions();

            return true;
         }
         else
            QLog_Error("Git", "The working directory is not a Git repository.");
      }
   }

   return false;
}

bool GitRepoLoader::configureRepoDirectory()
{
   QLog_Debug("Git", "Configuring repository directory.");

   const auto ret = mGitBase->run("git rev-parse --show-cdup");

   if (ret.success)
   {
      QDir d(QString("%1/%2").arg(mGitBase->getWorkingDir(), ret.output.toString().trimmed()));
      mGitBase->setWorkingDir(d.absolutePath());

      return true;
   }

   return false;
}

void GitRepoLoader::loadReferences()
{
   QLog_Debug("Git", "Loading references.");

   const auto ret3 = mGitBase->run("git show-ref -d");

   if (ret3.success)
   {
      auto ret = mGitBase->getLastCommit();

      if (ret.success)
         ret.output = ret.output.toString().trimmed();

      QString prevRefSha;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
      const auto referencesList = ret3.output.toString().split('\n', Qt::SkipEmptyParts);
#else
      const auto referencesList = ret3.output.toString().split('\n', QString::SkipEmptyParts);
#endif

      for (const auto &reference : referencesList)
      {
         const auto revSha = reference.left(40);
         const auto refName = reference.mid(41);

         if (!refName.startsWith("refs/tags/") || (refName.startsWith("refs/tags/") && refName.endsWith("^{}")))
         {
            References::Type type;
            QString name;

            if (refName.startsWith("refs/tags/"))
            {
               type = References::Type::LocalTag;
               name = refName.mid(10, reference.length());
               name.remove("^{}");
            }
            else if (refName.startsWith("refs/heads/"))
            {
               type = References::Type::LocalBranch;
               name = refName.mid(11);
            }
            else if (refName.startsWith("refs/remotes/") && !refName.endsWith("HEAD"))
            {
               type = References::Type::RemoteBranches;
               name = refName.mid(13);
            }
            else
               continue;

            mRevCache->insertReference(revSha, type, name);
         }

         prevRefSha = revSha;
      }
   }
}

void GitRepoLoader::requestRevisions()
{
   QLog_Debug("Git", "Loading revisions.");

   const auto maxCommits = mSettings->localValue("MaxCommits", 0).toInt();
   const auto commitsToRetrieve = maxCommits != 0 ? QString::fromUtf8("-n %1").arg(maxCommits)
                                                  : mShowAll ? QString("--all") : mGitBase->getCurrentBranch();

   QString order;

   switch (mSettings->localValue("GraphSortingOrder", 0).toInt())
   {
      case 0:
         order = "--author-date-order";
         break;
      case 1:
         order = "--date-order";
         break;
      case 2:
         order = "--topo-order";
         break;
      default:
         order = "--author-date-order";
         break;
   }

   const auto baseCmd = QString("git log %1 --no-color --log-size --parents --boundary -z --pretty=format:%2 %3")
                            .arg(order, QString::fromUtf8(GIT_LOG_FORMAT), commitsToRetrieve);

   emit signalLoadingStarted();

   const auto requestor = new GitRequestorProcess(mGitBase->getWorkingDir());
   connect(requestor, &GitRequestorProcess::procDataReady, this, &GitRepoLoader::processRevision);
   connect(this, &GitRepoLoader::cancelAllProcesses, requestor, &AGitProcess::onCancel);

   requestor->run(baseCmd);
}

void GitRepoLoader::processRevision(QByteArray ba)
{
   QLog_Info("Git", "Revisions received!");

   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGitBase));
   const auto serverUrl = gitConfig->getServerUrl();

   if (serverUrl.contains("github"))
      QLog_Info("Git", "Requesting PR status!");

   QLog_Debug("Git", "Processing revisions...");

   emit signalLoadingStarted();

   QList<QPair<QString, QString>> subtrees;
   const auto ret = gitConfig->getGitValue("log.showSignature");
   const auto showSignature = ret.success ? ret.output.toString().contains("true") : false;
   const auto commits = showSignature ? processSignedLog(ba, subtrees) : processUnsignedLog(ba, subtrees);

   QScopedPointer<GitWip> git(new GitWip(mGitBase, mRevCache));
   git->updateUntrackedFiles();

   mRevCache->setup(git->getWipInfo(), commits);

   if (!subtrees.isEmpty())
      mRevCache->addSubtrees(subtrees);

   if (mRefreshReferences)
   {
      mRevCache->clearReferences();
      loadReferences();
   }
   else
      mRevCache->reloadCurrentBranchInfo(mGitBase->getCurrentBranch(),
                                         mGitBase->getLastCommit().output.toString().trimmed());

   mRevCache->setConfigurationDone();

   emit signalLoadingFinished(mRefreshReferences);

   mLocked = false;
   mRefreshReferences = false;
}

QList<CommitInfo> GitRepoLoader::processUnsignedLog(QByteArray &log, QList<QPair<QString, QString>> &subtrees)
{
   QList<CommitInfo> commits;
   auto commitsLog = log.split('\000');

   for (auto &commitData : commitsLog)
   {
      bool subtree = false;
      if (auto commit = parseCommitData(commitData, subtree); commit.isValid())
      {
         commits.append(std::move(commit));

         if (subtree)
         {
            auto fields = commit.longLog().trimmed().split("\n");
            subtrees.append(qMakePair(fields.first().remove("git-subtree-dir:").trimmed(),
                                      fields.last().remove("git-subtree-split:").trimmed()));
         }
      }
   }

   return commits;
}

QList<CommitInfo> GitRepoLoader::processSignedLog(QByteArray &log, QList<QPair<QString, QString>> &subtrees) const
{
   auto preProcessedCommits = log.replace('\000', '\n').split('\n');
   QList<CommitInfo> commits;
   QByteArray commit;
   QByteArray gpg;
   QString gpgKey;
   auto processingCommit = false;

   for (const auto &line : preProcessedCommits)
   {
      if (line.startsWith("gpg: "))
      {
         processingCommit = false;
         gpg.append(line);

         if (line.contains("using RSA key"))
         {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            gpgKey = QString::fromUtf8(line).split("using RSA key", Qt::SkipEmptyParts).last();
#else
            gpgKey = QString::fromUtf8(line).split("using RSA key", QString::SkipEmptyParts).last();
#endif
            gpgKey.append('\n');
         }
      }
      else if (line.startsWith("log size"))
      {
         if (!commit.isEmpty())
         {
            bool subtree = false;
            if (auto revision = parseCommitData(commit, subtree); revision.isValid())
            {
               commits.append(std::move(revision));

               if (subtree)
               {
                  auto fields = revision.longLog().trimmed().split("\n");
                  subtrees.append(qMakePair(fields.first().remove("git-subtree-dir:").trimmed(),
                                            fields.last().remove("git-subtree-split:").trimmed()));
               }
            }

            commit.clear();
         }
         processingCommit = true;
         const auto isSigned = !gpg.isEmpty() && gpg.contains("Good signature");
         commit.append(isSigned ? gpgKey.toUtf8() : "\n");
         gpg.clear();
      }
      else if (processingCommit)
      {
         commit.append(line + '\n');
      }
   }

   return commits;
}

CommitInfo GitRepoLoader::parseCommitData(QByteArray &commitData, bool &isSubtree) const
{
   if (const auto fields = QString::fromUtf8(commitData).split('\n'); fields.count() > 6)
   {
      const auto firstField = fields.constFirst();
      const auto isSigned = !fields.first().isEmpty() && !firstField.contains("log size") ? true : false;
      auto combinedShas = fields.at(1);
      auto commitSha = combinedShas.split('X').first();
      const auto boundary = commitSha[0];
      const auto sha = commitSha.remove(0, 1);
      combinedShas = combinedShas.remove(0, sha.size() + 1 + 1).trimmed();
      QStringList parentsSha;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
      parentsSha = combinedShas.split(' ', Qt::SkipEmptyParts);
#else
      parentsSha = combinedShas.split(' ', QString::SkipEmptyParts);
#endif
      const auto committer = fields.at(2);
      const auto author = fields.at(3);
      const auto commitDate = QDateTime::fromSecsSinceEpoch(fields.at(4).toInt());
      const auto shortLog = fields.at(5);
      QString longLog;

      for (auto i = 6; i < fields.count(); ++i)
         longLog += fields.at(i) + '\n';

      longLog = longLog.trimmed();

      if (longLog.contains("git-subtree-dir") || shortLog.contains("git-subtree-dir"))
         isSubtree = true;

      return CommitInfo { sha,    parentsSha, boundary, committer, commitDate,
                          author, shortLog,   longLog,  isSigned,  isSigned ? firstField : QString() };
   }

   return CommitInfo();
}
