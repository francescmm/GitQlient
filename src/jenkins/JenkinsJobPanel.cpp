#include "JenkinsJobPanel.h"
#include <BuildGeneralInfoFetcher.h>

#include <QLabel>
#include <QCheckBox>
#include <QGridLayout>
#include <QScrollArea>
#include <QRadioButton>
#include <QButtonGroup>
#include <QButtonGroup>

namespace QJenkins
{

JenkinsJobPanel::JenkinsJobPanel(const QString &user, const QString &token, QWidget *parent)
   : QFrame(parent)
   , mUser(user)
   , mToken(token)
   , mName(new QLabel())
   , mUrl(new QLabel())
   , mBuildable(new QCheckBox(tr("is buildable")))
   , mInQueue(new QCheckBox(tr("is in queue")))
   , mHealthDesc(new QLabel())
   , mBuildsGroup(new QButtonGroup())
{
   mBuildable->setDisabled(true);
   mInQueue->setDisabled(true);

   const auto auxFrame = new QFrame();
   mBuildListLayout = new QVBoxLayout(auxFrame);
   mBuildListLayout->setContentsMargins(QMargins());
   mBuildListLayout->setSpacing(10);

   const auto scrollArea = new QScrollArea();
   scrollArea->setWidget(auxFrame);
   scrollArea->setWidgetResizable(true);

   const auto layout = new QGridLayout(this);
   layout->setContentsMargins(QMargins());
   layout->setSpacing(10);
   layout->addWidget(new QLabel(tr("Job name: ")), 0, 0);
   layout->addWidget(mName, 0, 1);
   layout->addWidget(new QLabel(tr("URL: ")), 1, 0);
   layout->addWidget(mUrl, 1, 1);
   layout->addWidget(new QLabel(tr("Description: ")), 2, 0);
   layout->addWidget(mHealthDesc, 2, 1);
   layout->addWidget(mBuildable, 3, 0, 1, 2);
   layout->addWidget(mInQueue, 4, 0, 1, 2);
   layout->addWidget(scrollArea, 5, 0, 1, 2);
}

void JenkinsJobPanel::onJobInfoReceived(const JenkinsJobInfo &job)
{
   mRequestedJob = job;
   mName->setText(mRequestedJob.name);
   mUrl->setText(mRequestedJob.url);
   mHealthDesc->setText(mRequestedJob.healthStatus.description);
   mBuildable->setChecked(mRequestedJob.buildable);
   mInQueue->setChecked(mRequestedJob.inQueue);

   mTmpBuildsCounter = mRequestedJob.builds.count();

   for (const auto &build : mRequestedJob.builds)
   {
      const auto buildFetcher = new BuildGeneralInfoFetcher(mUser, mToken, build);
      connect(buildFetcher, &BuildGeneralInfoFetcher::signalBuildInfoReceived, this, &JenkinsJobPanel::appendJobsData);

      buildFetcher->triggerFetch();
   }

   for (const auto button : mBuilds)
   {
      mBuildsGroup->removeButton(button);
      delete button;
   }

   mBuilds.clear();
}

void JenkinsJobPanel::appendJobsData(const JenkinsJobBuildInfo &build)
{
   mRequestedJob.builds[build.number] = build;

   --mTmpBuildsCounter;

   if (mTmpBuildsCounter == 0)
   {
      for (const auto build : mRequestedJob.builds)
      {
         const auto btn = new QRadioButton(tr("%1 - %2").arg(build.number).arg(build.result));
         mBuilds.append(btn);
         mBuildsGroup->addButton(btn);

         mBuildListLayout->addWidget(btn);
      }
   }
}

}
