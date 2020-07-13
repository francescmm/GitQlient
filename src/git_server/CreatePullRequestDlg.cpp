#include "CreatePullRequestDlg.h"
#include "ui_CreatePullRequestDlg.h"

#include <GitHubRestApi.h>
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

   if (serverUrl.contains("github"))
   {
      GitQlientSettings settings;
      mUserName = settings.value(QString("%1/user").arg(serverUrl)).toString();
      const auto userToken = settings.value(QString("%1/token").arg(serverUrl)).toString();
      const auto repoInfo = gitConfig->getCurrentRepoAndOwner();
      const auto endpoint = settings.value(QString("%1/endpoint").arg(serverUrl)).toString();

      mApi = new GitHubRestApi(repoInfo.first, repoInfo.second, { mUserName, userToken, endpoint });
      connect(mApi, &GitHubRestApi::signalIssueUpdated, this, &CreatePullRequestDlg::onPullRequestUpdated);
      connect(mApi, &GitHubRestApi::signalPullRequestCreated, this, &CreatePullRequestDlg::onPullRequestCreated);
      connect(mApi, &GitHubRestApi::signalMilestonesReceived, this, &CreatePullRequestDlg::onMilestones);
      connect(mApi, &GitHubRestApi::signalLabelsReceived, this, &CreatePullRequestDlg::onLabels);

      mApi->getMilestones();
      mApi->requestLabels();
   }

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
      ui->teDescription->setText(f.readAll());
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

   mApi->createPullRequest(
       { ui->leTitle->text(),
         ui->teDescription->toPlainText().toUtf8(),
         mUserName + ":" + ui->cbOrigin->currentText().remove(0, ui->cbOrigin->currentText().indexOf("/") + 1),
         ui->cbDestination->currentText().remove(0, ui->cbDestination->currentText().indexOf("/") + 1),
         true,
         ui->chModify->isChecked(),
         ui->chDraft->isChecked(),
         0,
         "",
         {},
         {} });
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

void CreatePullRequestDlg::onPullRequestUpdated()
{
   QTimer::singleShot(200, [this]() {
      QMessageBox::information(
          this, tr("Pull Request created"),
          tr("The Pull Request has been created. You can <a href=\"%1\">find it here</a>.").arg(mFinalUrl));

      QDialog::accept();
   });
}
