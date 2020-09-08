#include <JenkinsJobPanel.h>

#include <BuildGeneralInfoFetcher.h>
#include <CheckBox.h>
#include <ButtonLink.hpp>

#include <QLineEdit>
#include <QPlainTextEdit>
#include <QUrl>
#include <QFile>
#include <QLabel>
#include <QGroupBox>
#include <QGridLayout>
#include <QScrollArea>
#include <QRadioButton>
#include <QButtonGroup>
#include <QButtonGroup>
#include <QTabWidget>
#include <QStandardPaths>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMessageBox>

namespace Jenkins
{

JenkinsJobPanel::JenkinsJobPanel(const IFetcher::Config &config, QWidget *parent)
   : QFrame(parent)
   , mConfig(config)
   , mName(new QLabel())
   , mUrl(new QLabel())
   , mBuildable(new CheckBox(tr("is buildable")))
   , mInQueue(new CheckBox(tr("is in queue")))
   , mHealthDesc(new QLabel())
   , mManager(new QNetworkAccessManager(this))
{
   setObjectName("JenkinsJobPanel");

   mBuildable->setDisabled(true);
   mInQueue->setDisabled(true);

   mScrollFrame = new QFrame();
   mScrollFrame->setObjectName("TransparentScrollArea");

   mLastBuildFrame = new QFrame();

   const auto scrollArea = new QScrollArea();
   scrollArea->setWidget(mScrollFrame);
   scrollArea->setWidgetResizable(true);
   scrollArea->setStyleSheet("background: #404142;");

   mTabWidget = new QTabWidget();
   mTabWidget->addTab(scrollArea, "Previous builds");

   const auto layout = new QGridLayout(this);
   layout->setContentsMargins(0, 10, 10, 10);
   layout->setSpacing(10);
   layout->addWidget(new QLabel(tr("Job name: ")), 0, 0);
   layout->addWidget(mName, 0, 1);
   layout->addWidget(new QLabel(tr("URL: ")), 1, 0);
   layout->addWidget(mUrl, 1, 1);
   layout->addWidget(new QLabel(tr("Description: ")), 2, 0);
   layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 2);
   layout->addWidget(mHealthDesc, 2, 1);
   layout->addWidget(mBuildable, 3, 0, 1, 2);
   layout->addWidget(mInQueue, 4, 0, 1, 2);
   layout->addWidget(mLastBuildFrame, 5, 0, 1, 3);
   layout->addWidget(mTabWidget, 6, 0, 1, 3);
}

void JenkinsJobPanel::onJobInfoReceived(const JenkinsJobInfo &job)
{
   for (const auto &widget : qAsConst(mTempWidgets))
      delete widget;

   mTempWidgets.clear();

   delete mBuildListLayout;
   delete mLastBuildLayout;

   mTabWidget->setCurrentIndex(0);
   mTabBuildMap.clear();

   for (auto i = 1; i < mTabWidget->count(); ++i)
      mTabWidget->removeTab(i);

   mRequestedJob = job;
   mName->setText(mRequestedJob.name);
   mUrl->setText(mRequestedJob.url);
   mHealthDesc->setText(mRequestedJob.healthStatus.description);
   mBuildable->setChecked(mRequestedJob.buildable);
   mInQueue->setChecked(mRequestedJob.inQueue);

   mTmpBuildsCounter = mRequestedJob.builds.count();

   for (const auto &build : qAsConst(mRequestedJob.builds))
   {
      const auto buildFetcher = new BuildGeneralInfoFetcher(mConfig, build);
      connect(buildFetcher, &BuildGeneralInfoFetcher::signalBuildInfoReceived, this, &JenkinsJobPanel::appendJobsData);
      connect(buildFetcher, &BuildGeneralInfoFetcher::signalBuildInfoReceived, buildFetcher,
              &BuildGeneralInfoFetcher::deleteLater);

      buildFetcher->triggerFetch();
   }
}

void JenkinsJobPanel::appendJobsData(const JenkinsJobBuildInfo &build)
{
   auto iter = std::find(mRequestedJob.builds.begin(), mRequestedJob.builds.end(), build);
   *iter = build;

   --mTmpBuildsCounter;

   if (mTmpBuildsCounter == 0)
   {
      mBuildListLayout = new QVBoxLayout(mScrollFrame);
      mBuildListLayout->setContentsMargins(QMargins());
      mBuildListLayout->setSpacing(10);

      mLastBuildLayout = new QHBoxLayout(mLastBuildFrame);
      mLastBuildLayout->setContentsMargins(10, 10, 10, 10);
      mLastBuildLayout->setSpacing(10);

      std::sort(mRequestedJob.builds.begin(), mRequestedJob.builds.end(),
                [](const JenkinsJobBuildInfo &build1, const JenkinsJobBuildInfo &build2) {
                   return build1.number > build2.number;
                });

      const auto build = mRequestedJob.builds.takeFirst();

      fillBuildLayout(build, mLastBuildLayout);

      for (const auto &build : qAsConst(mRequestedJob.builds))
      {
         const auto stagesLayout = new QHBoxLayout();
         stagesLayout->setContentsMargins(10, 10, 10, 10);
         stagesLayout->setSpacing(10);

         fillBuildLayout(build, stagesLayout);

         mBuildListLayout->addLayout(stagesLayout);
      }

      mBuildListLayout->addStretch();
   }
}

