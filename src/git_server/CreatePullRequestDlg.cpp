#include "CreatePullRequestDlg.h"
#include "ui_CreatePullRequestDlg.h"

#include <GitHubRestApi.h>
#include <GitLabRestApi.h>
#include <GitQlientSettings.h>
#include <GitBase.h>
#include <GitConfig.h>
#include <ServerPullRequest.h>
#include <RevisionsCache.h>
#include <ServerIssue.h>

#include <QStandardItemModel>
#include <QMessageBox>
#include <QTimer>
#include <QFile>

CreatePullRequestDlg::CreatePullRequestDlg(const QSharedPointer<RevisionsCache> &cache,
                                           const QSharedPointer<GitBase> &git, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::CreatePullRequestDlg)
   , mCache(cache)
   , mGit(git)
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
   else if (serverUrl.contains("gitlab"))
      mApi = new GitLabRestApi(mUserName, repoInfo.second, serverUrl, { mUserName, userToken, endpoint });

   connect(mApi, &IRestApi::issueUpdated, this, &CreatePullRequestDlg::onPullRequestUpdated);
   connect(mApi, &IRestApi::pullRequestCreated, this, &CreatePullRequestDlg::onPullRequestCreated);
   connect(mApi, &IRestApi::milestonesReceived, this, &CreatePullRequestDlg::onMilestones);
   connect(mApi, &IRestApi::labelsReceived, this, &CreatePullRequestDlg::onLabels);
   connect(mApi, &IRestApi::errorOccurred, this, &CreatePullRequestDlg::onGitServerError);

   mApi->requestMilestones();
   mApi->requestLabels();

   const auto branches = mCache->getBranches(References::Type::RemoteBranches);

   for (auto &value : branches)
   {
      ui->cbOrigin->addItems(value.second);
      ui->cbDestination->addItems(value.second);
   }

   const auto index = ui->cbOrigin->findText(mGit->getCurrentBranch(), Qt::MatchEndsWith);
   ui->cbOrigin->setCurrentIndex(index);

   connect(ui->pbCreate, &QPushButton::clicked, this, &CreatePullRequestDlg::accept);
   connect(ui->pbClose, &QPushButton::clicked, this, &CreatePullRequestDlg::reject);

   QFile f(mGit->getWorkingDir() + "/.github/PULL_REQUEST_TEMPLATE.md");

   if (f.open(QIODevice::ReadOnly))
   {
      ui->teDescription->setText(QString::fromUtf8(f.readAll()));
      f.close();
   }
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

   ServerPullRequest pr;
   pr.title = ui->leTitle->text(), pr.body = ui->teDescription->toPlainText().toUtf8();
   pr.head = mUserName + ":" + ui->cbOrigin->currentText().remove(0, ui->cbOrigin->currentText().indexOf("/") + 1);
   pr.base = ui->cbDestination->currentText().remove(0, ui->cbDestination->currentText().indexOf("/") + 1);
   pr.maintainerCanModify = ui->chModify->isChecked();
   pr.draft = ui->chDraft->isChecked();

   if (dynamic_cast<GitLabRestApi *>(mApi))
   {
      pr.head = ui->cbOrigin->currentText().remove(0, ui->cbOrigin->currentText().indexOf("/") + 1);

      QStringList labels;

      if (const auto cbModel = qobject_cast<QStandardItemModel *>(ui->labelsListView->model()))
      {
         for (auto i = 0; i < cbModel->rowCount(); ++i)
         {
            if (cbModel->item(i)->checkState() == Qt::Checked)
               labels.append(cbModel->item(i)->text());
         }
      }

      pr.labels = labels;
      pr.milestone = ui->cbMilesone->count() > 0 ? ui->cbMilesone->currentData().toInt() : -1;
   }
   else
      pr.head = mUserName + ":" + ui->cbOrigin->currentText().remove(0, ui->cbOrigin->currentText().indexOf("/") + 1);

   ui->pbCreate->setEnabled(false);

   mApi->createPullRequest(pr);
}

void CreatePullRequestDlg::onMilestones(const QVector<ServerMilestone> &milestones)
{
   ui->cbMilesone->addItem("Select milestone", -1);

   for (auto &milestone : milestones)
      ui->cbMilesone->addItem(milestone.title, milestone.number);

   ui->cbMilesone->setCurrentIndex(0);
}

void CreatePullRequestDlg::onLabels(const QVector<ServerLabel> &labels)
{
   const auto model = new QStandardItemModel(labels.count(), 0, this);
   auto count = 0;
   for (auto label : labels)
   {
      const auto item = new QStandardItem(label.name);
      item->setCheckable(true);
      item->setCheckState(Qt::Unchecked);
      model->setItem(count++, item);
   }
   ui->labelsListView->setModel(model);
}

void CreatePullRequestDlg::onPullRequestCreated(QString url)
{
   mFinalUrl = url;

   if (dynamic_cast<GitHubRestApi *>(mApi))
   {
      mIssue = mFinalUrl.mid(mFinalUrl.lastIndexOf("/") + 1, mFinalUrl.count() - 1).toInt();

      const auto milestone = ui->cbMilesone->count() > 0 ? ui->cbMilesone->currentData().toInt() : -1;

      QStringList labels;

      if (const auto cbModel = qobject_cast<QStandardItemModel *>(ui->labelsListView->model()))
      {
         for (auto i = 0; i < cbModel->rowCount(); ++i)
         {
            if (cbModel->item(i)->checkState() == Qt::Checked)
               labels.append(cbModel->item(i)->text());
         }
      }

      mApi->updateIssue(mIssue, { ui->leTitle->text(), "", milestone, labels, { mUserName } });
   }
   else
      onPullRequestUpdated();
}

void CreatePullRequestDlg::onPullRequestUpdated()
{
   QTimer::singleShot(200, [this]() {
      QMessageBox::information(
          this, tr("Pull Request created"),
          tr("The Pull Request has been created. You can <a href=\"%1\">find it here</a>.").arg(mFinalUrl));

      emit signalRefreshPRsCache();

      QDialog::accept();
   });
}

void CreatePullRequestDlg::onGitServerError(const QString &error)
{
   ui->pbCreate->setEnabled(true);

   QMessageBox::warning(this, tr("API access error!"), error);
}
