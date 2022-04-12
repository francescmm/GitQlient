#include "MergePullRequestDlg.h"
#include "ui_MergePullRequestDlg.h"

#include <GitHubRestApi.h>
#include <GitLabRestApi.h>

#include <QJsonDocument>
#include <QMessageBox>

using namespace GitServer;

MergePullRequestDlg::MergePullRequestDlg(const QSharedPointer<GitBase> git, GitServer::ConfigData data,
                                         const PullRequest &pr, const QString &sha, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::MergePullRequestDlg)
   , mGit(git)
   , mPr(pr)
   , mSha(sha)
{
   ui->setupUi(this);

   if (data.serverUrl.contains("github"))
      mApi = new GitHubRestApi(data.repoOwner, data.repoName, { data.user, data.token, data.endPoint });
   else
      mApi = new GitLabRestApi(data.user, data.repoName, data.serverUrl, { data.user, data.token, data.endPoint });

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

   emit pullRequestMerged();

   QDialog::accept();
}

void MergePullRequestDlg::onGitServerError(const QString &error)
{
   ui->pbMerge->setEnabled(true);

   QMessageBox::warning(this, tr("API access error!"), error);
}