void JenkinsJobPanel::fillBuildLayout(const JenkinsJobBuildInfo &build, QHBoxLayout *layout)
{
   const auto mark = new ButtonLink(QString::number(build.number));
   mark->setToolTip(build.result);
   mark->setFixedSize(30, 30);
   mark->setStyleSheet(QString("QLabel{"
                               "background: %1;"
                               "border-radius: 15px;"
                               "qproperty-alignment: AlignCenter;"
                               "}")
                           .arg(Jenkins::resultColor(build.result).name()));
   connect(mark, &ButtonLink::clicked, this, [this, build]() { requestFile(build); });

   mTempWidgets.append(mark);

   layout->addWidget(mark);

   for (const auto &stage : build.stages)
   {
      QTime t = QTime(0, 0, 0).addMSecs(stage.duration);
      const auto tStr = t.toString("HH:mm:ss.zzz");
      const auto label = new QLabel(QString("%1").arg(stage.name));
      label->setToolTip(tStr);
      label->setObjectName("StageStatus");
      label->setFixedSize(100, 80);
      label->setWordWrap(true);
      label->setStyleSheet(QString("QLabel{"
                                   "background: %1;"
                                   "color: white;"
                                   "border-radius: 10px;"
                                   "border-bottom-right-radius: 0px;"
                                   "border-bottom-left-radius: 0px;"
                                   "qproperty-alignment: AlignCenter;"
                                   "padding: 5px;"
                                   "}")
                               .arg(Jenkins::resultColor(stage.result).name()));
      mTempWidgets.append(label);

      const auto time = new QLabel(tStr);
      time->setToolTip(tStr);
      time->setFixedSize(100, 20);
      time->setStyleSheet(QString("QLabel{"
                                  "background: %1;"
                                  "color: white;"
                                  "border-radius: 10px;"
                                  "border-top-right-radius: 0px;"
                                  "border-top-left-radius: 0px;"
                                  "qproperty-alignment: AlignCenter;"
                                  "padding: 5px;"
                                  "}")
                              .arg(Jenkins::resultColor(stage.result).name()));
      mTempWidgets.append(time);

      const auto stageLayout = new QVBoxLayout();
      stageLayout->setContentsMargins(QMargins());
      stageLayout->setSpacing(0);
      stageLayout->addWidget(label);
      stageLayout->addWidget(time);

      layout->addLayout(stageLayout);
   }

   layout->addStretch();
}

void JenkinsJobPanel::requestFile(const JenkinsJobBuildInfo &build)
{
   if (mTabBuildMap.contains(build.number))
      mTabWidget->setCurrentIndex(mTabBuildMap.value(build.number));
   else
   {
      auto urlStr = build.url;
      urlStr.append("/consoleText");
      QNetworkRequest request(urlStr);

      if (!mConfig.user.isEmpty() && !mConfig.token.isEmpty())
         request.setRawHeader(QByteArray("Authorization"),
                              QString("Basic %1:%2").arg(mConfig.user, mConfig.token).toLocal8Bit().toBase64());

      const auto reply = mManager->get(request);
      connect(reply, &QNetworkReply::finished, this, [this, number = build.number]() { storeFile(number); });
   }
}

void JenkinsJobPanel::storeFile(int buildNumber)
{
   const auto reply = qobject_cast<QNetworkReply *>(sender());
   const auto data = reply->readAll();

   if (!data.isEmpty())
   {

      const auto text = new QPlainTextEdit(QString::fromUtf8(data));
      text->setReadOnly(true);
      text->setObjectName("JenkinsOutput");
      mTempWidgets.append(text);

      const auto find = new QLineEdit();
      find->setPlaceholderText(tr("Find text... "));
      connect(find, &QLineEdit::editingFinished, this, [this, text, find]() { findString(find->text(), text); });
      mTempWidgets.append(find);

      const auto frame = new QFrame();
      frame->setObjectName("JenkinsOutput");

      const auto layout = new QVBoxLayout(frame);
      layout->setContentsMargins(10, 10, 10, 10);
      layout->setSpacing(10);
      layout->addWidget(find);
      layout->addWidget(text);

      const auto index = mTabWidget->addTab(frame, QString("Output for #%1").arg(buildNumber));
      mTabWidget->setCurrentIndex(index);

      mTabBuildMap.insert(buildNumber, index);
   }

   reply->deleteLater();
}

void JenkinsJobPanel::findString(QString s, QPlainTextEdit *textEdit)
{
   QTextCursor cursor = textEdit->textCursor();
   QTextCursor cursorSaved = cursor;

   if (!textEdit->find(s))
   {
      cursor.movePosition(QTextCursor::Start);
      textEdit->setTextCursor(cursor);

      if (!textEdit->find(s))
      {
         textEdit->setTextCursor(cursorSaved);

         QMessageBox::information(this, tr("Text not found"), tr("Text not found."));
      }
   }
}

}
