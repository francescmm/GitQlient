#include "ServerConfigDlg.h"
#include "ui_ServerConfigDlg.h"

#include <GitHubRestApi.h>
#include <GitLabRestApi.h>
#include <GitServerCache.h>

#include <QLogger.h>

#include <QDesktopServices>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

#include <utility>

using namespace QLogger;
using namespace GitServer;

namespace
{
enum GitServerPlatform
{
   GitHub,
   GitHubEnterprise,
   GitLab,
   Bitbucket
};

static const QMap<GitServerPlatform, const char *> repoUrls { { GitHub, "https://api.github.com" },
                                                              { GitHubEnterprise, "" },
                                                              { GitLab, "https://gitlab.com/api/v4" } };
}

ServerConfigDlg::ServerConfigDlg(const QSharedPointer<GitServerCache> &gitServerCache,
                                 const GitServer::ConfigData &data, const QString &styleSheet, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::ServerConfigDlg)
   , mGitServerCache(gitServerCache)
   , mData(data)
   , mManager(new QNetworkAccessManager())
{
   setStyleSheet(styleSheet);

   ui->setupUi(this);

   connect(ui->cbServer, &QComboBox::currentTextChanged, this, &ServerConfigDlg::onServerChanged);

   ui->leEndPoint->setHidden(true);

   ui->leUserName->setText(mData.user);
   ui->leUserToken->setText(mData.token);
   ui->leEndPoint->setText(mData.endPoint);

   ui->cbServer->insertItem(GitHub, "GitHub", repoUrls.value(GitHub));
   ui->cbServer->insertItem(GitHubEnterprise, "GitHub Enterprise", repoUrls.value(GitHubEnterprise));

   if (mData.serverUrl.contains("github"))
   {
      const auto index = repoUrls.key(ui->leEndPoint->text().toUtf8(), GitHubEnterprise);
      ui->cbServer->setCurrentIndex(index);
   }
   else
   {
      ui->cbServer->insertItem(GitLab, "GitLab", repoUrls.value(GitLab));
      ui->cbServer->setCurrentIndex(GitLab);
      ui->cbServer->setVisible(false);
   }

   ui->lAccessToken->setText(tr("How to get a token?"));
   connect(ui->lAccessToken, &ButtonLink::clicked, [isGitHub = mData.serverUrl.contains("github")]() {
      const auto url = isGitHub
          ? "https://docs.github.com/en/github/authenticating-to-github/creating-a-personal-access-token"
          : "https://docs.gitlab.com/ee/user/profile/personal_access_tokens.html";
      QDesktopServices::openUrl(QUrl(QString::fromUtf8(url)));
   });

   connect(ui->leUserToken, &QLineEdit::editingFinished, this, &ServerConfigDlg::checkToken);
   connect(ui->leUserToken, &QLineEdit::returnPressed, this, &ServerConfigDlg::accept);
   connect(ui->pbAccept, &QPushButton::clicked, this, &ServerConfigDlg::accept);
   connect(ui->pbCancel, &QPushButton::clicked, this, &ServerConfigDlg::reject);
   connect(ui->pbTest, &QPushButton::clicked, this, &ServerConfigDlg::testToken);
}

ServerConfigDlg::~ServerConfigDlg()
{
   delete mManager;
   delete ui;
}

ConfigData ServerConfigDlg::getNewConfigData() const
{
   return GitServer::ConfigData { ui->leUserName->text(), ui->leUserToken->text(), mData.serverUrl, mEndPoint,
                                  mData.repoName,         mData.repoOwner };
}

void ServerConfigDlg::checkToken()
{
   if (ui->leUserToken->text().isEmpty())
      ui->leUserName->setStyleSheet("border: 1px solid red;");
}

void ServerConfigDlg::accept()
{
   mEndPoint = ui->cbServer->currentIndex() == GitHubEnterprise ? ui->leEndPoint->text()
                                                                : ui->cbServer->currentData().toString();
}

void ServerConfigDlg::testToken()
{
   if (ui->leUserToken->text().isEmpty())
      ui->leUserName->setStyleSheet("border: 1px solid red;");
   else
   {
      const auto endpoint = ui->cbServer->currentIndex() == GitHubEnterprise ? ui->leEndPoint->text()
                                                                             : ui->cbServer->currentData().toString();
      IRestApi *api = nullptr;

      if (ui->cbServer->currentIndex() == GitLab)
      {
         api = new GitLabRestApi(ui->leUserName->text(), mData.repoName, mData.serverUrl,
                                 { ui->leUserName->text(), ui->leUserToken->text(), endpoint }, this);
      }
      else
      {
         api = new GitHubRestApi(mData.repoOwner, mData.repoName,
                                 { ui->leUserName->text(), ui->leUserToken->text(), endpoint }, this);
      }

      api->testConnection();

      connect(api, &IRestApi::connectionTested, this, &ServerConfigDlg::onTestSucceeded);
      connect(api, &IRestApi::errorOccurred, this,
              [this](const QString &error) { QMessageBox::warning(this, tr("API access error!"), error); });
   }
}

void ServerConfigDlg::onServerChanged()
{
   ui->leEndPoint->setVisible(ui->cbServer->currentIndex() == GitHubEnterprise);
}

void ServerConfigDlg::onTestSucceeded()
{
   ui->lTestResult->setText(tr("Token confirmed!"));
   QTimer::singleShot(3000, ui->lTestResult, &QLabel::clear);
}
