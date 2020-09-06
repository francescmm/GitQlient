#include "JobContainer.h"
#include <JenkinsViewInfo.h>
#include <JobFetcher.h>
#include <JobButton.h>
#include <ClickableFrame.h>

#include <QVBoxLayout>
#include <QScrollArea>
#include <QTreeWidget>
#include <QLabel>
#include <QListWidget>

namespace Jenkins
{

JobContainer::JobContainer(const IFetcher::Config &config, const JenkinsViewInfo &viewInfo, QWidget *parent)
   : QFrame(parent)
{

   const auto layout = new QVBoxLayout(this);
   layout->setContentsMargins(10, 10, 10, 10);
   layout->setSpacing(0);

   const auto jobFetcher = new JobFetcher(config, viewInfo.url);
   connect(jobFetcher, &JobFetcher::signalJobsReceived, this, &JobContainer::addJobs);
   jobFetcher->triggerFetch();
}

void JobContainer::addJobs(const QMultiMap<QString, JenkinsJobInfo> &jobs)
{
   QVector<JenkinsViewInfo> views;

   const auto keys = jobs.uniqueKeys();
   const auto totalKeys = keys.count();
   QTreeWidget *mJobsTree = nullptr;
   const auto splitView = totalKeys <= 2;

   if (!splitView)
   {
      mJobsTree = new QTreeWidget();
      connect(mJobsTree, &QTreeWidget::itemClicked, this, &JobContainer::showJobInfo);
   }

   for (const auto &key : keys)
   {
      QTreeWidgetItem *item = nullptr;
      QListWidget *listWidget = nullptr;

      if (splitView)
      {
         listWidget = new QListWidget();

         connect(listWidget, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
            const auto job = qvariant_cast<JenkinsJobInfo>(item->data(Qt::UserRole));
            emit signalJobInfoReceived(job);
         });

         createHeader(key, listWidget);
         layout()->addWidget(listWidget);
      }
      else
      {
         item = new QTreeWidgetItem({ key });
         mJobsTree->addTopLevelItem(item);
      }

      const auto values = jobs.values(key);

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
            QVariant v;
            v.setValue(job);

            if (splitView)
            {
               auto jobItem = new QListWidgetItem(getIconForJob(job), job.name, listWidget);

               QVariant v;
               v.setValue(job);
               jobItem->setData(Qt::UserRole, std::move(v));
            }
            else
            {
               QTreeWidgetItem *jobItem = nullptr;

               if (item)
                  jobItem = new QTreeWidgetItem(item, { job.name });
               else
                  jobItem = new QTreeWidgetItem(mJobsTree, { job.name });

               jobItem->setData(0, Qt::UserRole, std::move(v));
               jobItem->setIcon(0, getIconForJob(job));
            }
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

   return QIcon(QString(":/icons/%1").arg(job.color)).pixmap(15, 15);
}

void JobContainer::createHeader(const QString &name, QListWidget *listWidget)
{
   const auto headerFrame = new ClickableFrame();
   headerFrame->setObjectName("tagsFrame");

   const auto headerLayout = new QHBoxLayout(headerFrame);
   headerLayout->setContentsMargins(20, 9, 10, 9);
   headerLayout->setSpacing(10);
   headerLayout->setAlignment(Qt::AlignTop);

   headerLayout->addWidget(new QLabel(name));
   headerLayout->addStretch();

   const auto headerArrow = new QLabel();
   headerArrow->setPixmap(QIcon(":/icons/arrow_down").pixmap(QSize(15, 15)));
   headerLayout->addWidget(headerArrow);

   connect(headerFrame, &ClickableFrame::clicked, this,
           [this, listWidget, headerArrow]() { onHeaderClicked(listWidget, headerArrow); });

   layout()->addWidget(headerFrame);
}

void JobContainer::onHeaderClicked(QListWidget *listWidget, QLabel *arrowIcon)
{
   const auto isVisible = listWidget->isVisible();
   const auto icon = QIcon(isVisible ? QString(":/icons/arrow_up") : QString(":/icons/arrow_down"));
   arrowIcon->setPixmap(icon.pixmap(QSize(15, 15)));
   listWidget->setVisible(!isVisible);
}

}
