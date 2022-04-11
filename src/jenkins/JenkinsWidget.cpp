#include "JenkinsWidget.h"

#include <JobContainer.h>
#include <RepoFetcher.h>

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QNetworkAccessManager>
#include <QPushButton>
#include <QStackedLayout>
#include <QTimer>

namespace Jenkins
{

JenkinsWidget::JenkinsWidget(QWidget *parent)
   : QWidget(parent)
   , mStackedLayout(new QStackedLayout())
   , mBodyLayout(new QHBoxLayout())
   , mBtnGroup(new QButtonGroup())
   , mButtonsLayout(new QVBoxLayout())
   , mTimer(new QTimer(this))
{
   setObjectName("JenkinsWidget");

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

   mTimer->setInterval(15 * 60 * 1000); // 15 mins

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
   connect(mBtnGroup, &QButtonGroup::idClicked, mStackedLayout, &QStackedLayout::setCurrentIndex);
#else
   connect(mBtnGroup, SIGNAL(buttonClicked(int)), mStackedLayout, SLOT(setCurrentIndex(int)));
#endif
}

JenkinsWidget::~JenkinsWidget()
{
   delete mBtnGroup;
}

void JenkinsWidget::initialize(const QString &url, const QString &user, const QString &token)
{
   mConfig = IFetcher::Config { user, token, nullptr };
   mConfig.accessManager.reset(new QNetworkAccessManager());

   mRepoFetcher = new RepoFetcher(mConfig, url, this);
   connect(mRepoFetcher, &RepoFetcher::signalViewsReceived, this, &JenkinsWidget::configureGeneralView);
}

void JenkinsWidget::reload() const
{
   mTimer->stop();
   mRepoFetcher->triggerFetch();
   mTimer->start();
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

         mJobsMap.insert(view.name, container);

         mButtonsLayout->addWidget(button);
         const auto id = mStackedLayout->addWidget(container);
         mBtnGroup->addButton(button, id);

         mViews.append(view);

         if (mViews.count() == 1)
            button->setChecked(true);
      }
      else
         mJobsMap[view.name]->reload();
   }
}

}
