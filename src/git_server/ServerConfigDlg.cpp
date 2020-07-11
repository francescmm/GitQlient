#include "ServerConfigDlg.h"
#include "ui_ServerConfigDlg.h"

#include <GitBase.h>
#include <GitConfig.h>
#include <GitQlientStyles.h>
#include <GitQlientSettings.h>

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
      auto repoUrl = gitConfig->getGitValue("remote.origin.url").output.toString().trimmed();
      QString repo = repoUrl;

      if (repo.startsWith("git@"))
         repo = repo.mid(repo.lastIndexOf(":") + 1);
      else if (repo.startsWith("https"))
      {
         repo = repo.remove("https://");
         repo = repo.mid(repo.indexOf("/") + 1);
      }
      repo.remove(".git");

      QUrl url(QString("https://api.github.com/repos/%1").arg(repo));
      url.setUserName(ui->leUserName->text());
      url.setPassword(ui->leUserToken->text());

      QNetworkRequest request;
      request.setUrl(url);

      const auto reply = mManager->get(request);
      connect(reply, &QNetworkReply::readyRead, this, [reply, this]() { onUserTokenCheck(reply); });

      if (repoUrl.startsWith("git@"))
      {
         repoUrl.remove("git@");
         repoUrl.replace(":", "/");
      }
      else if (repoUrl.startsWith("https://"))
         repoUrl.remove("https://");

      repoUrl = repoUrl.mid(0, repo.indexOf("/"));

      GitQlientSettings settings;
      settings.setValue(QString("%1/user").arg(repoUrl), ui->leUserName->text());
      settings.setValue(QString("%1/token").arg(repoUrl), ui->leUserToken->text());
   }
}

void ServerConfigDlg::onUserTokenCheck(QNetworkReply *reply)
{
   if (reply == nullptr)
      return;

   const auto data = reply->readAll();
   const auto jsonDoc = QJsonDocument::fromJson(data);

   if (jsonDoc.isNull())
   {
      QLog_Error("Ui", QString("Error when parsing Json. Current data:\n%1").arg(QString::fromUtf8(data)));
      return;
   }

   QJsonObject jsonObject = jsonDoc.object();
   if (jsonObject.contains(QStringLiteral("message")))
   {
      const auto message = jsonObject[QStringLiteral("message")].toString();

      if (message.contains("Not found", Qt::CaseInsensitive))
         QLog_Error("Ui", QString("Error when validating user and token."));

      return;
   }

   QDialog::accept();
}
