#include "JobContainer.h"
#include <JenkinsViewInfo.h>
#include <JobFetcher.h>
#include <JobButton.h>

#include <QVBoxLayout>
#include <QScrollArea>
#include <QTreeWidget>

namespace Jenkins
{

JobContainer::JobContainer(const IFetcher::Config &config, const JenkinsViewInfo &viewInfo, QWidget *parent)
   : QFrame(parent)
   , mJobsTree(new QTreeWidget())
{
   connect(mJobsTree, &QTreeWidget::itemClicked, this, &JobContainer::showJobInfo);

   const auto layout = new QVBoxLayout(this);
   layout->setContentsMargins(QMargins());
   layout->setSpacing(0);
   layout->addWidget(mJobsTree);

   const auto jobFetcher = new JobFetcher(config, viewInfo.url);
   connect(jobFetcher, &JobFetcher::signalJobsReceived, this, &JobContainer::addJobs);
   jobFetcher->triggerFetch();
}

void JobContainer::addJobs(const QMultiMap<QString, JenkinsJobInfo> &jobs)
{
   QVector<JenkinsViewInfo> views;

   const auto keys = jobs.uniqueKeys();
   for (const auto &key : keys)
   {
      QTreeWidgetItem *item = nullptr;

      if (keys.count() > 1)
      {
         item = new QTreeWidgetItem({ key });
         mJobsTree->addTopLevelItem(item);
      }

      const auto values = jobs.values(key);
      auto count = 0;

      for (const auto &job : qAsConst(values))
      {
         if (job.builds.isEmpty() && job.color.isEmpty())
         {
            JenkinsViewInfo view;
            view.name = job.name;
            view.url = job.url;

            views.append(std::move(view));
         }
         else
         {
            ++count;
            QTreeWidgetItem *jobItem = nullptr;

            if (item)
               jobItem = new QTreeWidgetItem(item, { job.name });
            else
               jobItem = new QTreeWidgetItem(mJobsTree, { job.name });

            QVariant v;
            v.setValue(job);
            jobItem->setData(0, Qt::UserRole, std::move(v));
            jobItem->setIcon(0, getIconForJob(job));
         }
      }

      if (item)
         item->setExpanded(true);
   }

   if (!views.isEmpty())
      emit signalJobAreViews(views);
}

void JobContainer::showJobInfo(QTreeWidgetItem *item, int column)
{
   const auto job = qvariant_cast<JenkinsJobInfo>(item->data(column, Qt::UserRole));
   emit signalJobInfoReceived(job);
}

QIcon JobContainer::getIconForJob(JenkinsJobInfo job) const
{
   job.color.remove("_anime");

   if (job.color.contains("blue"))
      job.color = "green";
   else if (job.color.contains("disabled") || job.color.contains("grey") || job.color.contains("notbuilt"))
      job.color = "grey";
   else if (job.color.contains("aborted"))
      job.color = "dark_grey";

   return QIcon(QString(":/icons/%1").arg(job.color)).pixmap(22, 22);
}

}
