#include <IssuesWidget.h>

#include <IRestApi.h>
#include <CreateIssueDlg.h>
#include <IssueItem.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>

IssuesWidget::IssuesWidget(const QSharedPointer<GitBase> &git, IRestApi *api, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mApi(api)
{
   const auto headerTitle = new QLabel(tr("Issues"));
   headerTitle->setObjectName("HeaderTitle");

   const auto newIssue = new QPushButton(tr("New issue"));
   newIssue->setObjectName("ButtonIssuesHeaderFrame");
   connect(newIssue, &QPushButton::clicked, this, &IssuesWidget::createNewIssue);

   const auto headerFrame = new QFrame();
   headerFrame->setObjectName("IssuesHeaderFrame");
   const auto headerLayout = new QHBoxLayout(headerFrame);
   headerLayout->setContentsMargins(QMargins());
   headerLayout->setSpacing(0);
   headerLayout->addWidget(headerTitle);
   headerLayout->addStretch();
   headerLayout->addWidget(newIssue);

   const auto issuesWidget = new QFrame();
   issuesWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
   issuesWidget->setObjectName("IssuesWidget");
   issuesWidget->setStyleSheet("#IssuesWidget{"
                               "background-color: #2E2F30;"
                               "}");
   mIssuesLayout = new QVBoxLayout(issuesWidget);
   mIssuesLayout->setAlignment(Qt::AlignTop | Qt::AlignVCenter);
   mIssuesLayout->setContentsMargins(QMargins());
   mIssuesLayout->setSpacing(0);

   const auto separator = new QFrame();
   separator->setObjectName("orangeHSeparator");

   const auto scrollArea = new QScrollArea();
   scrollArea->setWidget(issuesWidget);
   scrollArea->setWidgetResizable(true);

   const auto footerFrame = new QFrame();
   footerFrame->setObjectName("IssuesFooterFrame");

   const auto issuesLayout = new QVBoxLayout(this);
   issuesLayout->setContentsMargins(QMargins());
   issuesLayout->setSpacing(0);
   issuesLayout->addWidget(headerFrame);
   issuesLayout->addWidget(separator);
   issuesLayout->addWidget(scrollArea);
   issuesLayout->addWidget(footerFrame);

   connect(mApi, &IRestApi::issuesReceived, this, &IssuesWidget::onIssuesReceived);

   mApi->requestIssues();
}

void IssuesWidget::createNewIssue()
{
   const auto createIssue = new CreateIssueDlg(mGit, this);
   createIssue->exec();
}

void IssuesWidget::onIssuesReceived(const QVector<ServerIssue> &issues)
{
   auto totalIssues = issues.count();
   auto count = 0;

   for (auto &issue : issues)
   {
      mIssuesLayout->addWidget(new IssueItem(issue));

      if (count++ < totalIssues - 1)
      {
         const auto separator = new QFrame();
         separator->setObjectName("orangeHSeparator");
         mIssuesLayout->addWidget(separator);
      }
   }

   mIssuesLayout->addStretch();
}
