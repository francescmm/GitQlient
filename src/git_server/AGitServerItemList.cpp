#include <AGitServerItemList.h>

#include <GitServerCache.h>
#include <IssueItem.h>
#include <ClickableFrame.h>
#include <IRestApi.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QSpinBox>
#include <QToolButton>

using namespace GitServer;

AGitServerItemList::AGitServerItemList(const QSharedPointer<GitServerCache> &gitServerCache, QWidget *parent)
   : QFrame(parent)
   , mGitServerCache(gitServerCache)
   , mHeaderIconLabel(new QLabel())
   , mHeaderTitle(new QLabel())
   , mArrow(new QLabel())
{
   const auto pagesSpinBox = new QSpinBox(this);
   connect(pagesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(loadPage(int)));

   const auto pagesLabel = new QLabel(tr("Page: "));

   mHeaderTitle->setObjectName("HeaderTitle");

   const auto headerFrame = new ClickableFrame();
   headerFrame->setObjectName("IssuesHeaderFrame");
   connect(headerFrame, &ClickableFrame::clicked, this, &AGitServerItemList::onHeaderClicked);

   mArrow->setPixmap(QIcon(":/icons/arrow_up").pixmap(QSize(15, 15)));

   const auto headerLayout = new QHBoxLayout(headerFrame);
   headerLayout->setContentsMargins(QMargins());
   headerLayout->setSpacing(0);
   headerLayout->addWidget(mHeaderIconLabel);
   headerLayout->addSpacing(10);
   headerLayout->addWidget(mHeaderTitle);
   headerLayout->addStretch();
   headerLayout->addWidget(mArrow);

   mIssuesLayout = new QVBoxLayout();

   const auto footerFrame = new QFrame();
   footerFrame->setObjectName("IssuesFooterFramePagination");

   pagesSpinBox->setVisible(false);

   const auto footerLayout = new QHBoxLayout(footerFrame);
   footerLayout->setContentsMargins(5, 5, 5, 5);
   footerLayout->addWidget(pagesLabel);
   footerLayout->addWidget(pagesSpinBox);
   footerLayout->addStretch();

   const auto issuesLayout = new QVBoxLayout(this);
   issuesLayout->setContentsMargins(QMargins());
   issuesLayout->setSpacing(0);
   issuesLayout->setAlignment(Qt::AlignTop);
   issuesLayout->addWidget(headerFrame);
   issuesLayout->addLayout(mIssuesLayout);
   issuesLayout->addWidget(footerFrame);

   const auto timer = new QTimer();
   connect(timer, &QTimer::timeout, this, &AGitServerItemList::loadData);
   timer->start(900000);

   /*
   connect(mApi, &IRestApi::paginationPresent, this, [pagesSpinBox, pagesLabel](int current, int, int total) {
      if (total != 0)
      {
         pagesSpinBox->blockSignals(true);
         pagesSpinBox->setRange(1, total);
         pagesSpinBox->setValue(current);
         pagesSpinBox->setVisible(true);
         pagesLabel->setVisible(true);
         pagesSpinBox->blockSignals(false);
      }
   });
*/
}

void AGitServerItemList::loadData()
{
   loadPage();
}

void AGitServerItemList::createContent(QVector<IssueItem *> items)
{
   delete mIssuesWidget;
   delete mScrollArea;

   const auto issuesLayout = new QVBoxLayout();
   issuesLayout->setAlignment(Qt::AlignTop | Qt::AlignVCenter);
   issuesLayout->setContentsMargins(QMargins());
   issuesLayout->setSpacing(0);

   for (auto item : items)
   {
      issuesLayout->addWidget(item);

      const auto separator = new QFrame();
      separator->setObjectName("orangeHSeparator");
      issuesLayout->addWidget(separator);
   }

   issuesLayout->addStretch();

   mIssuesWidget = new QFrame();
   mIssuesWidget->setLayout(issuesLayout);
   mIssuesWidget->setObjectName("IssuesWidget");
   mIssuesWidget->setStyleSheet("#IssuesWidget{"
                                "background-color: #2E2F30;"
                                "}");

   mScrollArea = new QScrollArea();
   mScrollArea->setWidget(mIssuesWidget);
   mScrollArea->setWidgetResizable(true);

   mIssuesLayout->addWidget(mScrollArea);

   const auto icon = QIcon(mScrollArea->isVisible() ? QString(":/icons/arrow_up") : QString(":/icons/arrow_down"));
   mArrow->setPixmap(icon.pixmap(QSize(15, 15)));

   setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
}

void AGitServerItemList::onHeaderClicked()
{
   if (mScrollArea)
   {
      const auto issuesVisible = mScrollArea->isVisible();

      mScrollArea->setWidgetResizable(issuesVisible);

      const auto icon = QIcon(issuesVisible ? QString(":/icons/arrow_up") : QString(":/icons/arrow_down"));
      mArrow->setPixmap(icon.pixmap(QSize(15, 15)));
      mScrollArea->setVisible(!issuesVisible);

      /*
      GitQlientSettings settings;
      settings.setLocalValue(mGit->getGitQlientSettingsDir(),
      QString("IssuesWidgetHeader/%1").arg(static_cast<int>(mConfig), !tagsAreVisible);
      */

      if (issuesVisible)
         setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
      else
         setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
   }
}

void AGitServerItemList::loadPage(int)
{
   /*
   if (page == -1)
   {
      if (mConfig == Config::Issues)
         mApi->requestIssues();
      else
         mApi->requestPullRequests();
   }
   else
   {
      if (mConfig == Config::Issues)
         mApi->requestIssues(page);
      else
         mApi->requestPullRequests(page);
   }
*/
}
