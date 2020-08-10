#include <IssueItem.h>

#include <ButtonLink.hpp>
#include <Issue.h>

#include <QDir>
#include <QFile>
#include <QUrl>
#include <QGridLayout>
#include <QDesktopServices>

using namespace GitServer;

IssueItem::IssueItem(const Issue &issueData, QWidget *parent)
   : QFrame(parent)
   , mIssue(issueData)
{
   setObjectName("IssueItem");

   const auto title = new ButtonLink(issueData.title);
   title->setWordWrap(false);
   title->setObjectName("IssueTitle");
   connect(title, &ButtonLink::clicked, [this]() { emit selected(mIssue); });

   const auto titleLayout = new QHBoxLayout();
   titleLayout->setContentsMargins(QMargins());
   titleLayout->setSpacing(0);
   titleLayout->addWidget(title);
   titleLayout->addStretch();

   const auto creationLayout = new QHBoxLayout();
   creationLayout->setContentsMargins(QMargins());
   creationLayout->setSpacing(0);
   creationLayout->addWidget(new QLabel(tr("Created by ")));
   const auto creator = new ButtonLink(QString("<b>%1</b>").arg(issueData.creator.name));
   creator->setObjectName("CreatorLink");
   connect(creator, &ButtonLink::clicked, [this]() { QDesktopServices::openUrl(mIssue.url); });

   creationLayout->addWidget(creator);

   const auto days = mIssue.creation.daysTo(QDateTime::currentDateTime());
   const auto whenText = days <= 30 ? QString::fromUtf8(" %1 days ago").arg(days)
                                    : QString(" on %1").arg(mIssue.creation.date().toString(Qt::SystemLocaleShortDate));

   const auto whenLabel = new QLabel(whenText);
   whenLabel->setToolTip(issueData.creation.toString(Qt::SystemLocaleShortDate));

   creationLayout->addWidget(whenLabel);
   creationLayout->addStretch();

   const auto labelsLayout = new QHBoxLayout();
   labelsLayout->setContentsMargins(QMargins());
   labelsLayout->setSpacing(10);

   QStringList labelsList;

   for (auto &label : issueData.labels)
   {
      auto labelWidget = new QLabel();
      labelWidget->setStyleSheet(QString("QLabel {"
                                         "background-color: #%1;"
                                         "border-radius: 7px;"
                                         "min-height: 15px;"
                                         "max-height: 15px;"
                                         "min-width: 15px;"
                                         "max-width: 15px;}")
                                     .arg(label.colorHex));
      labelWidget->setToolTip(label.name);
      labelsLayout->addWidget(labelWidget);
   }

   const auto milestone = new QLabel(QString("%1").arg(issueData.milestone.title));
   milestone->setObjectName("IssueLabel");
   labelsLayout->addWidget(milestone);
   labelsLayout->addStretch();

   const auto layout = new QVBoxLayout(this);
   layout->setContentsMargins(0, 10, 0, 10);
   layout->setSpacing(5);
   layout->addLayout(titleLayout);
   layout->addLayout(creationLayout);

   if (!issueData.assignees.isEmpty())
   {
      const auto assigneeLayout = new QHBoxLayout();
      assigneeLayout->setContentsMargins(QMargins());
      assigneeLayout->setSpacing(0);
      assigneeLayout->addWidget(new QLabel(tr("Assigned to ")));

      auto count = 0;
      const auto totalAssignees = issueData.assignees.count();
      for (auto &assignee : issueData.assignees)
      {
         const auto assigneLabel = new ButtonLink(QString("<b>%1</b>").arg(assignee.name));
         assigneLabel->setObjectName("CreatorLink");
         connect(assigneLabel, &ButtonLink::clicked, [url = assignee.url]() { QDesktopServices::openUrl(url); });
         assigneeLayout->addWidget(assigneLabel);

         if (count++ < totalAssignees - 1)
            assigneeLayout->addWidget(new QLabel(", "));
      }
      assigneeLayout->addStretch();

      layout->addLayout(assigneeLayout);
   }

   layout->addLayout(labelsLayout);
}
