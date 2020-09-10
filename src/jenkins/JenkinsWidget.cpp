#include "JenkinsWidget.h"

#include <RepoFetcher.h>
#include <JobContainer.h>
#include <GitQlientSettings.h>
#include <GitBase.h>

#include <QButtonGroup>
#include <QStackedLayout>
#include <QPushButton>
#include <QHBoxLayout>
#include <QNetworkAccessManager>

namespace Jenkins
{

JenkinsWidget::JenkinsWidget(const QSharedPointer<GitBase> &git, QWidget *parent)
   : QWidget(parent)
   , mGit(git)
   , mStackedLayout(new QStackedLayout())
   , mBodyLayout(new QHBoxLayout())
   , mBtnGroup(new QButtonGroup())
   , mButtonsLayout(new QVBoxLayout())
{
   setObjectName("JenkinsWidget");

   GitQlientSettings settings;
   const auto url = settings.localValue(mGit->getGitQlientSettingsDir(), "BuildSystemUrl", "").toString();
   const auto user = settings.localValue(mGit->getGitQlientSettingsDir(), "BuildSystemUser", "").toString();
   const auto token = settings.localValue(mGit->getGitQlientSettingsDir(), "BuildSystemToken", "").toString();

   mConfig = IFetcher::Config { user, token, nullptr };
   mConfig.accessManager.reset(new QNetworkAccessManager());

   const auto superBtnsLayout = new QVBoxLayout();
   superBtnsLayout->setContentsMargins(QMargins());
   superBtnsLayout->setSpacing(0);
   superBtnsLayout->addLayout(mButtonsLayout);
   superBtnsLayout->addStretch();

   mBodyLayout->setSpacing(0);
   mBodyLayout->addLayout(superBtnsLayout);
   mBodyLayout->addLayout(mStackedLayout);

   const auto layout = new QHBoxLayout(this);
   layout->setContentsMargins(QMargins());
   layout->setSpacing(0);
   layout->addLayout(mBodyLayout);

   setMinimumSize(800, 600);

   mRepoFetcher = new RepoFetcher(mConfig, url, this);
   connect(mRepoFetcher, &RepoFetcher::signalViewsReceived, this, &JenkinsWidget::configureGeneralView);

   connect(mBtnGroup, &QButtonGroup::idClicked, mStackedLayout, &QStackedLayout::setCurrentIndex);
}

void JenkinsWidget::reload() const
{
   mRepoFetcher->triggerFetch();
}

void JenkinsWidget::configureGeneralView(const QVector<JenkinsViewInfo> &views)
{
   for (auto &view : views)
   {
      if (!mViews.contains(view))
      {
         const auto button = new QPushButton(view.name);
         button->setObjectName("JenkinsWidgetTabButton");
         button->setCheckable(true);

         const auto container = new JobContainer(mConfig, view, this);
         container->setObjectName("JobContainer");
         connect(container, &JobContainer::signalJobAreViews, this, &JenkinsWidget::configureGeneralView);
         connect(container, &JobContainer::gotoBranch, this, &JenkinsWidget::gotoBranch);
         connect(container, &JobContainer::gotoPullRequest, this, &JenkinsWidget::gotoPullRequest);

         mButtonsLayout->addWidget(button);
         const auto id = mStackedLayout->addWidget(container);
         mBtnGroup->addButton(button, id);

         mViews.append(view);

         if (mViews.count() == 1)
            button->setChecked(true);
      }
   }
}

}
