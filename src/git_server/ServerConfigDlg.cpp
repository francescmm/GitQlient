#include "ServerConfigDlg.h"
#include "ui_ServerConfigDlg.h"

#include <GitBase.h>
#include <GitConfig.h>
#include <GitQlientStyles.h>
#include <GitQlientSettings.h>
#include <GitHubRestApi.h>

#include <QLogger.h>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

#include <utility>

using namespace QLogger;

ServerConfigDlg::ServerConfigDlg(const QSharedPointer<GitBase> &git, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::ServerConfigDlg)
   , mGit(git)
   , mManager(new QNetworkAccessManager())
{
   setStyleSheet(GitQlientStyles::getStyles());

   ui->setupUi(this);

   connect(ui->leUserToken, &QLineEdit::editingFinished, this, &ServerConfigDlg::checkToken);
   connect(ui->leUserToken, &QLineEdit::returnPressed, this, &ServerConfigDlg::accept);
   connect(ui->pbAccept, &QPushButton::clicked, this, &ServerConfigDlg::accept);
   connect(ui->pbCancel, &QPushButton::clicked, this, &ServerConfigDlg::reject);
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
   if (ui->leUserToken->text().isEmpty())
      ui->leUserName->setStyleSheet("border: 1px solid red;");
   else
   {
      QScopedPointer<GitConfig> gitConfig(new GitConfig(mGit));
      const auto serverUrl = gitConfig->getServerUrl();
      const auto parts = gitConfig->getCurrentRepoAndOwner();
      const auto api
          = new GitHubRestApi(parts.first, parts.second, { ui->leUserName->text(), ui->leUserToken->text() });

      api->testConnection();

      connect(api, &GitHubRestApi::signalConnectionSuccessful, this, [this, serverUrl]() {
         GitQlientSettings settings;
         settings.setValue(QString("%1/user").arg(serverUrl), ui->leUserName->text());
         settings.setValue(QString("%1/token").arg(serverUrl), ui->leUserToken->text());

         QDialog::accept();
      });
   }
}
