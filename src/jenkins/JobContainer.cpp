#include "JobContainer.h"
#include <JenkinsViewInfo.h>
#include <JobFetcher.h>
#include <JobButton.h>

#include <QVBoxLayout>
#include <QScrollArea>

namespace QJenkins
{

JobContainer::JobContainer(const QString &user, const QString &token, const JenkinsViewInfo &viewInfo, QWidget *parent)
   : QFrame(parent)
{
   const auto auxFrame = new QFrame();
   auxFrame->setObjectName("JobContainer");
   mLayout = new QVBoxLayout(auxFrame);
   mLayout->setContentsMargins(10, 10, 10, 10);
   mLayout->setSpacing(10);

   const auto scrollArea = new QScrollArea();
   scrollArea->setObjectName("JobContainerScrollArea");
   scrollArea->setWidget(auxFrame);
   scrollArea->setWidgetResizable(true);

   const auto layout = new QVBoxLayout(this);
   layout->setContentsMargins(QMargins());
   layout->setSpacing(0);
   layout->addWidget(scrollArea);

   const auto jobFetcher = new JobFetcher(user, token, viewInfo.url);
   connect(jobFetcher, &JobFetcher::signalJobsReceived, this, &JobContainer::addJobs);
   jobFetcher->triggerFetch();
}

void JobContainer::addJobs(const QVector<JenkinsJobInfo> &jobs)
{
   for (const auto &job : jobs)
   {
      const auto button = new JobButton(job);
      button->setObjectName("JobButton");
      connect(button, &JobButton::clicked, this, &JobContainer::showJobInfo);
      mLayout->addWidget(button);
   }
}

void JobContainer::showJobInfo()
{
   if (auto btn = qobject_cast<JobButton *>(sender()))
      emit signalJobInfoReceived(btn->getJenkinsJob());
}

}
