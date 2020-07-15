#include "ServerConfigDlg.h"
#include "ui_ServerConfigDlg.h"

#include <GitBase.h>
#include <GitConfig.h>
#include <GitQlientStyles.h>
#include <GitQlientSettings.h>
#include <GitHubRestApi.h>

#include <QLogger.h>

#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

#include <utility>

using namespace QLogger;

namespace
{
enum GitServerPlatform
{
   GitHub,
   GitHubEnterprise,
   GitLab,
   Bitbucket
};

static const QMap<GitServerPlatform, QString> repoUrls { { GitHub, "https://api.github.com/" },
                                                         { GitHubEnterprise, "" },
                                                         { GitLab, "https://gitlab.com/api/v4/" } };
}

ServerConfigDlg::ServerConfigDlg(const QSharedPointer<GitBase> &git, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::ServerConfigDlg)
   , mGit(git)
   , mManager(new QNetworkAccessManager())
{
   setStyleSheet(GitQlientStyles::getStyles());

   ui->setupUi(this);

   ui->leEndPoint->setHidden(true);

   ui->cbServer->addItem(tr("Select server endpoint"));
   ui->cbServer->insertItem(GitHub, "GitHub", repoUrls.value(GitHub));
   ui->cbServer->insertItem(GitHubEnterprise, "GitHub Enterprise", repoUrls.value(GitHubEnterprise));
   ui->cbServer->insertItem(GitLab, "GitLab", repoUrls.value(GitLab));

   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
   const auto serverUrl = gitConfig->getServerUrl();

   GitQlientSettings settings;
   ui->leUserName->setText(settings.value(QString("%1/user").arg(serverUrl)).toString());
   ui->leUserToken->setText(settings.value(QString("%1/token").arg(serverUrl)).toString());
   ui->leEndPoint->setText(
       settings.value(QString("%1/endpoint").arg(serverUrl), QString("https://api.github.com/")).toString());

   if (const auto index = repoUrls.key(ui->leEndPoint->text(), GitHubEnterprise))
      ui->cbServer->setCurrentIndex(index);

   connect(ui->leUserToken, &QLineEdit::editingFinished, this, &ServerConfigDlg::checkToken);
   connect(ui->leUserToken, &QLineEdit::returnPressed, this, &ServerConfigDlg::accept);
   connect(ui->pbAccept, &QPushButton::clicked, this, &ServerConfigDlg::accept);
   connect(ui->pbCancel, &QPushButton::clicked, this, &ServerConfigDlg::reject);
   connect(ui->pbTest, &QPushButton::clicked, this, &ServerConfigDlg::testToken);
   connect(ui->cbServer, &QComboBox::currentTextChanged, this, &ServerConfigDlg::onServerChanged);
}

ServerConfigDlg::~ServerConfigDlg()
{
   delete mManager;
   delete ui;
}

void ServerConfigDlg::checkToken()
{
   if (ui->leUserToken->text().isEmpty())
      ui->leUserName->setStyleSheet("border: 1px solid red;");
}

void ServerConfigDlg::accept()
{
   const auto endpoint = ui->cbServer->currentIndex() == GitHubEnterprise ? ui->leEndPoint->text()
                                                                          : ui->cbServer->currentData().toString();

   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
   const auto serverUrl = gitConfig->getServerUrl();

   GitQlientSettings settings;
   settings.setValue(QString("%1/user").arg(serverUrl), ui->leUserName->text());
   settings.setValue(QString("%1/token").arg(serverUrl), ui->leUserToken->text());
   settings.setValue(QString("%1/endpoint").arg(serverUrl), endpoint);

   QDialog::accept();
}

void ServerConfigDlg::testToken()
{
   if (ui->leUserToken->text().isEmpty())
      ui->leUserName->setStyleSheet("border: 1px solid red;");
   else
   {
      const auto endpoint = ui->cbServer->currentIndex() == GitHubEnterprise ? ui->leEndPoint->text()
                                                                             : ui->cbServer->currentData().toString();
      // TODO: Initialize depending on the server
      QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
      const auto serverUrl = gitConfig->getServerUrl();
      const auto parts = gitConfig->getCurrentRepoAndOwner();
      const auto api
          = new GitHubRestApi(parts.first, parts.second, { ui->leUserName->text(), ui->leUserToken->text(), endpoint });

      api->testConnection();

      connect(api, &IRestApi::signalConnectionSuccessful, this, [this]() {
         ui->lTestResult->setText("Token confirmed!");
         QTimer::singleShot(3000, ui->lTestResult, &QLabel::clear);
      });
   }
}

void ServerConfigDlg::onServerChanged()
{
   if (ui->cbServer->currentIndex() == GitHubEnterprise)
      ui->leEndPoint->setVisible(true);
}
