#include "CreatePullRequestDlg.h"
#include "ui_CreatePullRequestDlg.h"

#include <GitHubRestApi.h>
#include <GitQlientSettings.h>
#include <GitBase.h>
#include <GitConfig.h>
#include <ServerPullRequest.h>
#include <RevisionsCache.h>

#include <QMessageBox>

CreatePullRequestDlg::CreatePullRequestDlg(const QSharedPointer<RevisionsCache> &cache,
                                           const QSharedPointer<GitBase> &git, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::CreatePullRequestDlg)
   , mCache(cache)
   , mGit(git)
{
   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
   const auto gameServerUrl = gitConfig->getServerUrl();

   if (gameServerUrl.contains("github"))
   {
      GitQlientSettings settings;
      mUserName = settings.value(QString("%1/user").arg(gameServerUrl)).toString();
      const auto userToken = settings.value(QString("%1/token").arg(gameServerUrl)).toString();
      const auto repoInfo = gitConfig->getCurrentRepoAndOwner();

      mApi = new GitHubRestApi(repoInfo.first, repoInfo.second, { mUserName, userToken });
      connect(mApi, &GitHubRestApi::signalIssueCreated, this, &CreatePullRequestDlg::onPullRequestCreated);

      mApi->getMilestones();
      mApi->requestLabels();
   }

   ui->setupUi(this);

   const auto branches = mCache->getBranches(References::Type::RemoteBranches);

   for (auto &value : branches)
   {
      ui->cbOrigin->addItems(value.second);
      ui->cbDestination->addItems(value.second);
   }

   ui->cbOrigin->setCurrentText(mGit->getCurrentBranch());

   connect(ui->pbCreate, &QPushButton::clicked, this, &CreatePullRequestDlg::accept);
   connect(ui->pbClose, &QPushButton::clicked, this, &CreatePullRequestDlg::reject);
}

CreatePullRequestDlg::~CreatePullRequestDlg()
{
   delete ui;
}

void CreatePullRequestDlg::accept()
{
   if (ui->leTitle->text().isEmpty() || ui->teDescription->toPlainText().isEmpty())
      QMessageBox::warning(this, tr("Empty fields"), tr("Please, complete all fields with valid data."));
   else if (ui->cbOrigin->currentText() == ui->cbDestination->currentText())
   {
      QMessageBox::warning(
          this, tr("Error in the branch selection"),
          tr("The base branch and the branch to merge from cannot be the same. Please, select different branches."));
   }

   mApi->createPullRequest({ ui->leTitle->text(), ui->teDescription->toPlainText().toUtf8(),
                             ui->cbDestination->currentText(), ui->cbOrigin->currentText(), true,
                             ui->chModify->isChecked(), ui->chDraft->isChecked() });
}

void CreatePullRequestDlg::onPullRequestCreated(const QString &url)
{
   auto finalUrl = url;
   finalUrl = finalUrl.remove("api.").remove("repos/");

   QMessageBox::information(
       this, tr("Pull Request created"),
       tr("The Pull Request has been created. You can <a href=\"%1\">find it here</a>.").arg(finalUrl));

   QDialog::accept();
}
