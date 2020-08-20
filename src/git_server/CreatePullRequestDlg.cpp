#include "CreatePullRequestDlg.h"
#include "ui_CreatePullRequestDlg.h"

#include <GitHubRestApi.h>
#include <GitLabRestApi.h>
#include <GitQlientSettings.h>
#include <GitServerCache.h>
#include <PullRequest.h>
#include <GitCache.h>
#include <Issue.h>
#include <Milestone.h>
#include <Label.h>

#include <QStandardItemModel>
#include <QMessageBox>
#include <QTimer>
#include <QFile>

using namespace GitServer;

CreatePullRequestDlg::CreatePullRequestDlg(const QSharedPointer<GitCache> &cache,
                                           const QSharedPointer<GitServerCache> &gitServerCache,
                                           const QString &workingDir, const QString &currentBranch, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::CreatePullRequestDlg)
   , mCache(cache)
   , mGitServerCache(gitServerCache)
{
   setAttribute(Qt::WA_DeleteOnClose);

   ui->setupUi(this);

   connect(mGitServerCache->getApi(), &IRestApi::pullRequestUpdated, this, &CreatePullRequestDlg::onPullRequestUpdated);
   connect(mGitServerCache.get(), &GitServerCache::errorOccurred, this, &CreatePullRequestDlg::onGitServerError);

   onMilestones(mGitServerCache->getMilestones());
   onLabels(mGitServerCache->getLabels());

   const auto branches = mCache->getBranches(References::Type::RemoteBranches);

   for (auto &value : branches)
   {
      ui->cbOrigin->addItems(value.second);
      ui->cbDestination->addItems(value.second);
   }

   const auto index = ui->cbOrigin->findText(currentBranch, Qt::MatchEndsWith);
   ui->cbOrigin->setCurrentIndex(index);

   connect(ui->pbCreate, &QPushButton::clicked, this, &CreatePullRequestDlg::accept);
   connect(ui->pbClose, &QPushButton::clicked, this, &CreatePullRequestDlg::reject);

   QFile f(workingDir + "/.github/PULL_REQUEST_TEMPLATE.md");

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

   PullRequest pr;
   pr.title = ui->leTitle->text(), pr.body = ui->teDescription->toPlainText().toUtf8();
   pr.head = mGitServerCache->getUserName() + ":"
       + ui->cbOrigin->currentText().remove(0, ui->cbOrigin->currentText().indexOf("/") + 1);
   pr.base = ui->cbDestination->currentText().remove(0, ui->cbDestination->currentText().indexOf("/") + 1);
   pr.maintainerCanModify = ui->chModify->isChecked();
   pr.draft = ui->chDraft->isChecked();

   if (mGitServerCache->getPlatform() == Platform::GitLab)
   {
      pr.head = ui->cbOrigin->currentText().remove(0, ui->cbOrigin->currentText().indexOf("/") + 1);

      QVector<Label> labels;

      if (const auto cbModel = qobject_cast<QStandardItemModel *>(ui->labelsListView->model()))
      {
         for (auto i = 0; i < cbModel->rowCount(); ++i)
         {
            if (cbModel->item(i)->checkState() == Qt::Checked)
            {
               Label sLabel;
               sLabel.name = cbModel->item(i)->text();
               labels.append(sLabel);
            }
         }
      }

      pr.labels = labels;

      Milestone milestone;
      milestone.id = ui->cbMilesone->count() > 0 ? ui->cbMilesone->currentData().toInt() : -1;
      pr.milestone = milestone;

      GitServer::User sAssignee;
      sAssignee.name = mGitServerCache->getUserName();

      pr.assignees.append(sAssignee);
   }
   else
      pr.head = mGitServerCache->getUserName() + ":"
          + ui->cbOrigin->currentText().remove(0, ui->cbOrigin->currentText().indexOf("/") + 1);

   ui->pbCreate->setEnabled(false);

   mGitServerCache->getApi()->createPullRequest(pr);
}

void CreatePullRequestDlg::onMilestones(const QVector<Milestone> &milestones)
{
   ui->cbMilesone->addItem("Select milestone", -1);

   for (auto &milestone : milestones)
      ui->cbMilesone->addItem(milestone.title, milestone.number);

   ui->cbMilesone->setCurrentIndex(0);
}

void CreatePullRequestDlg::onLabels(const QVector<Label> &labels)
{
   const auto model = new QStandardItemModel(labels.count(), 0, this);
   auto count = 0;
   for (const auto &label : labels)
   {
      const auto item = new QStandardItem(label.name);
      item->setCheckable(true);
      item->setCheckState(Qt::Unchecked);
      model->setItem(count++, item);
   }
   ui->labelsListView->setModel(model);
}

void CreatePullRequestDlg::onPullRequestUpdated(const PullRequest &pr)
{
   QTimer::singleShot(200, this, [this, pr]() {
      QMessageBox::information(
          this, tr("Pull Request created"),
          tr("The Pull Request has been created. You can <a href=\"%1\">find it here</a>.").arg(pr.url));

      emit signalRefreshPRsCache();

      QDialog::accept();
   });
}

void CreatePullRequestDlg::onGitServerError(const QString &error)
{
   ui->pbCreate->setEnabled(true);

   QMessageBox::warning(this, tr("API access error!"), error);
}
