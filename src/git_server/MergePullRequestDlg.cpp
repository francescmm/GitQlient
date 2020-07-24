#include "MergePullRequestDlg.h"
#include "ui_MergePullRequestDlg.h"
#include <GitConfig.h>
#include <GitQlientSettings.h>
#include <GitHubRestApi.h>
#include <GitLabRestApi.h>
#include <GitRemote.h>

#include <QMessageBox>
#include <QJsonDocument>

MergePullRequestDlg::MergePullRequestDlg(const QSharedPointer<GitBase> git, const ServerPullRequest &pr,
                                         const QString &sha, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::MergePullRequestDlg)
   , mGit(git)
   , mPr(pr)
   , mSha(sha)
{
   ui->setupUi(this);

   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
   const auto serverUrl = gitConfig->getServerUrl();

   GitQlientSettings settings;
   mUserName = settings.globalValue(QString("%1/user").arg(serverUrl)).toString();
   const auto userToken = settings.globalValue(QString("%1/token").arg(serverUrl)).toString();
   const auto repoInfo = gitConfig->getCurrentRepoAndOwner();
   const auto endpoint = settings.globalValue(QString("%1/endpoint").arg(serverUrl)).toString();

   if (serverUrl.contains("github"))
      mApi = new GitHubRestApi(repoInfo.first, repoInfo.second, { mUserName, userToken, endpoint });
   else
   {
      mApi = new GitLabRestApi(mUserName, repoInfo.second, serverUrl, { mUserName, userToken, endpoint });
      mUserName = dynamic_cast<GitLabRestApi *>(mApi)->getUserId();
   }

   connect(mApi, &GitHubRestApi::pullRequestMerged, this, &MergePullRequestDlg::onPRMerged);
   connect(mApi, &IRestApi::errorOccurred, this, &MergePullRequestDlg::onGitServerError);

   connect(ui->pbMerge, &QPushButton::clicked, this, &MergePullRequestDlg::accept);
   connect(ui->pbCancel, &QPushButton::clicked, this, &MergePullRequestDlg::reject);
}

MergePullRequestDlg::~MergePullRequestDlg()
{
   delete ui;
}

void MergePullRequestDlg::accept()
{
   if (ui->leTitle->text().isEmpty() || ui->leMessage->text().isEmpty())
      QMessageBox::warning(this, tr("Empty fields"), tr("Please, complete all fields with valid data."));
   else
   {
      ui->pbMerge->setEnabled(false);

      QJsonObject object;
      object.insert("commit_title", ui->leTitle->text());
      object.insert("commit_message", ui->leMessage->text());
      object.insert("sha", mSha);
      object.insert("merge_method", "merge");
      QJsonDocument doc(object);
      const auto data = doc.toJson(QJsonDocument::Compact);

      mApi->mergePullRequest(mPr.id, data);
   }
}

void MergePullRequestDlg::onPRMerged()
{
   QMessageBox::information(this, tr("PR merged!"), tr("The pull request has been merged."));

   QScopedPointer<GitRemote> git(new GitRemote(mGit));

   if (auto ret = git->pull(); ret.success)
   {
      git->prune();
      emit signalRepositoryUpdated();
   }

   QDialog::accept();
}

void MergePullRequestDlg::onGitServerError(const QString &error)
{
   ui->pbMerge->setEnabled(true);

   QMessageBox::warning(this, tr("API access error!"), error);
}
