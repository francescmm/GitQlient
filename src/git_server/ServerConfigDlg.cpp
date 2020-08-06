#include "ServerConfigDlg.h"
#include "ui_ServerConfigDlg.h"

#include <GitBase.h>
#include <GitConfig.h>
#include <GitQlientStyles.h>
#include <GitQlientSettings.h>
#include <GitHubRestApi.h>
#include <GitLabRestApi.h>

#include <QLogger.h>

#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QMessageBox>
#include <QDesktopServices>

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

static const QMap<GitServerPlatform, QString> repoUrls { { GitHub, "https://api.github.com" },
                                                         { GitHubEnterprise, "" },
                                                         { GitLab, "https://gitlab.com/api/v4" } };
}

ServerConfigDlg::ServerConfigDlg(const QSharedPointer<GitBase> &git, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::ServerConfigDlg)
   , mGit(git)
   , mManager(new QNetworkAccessManager())
{
   setStyleSheet(GitQlientStyles::getStyles());

   ui->setupUi(this);

   connect(ui->cbServer, &QComboBox::currentTextChanged, this, &ServerConfigDlg::onServerChanged);

   ui->leEndPoint->setHidden(true);

   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
   const auto serverUrl = gitConfig->getServerUrl();

   GitQlientSettings settings;
   ui->leUserName->setText(settings.globalValue(QString("%1/user").arg(serverUrl)).toString());
   ui->leUserToken->setText(settings.globalValue(QString("%1/token").arg(serverUrl)).toString());
   ui->leEndPoint->setText(
       settings.globalValue(QString("%1/endpoint").arg(serverUrl), repoUrls.value(GitHub)).toString());

   ui->cbServer->insertItem(GitHub, "GitHub", repoUrls.value(GitHub));
   ui->cbServer->insertItem(GitHubEnterprise, "GitHub Enterprise", repoUrls.value(GitHubEnterprise));

   if (serverUrl.contains("github"))
   {
      const auto index = repoUrls.key(ui->leEndPoint->text(), GitHubEnterprise);
      ui->cbServer->setCurrentIndex(index);
   }
   else
   {
      ui->cbServer->insertItem(GitLab, "GitLab", repoUrls.value(GitLab));
      ui->cbServer->setCurrentIndex(GitLab);
      ui->cbServer->setVisible(false);
   }

   ui->lAccessToken->setText(tr("How to get a token?"));
   connect(ui->lAccessToken, &ButtonLink::clicked, [serverUrl]() {
      const auto url = serverUrl.contains("github")
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
   const auto parts = gitConfig->getCurrentRepoAndOwner();

   GitQlientSettings settings;
   settings.setGlobalValue(QString("%1/user").arg(serverUrl), ui->leUserName->text());
   settings.setGlobalValue(QString("%1/token").arg(serverUrl), ui->leUserToken->text());
   settings.setGlobalValue(QString("%1/endpoint").arg(serverUrl), endpoint);

   if (ui->cbServer->currentIndex() == GitLab)
   {
      const auto api = new GitLabRestApi(ui->leUserName->text(), parts.second, serverUrl,
                                         { ui->leUserName->text(), ui->leUserToken->text(), endpoint });

      GitQlientSettings settings;
      settings.setGlobalValue(QString("%1/userId").arg(serverUrl), api->getUserId());
   }

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
      QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
      const auto serverUrl = gitConfig->getServerUrl();
      const auto parts = gitConfig->getCurrentRepoAndOwner();
      IRestApi *api = nullptr;

      if (ui->cbServer->currentIndex() == GitLab)
         api = new GitLabRestApi(ui->leUserName->text(), parts.second, serverUrl,
                                 { ui->leUserName->text(), ui->leUserToken->text(), endpoint });
      else
      {
         QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
         const auto parts = gitConfig->getCurrentRepoAndOwner();
         api = new GitHubRestApi(parts.first, parts.second,
                                 { ui->leUserName->text(), ui->leUserToken->text(), endpoint });
      }

      api->testConnection();

      connect(api, &IRestApi::connectionTested, this, &ServerConfigDlg::onTestSucceeded);
      connect(api, &IRestApi::errorOccurred, this, &ServerConfigDlg::onGitServerError);
   }
}

void ServerConfigDlg::onServerChanged()
{
   ui->leEndPoint->setVisible(ui->cbServer->currentIndex() == GitHubEnterprise);
}

void ServerConfigDlg::onTestSucceeded()
{
   ui->lTestResult->setText("Token confirmed!");
   QTimer::singleShot(3000, ui->lTestResult, &QLabel::clear);
}

void ServerConfigDlg::onGitServerError(const QString &error)
{
   QMessageBox::warning(this, tr("API access error!"), error);
}
