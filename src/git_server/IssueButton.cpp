#include <IssueButton.h>

#include <ServerIssue.h>
#include <ButtonLink.hpp>

#include <QGridLayout>

IssueButton::IssueButton(const ServerIssue &issueData, QWidget *parent)
   : QFrame(parent)
{
   const auto layout = new QGridLayout(this);
   layout->setContentsMargins(QMargins());
   layout->setSpacing(10);
   layout->addWidget(new ButtonLink(issueData.title), 0, 0);
   layout->addWidget(new QLabel(issueData.labels.join(", ")), 1, 0);
   layout->addWidget(new QLabel(issueData.milestone.title), 2, 0);

   /*
   QStringList assignees;
   for (auto &assignee : issueData.assignees)
      assignees.append(assignee.name);

   layout->addWidget(new QLabel(assignees.join(", ")), 3, 0);
   */
}
