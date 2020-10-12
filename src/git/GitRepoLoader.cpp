#include "GitRepoLoader.h"

#include <GitBase.h>
#include <GitConfig.h>
#include <GitCache.h>
#include <GitRequestorProcess.h>
#include <GitBranches.h>
#include <GitQlientSettings.h>
#include <GitHubRestApi.h>

#include <QLogger.h>

#include <QDir>

using namespace QLogger;

static const char *GIT_LOG_FORMAT("%m%HX%P%n%cn<%ce>%n%an<%ae>%n%at%n%s%n%b ");

GitRepoLoader::GitRepoLoader(QSharedPointer<GitBase> gitBase, QSharedPointer<GitCache> cache, QObject *parent)
   : QObject(parent)
   , mGitBase(gitBase)
   , mRevCache(std::move(cache))
{
}

bool GitRepoLoader::loadRepository()
{
   if (mLocked)
      QLog_Warning("Git", "Git is currently loading data.");
   else
   {
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
            auto localBranches = false;
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
               localBranches = true;
            }
            else if (refName.startsWith("refs/remotes/") && !refName.endsWith("HEAD"))
            {
               type = References::Type::RemoteBranches;
               name = refName.mid(13);
            }
            else
               continue;

            mRevCache->insertReference(revSha, type, name);

            if (localBranches)
            {
               const auto git = new GitBranches(mGitBase);
               GitCache::LocalBranchDistances distances;

               const auto distToMaster = git->getDistanceBetweenBranches(true, name);
               auto toMaster = distToMaster.output.toString();

               if (!toMaster.contains("fatal"))
               {
                  toMaster.replace('\n', "");
                  const auto values = toMaster.split('\t');
                  distances.behindMaster = values.first().toUInt();
                  distances.aheadMaster = values.last().toUInt();
               }

               const auto distToOrigin = git->getDistanceBetweenBranches(false, name);
               auto toOrigin = distToOrigin.output.toString();

               if (!toOrigin.contains("fatal"))
               {
                  toOrigin.replace('\n', "");
                  const auto values = toOrigin.split('\t');
                  distances.behindOrigin = values.first().toUInt();
                  distances.aheadOrigin = values.last().toUInt();
               }

               mRevCache->insertLocalBranchDistances(name, distances);
            }
         }

         prevRefSha = revSha;
      }
   }
}

void GitRepoLoader::requestRevisions()
{
   QLog_Debug("Git", "Loading revisions.");

   GitQlientSettings settings;
   const auto maxCommits = settings.localValue(mGitBase->getGitQlientSettingsDir(), "MaxCommits", 0).toInt();
   const auto commitsToRetrieve = maxCommits != 0 ? QString::fromUtf8("-n %1").arg(maxCommits)
                                                  : mShowAll ? QString("--all") : mGitBase->getCurrentBranch();

   const auto baseCmd = QString("git log --date-order --no-color --log-size --parents --boundary -z --pretty=format:")
                            .append(QString::fromUtf8(GIT_LOG_FORMAT))
                            .append(commitsToRetrieve);

   emit signalLoadingStarted(1);

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
   {
      QLog_Info("Git", "Requesting PR status!");
      const auto repoInfo = gitConfig->getCurrentRepoAndOwner();

      emit signalRefreshPRsCache(repoInfo.first, repoInfo.second, serverUrl);
   }

   QLog_Debug("Git", "Processing revisions...");

   emit signalLoadingStarted(1);

   const auto ret = gitConfig->getGitValue("log.showSignature");
   const auto showSignature = ret.success ? ret.output.toString().contains("true") : false;
   const auto commits = showSignature ? processSignedLog(ba) : processUnsignedLog(ba);

   const auto wipInfo = processWip();
   mRevCache->setup(wipInfo, commits);

   loadReferences();

   mRevCache->setConfigurationDone();

   mLocked = false;

   emit signalLoadingFinished();
}

WipRevisionInfo GitRepoLoader::processWip()
{
   QLog_Debug("Git", QString("Executing processWip."));

   mRevCache->setUntrackedFilesList(getUntrackedFiles());

   const auto ret = mGitBase->run("git rev-parse --revs-only HEAD");

   if (ret.success)
   {
      QString diffIndex;
      QString diffIndexCached;

      auto parentSha = ret.output.toString().trimmed();

      if (parentSha.isEmpty())
         parentSha = CommitInfo::INIT_SHA;

      const auto ret3 = mGitBase->run(QString("git diff-index %1").arg(parentSha));
      diffIndex = ret3.success ? ret3.output.toString() : QString();

      const auto ret4 = mGitBase->run(QString("git diff-index --cached %1").arg(parentSha));
      diffIndexCached = ret4.success ? ret4.output.toString() : QString();

      return { parentSha, diffIndex, diffIndexCached };
   }

   return {};
}

void GitRepoLoader::updateWipRevision()
{
   if (const auto wipInfo = processWip(); wipInfo.isValid())
      mRevCache->updateWipCommit(wipInfo.parentSha, wipInfo.diffIndex, wipInfo.diffIndexCached);
}

QVector<QString> GitRepoLoader::getUntrackedFiles() const
{
   QLog_Debug("Git", QString("Executing getUntrackedFiles."));

   auto runCmd = QString("git ls-files --others");
   const auto exFile = QString(".git/info/exclude");
   const auto path = QString("%1/%2").arg(mGitBase->getWorkingDir(), exFile);

   if (QFile::exists(path))
      runCmd.append(QString(" --exclude-from=$%1$").arg(exFile));

   runCmd.append(QString(" --exclude-per-directory=$%1$").arg(".gitignore"));

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
   const auto ret = mGitBase->run(runCmd).output.toString().split('\n', Qt::SkipEmptyParts).toVector();
#else
   const auto ret = mGitBase->run(runCmd).output.toString().split('\n', QString::SkipEmptyParts).toVector();
#endif

   return ret;
}

QList<CommitInfo> GitRepoLoader::processUnsignedLog(QByteArray &log)
{
   QList<CommitInfo> commits;
   auto commitsLog = log.split('\000');

   for (auto &commitData : commitsLog)
   {
      if (auto commit = parseCommitData(commitData); commit.isValid())
         commits.append(std::move(commit));
   }

   return commits;
}

QList<CommitInfo> GitRepoLoader::processSignedLog(QByteArray &log) const
{
   auto preProcessedCommits = log.replace('\000', '\n').split('\n');
   QList<CommitInfo> commits;
   QByteArray commit;
   QByteArray gpg;
   QString gpgKey;
   auto processingCommit = false;

   for (auto line : preProcessedCommits)
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
            gpgKey = QString::fromUtf8(line).split("using RSA key", QString::SkipEmptyParts);
#endif
            gpgKey.append('\n');
         }
      }
      else if (line.startsWith("log size"))
      {
         if (!commit.isEmpty())
         {
            if (auto revision = parseCommitData(commit); revision.isValid())
               commits.append(std::move(revision));

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

CommitInfo GitRepoLoader::parseCommitData(QByteArray &commitData) const
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

      return CommitInfo { sha,    parentsSha, boundary, committer, commitDate,
                          author, shortLog,   longLog,  isSigned,  isSigned ? firstField : QString() };
   }

   return CommitInfo();
}
