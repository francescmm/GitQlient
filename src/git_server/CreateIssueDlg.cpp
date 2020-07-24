#include "CreateIssueDlg.h"
#include "ui_CreateIssueDlg.h"
#include <GitHubRestApi.h>
#include <GitLabRestApi.h>
#include <GitQlientSettings.h>
#include <GitConfig.h>
#include <ServerIssue.h>

#include <QMessageBox>
#include <QStandardItemModel>
#include <QStandardItem>

CreateIssueDlg::CreateIssueDlg(const QSharedPointer<GitBase> git, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::CreateIssueDlg)
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
   else
   {
      mApi = new GitLabRestApi(mUserName, repoInfo.second, serverUrl, { mUserName, userToken, endpoint });
      mUserName = dynamic_cast<GitLabRestApi *>(mApi)->getUserId();
   }

   connect(mApi, &IRestApi::issueCreated, this, &CreateIssueDlg::onIssueCreated);
   connect(mApi, &IRestApi::milestonesReceived, this, &CreateIssueDlg::onMilestones);
   connect(mApi, &IRestApi::labelsReceived, this, &CreateIssueDlg::onLabels);
   connect(mApi, &IRestApi::errorOccurred, this, &CreateIssueDlg::onGitServerError);

   mApi->requestMilestones();
   mApi->requestLabels();

   connect(ui->pbAccept, &QPushButton::clicked, this, &CreateIssueDlg::accept);
   connect(ui->pbClose, &QPushButton::clicked, this, &CreateIssueDlg::reject);
}

CreateIssueDlg::~CreateIssueDlg()
{
   delete ui;
}

void CreateIssueDlg::accept()
{
   if (ui->leTitle->text().isEmpty() || ui->teDescription->toPlainText().isEmpty())
      QMessageBox::warning(this, tr("Empty fields"), tr("Please, complete all fields with valid data."));
   else
   {
      QStringList labels;

      if (const auto cbModel = qobject_cast<QStandardItemModel *>(ui->labelsListView->model()))
      {
         for (auto i = 0; i < cbModel->rowCount(); ++i)
         {
            if (cbModel->item(i)->checkState() == Qt::Checked)
               labels.append(cbModel->item(i)->text());
         }
      }

      const auto milestone = ui->cbMilesone->count() > 0 ? ui->cbMilesone->currentData().toInt() : -1;

      ui->pbAccept->setEnabled(false);

      mApi->createIssue(
          { ui->leTitle->text(), ui->teDescription->toPlainText().toUtf8(), milestone, labels, { mUserName } });
   }
}

void CreateIssueDlg::onMilestones(const QVector<ServerMilestone> &milestones)
{
   ui->cbMilesone->addItem("Select milestone", -1);

   for (auto &milestone : milestones)
      ui->cbMilesone->addItem(milestone.title, milestone.number);

   ui->cbMilesone->setCurrentIndex(0);
}

void CreateIssueDlg::onLabels(const QVector<ServerLabel> &labels)
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

void CreateIssueDlg::onIssueCreated(QString url)
{
   QMessageBox::information(this, tr("Issue created"),
                            tr("The issue has been created. You can <a href=\"%1\">find it here</a>.").arg(url));

   QDialog::accept();
}

void CreateIssueDlg::onGitServerError(const QString &error)
{
   ui->pbAccept->setEnabled(true);

   QMessageBox::warning(this, tr("API access error!"), error);
}
