#include "ConfigWidget.h"

#include <CreateRepoDlg.h>
#include <git.h>

#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QFileDialog>
#include <QSettings>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QStyle>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>

ConfigWidget::ConfigWidget(QWidget *parent)
   : QFrame(parent)
   , mGit(new Git())
   , mOpenRepo(new QPushButton(tr("Open existing repo")))
   , mCloneRepo(new QPushButton(tr("Clone new repo")))
   , mInitRepo(new QPushButton(tr("Init new repo")))
{
   mOpenRepo->setObjectName("bigButton");
   mCloneRepo->setObjectName("bigButton");
   mInitRepo->setObjectName("bigButton");

   const auto line = new QFrame();
   line->setObjectName("separator");

   // Adding buttons to open or init repos
   const auto repoSubtitle = new QLabel(tr("Repository options"));
   repoSubtitle->setObjectName("subtitle");

   const auto repoOptionsFrame = new QFrame();
   const auto repoOptionsLayout = new QVBoxLayout(repoOptionsFrame);
   repoOptionsLayout->setSpacing(20);
   repoOptionsLayout->setContentsMargins(QMargins());
   repoOptionsLayout->addWidget(repoSubtitle);
   repoOptionsLayout->addWidget(mOpenRepo);
   repoOptionsLayout->addWidget(mCloneRepo);
   repoOptionsLayout->addWidget(mInitRepo);
   repoOptionsLayout->addWidget(line);

   QSettings s;
   const auto mostUsedRepos = s.value("lastUsedRepos", QStringList()).toStringList();
   for (const auto &repo : mostUsedRepos)
   {
      const auto usedRepo = new QPushButton(repo);
      usedRepo->setToolTip(repo);
      repoOptionsLayout->addWidget(usedRepo);
      connect(usedRepo, &QPushButton::clicked, this,
              [this]() { emit signalOpenRepo(dynamic_cast<QPushButton *>(sender())->toolTip()); });
   }

   repoOptionsLayout->addStretch();

   const auto usedSubtitle = new QLabel(tr("Configuration"));
   usedSubtitle->setObjectName("subtitle");

   const auto configFrame = new QFrame();
   const auto configLayout = new QVBoxLayout(configFrame);
   configLayout->setContentsMargins(QMargins());
   configLayout->setSpacing(20);
   configLayout->addWidget(usedSubtitle);
   configLayout->addWidget(createConfigWidget());
   configLayout->addStretch();

   const auto widgetsLayout = new QHBoxLayout();
   widgetsLayout->setContentsMargins(QMargins());
   widgetsLayout->setSpacing(150);
   widgetsLayout->addWidget(repoOptionsFrame);
   widgetsLayout->addWidget(configFrame);

   const auto title = new QLabel(tr("Welcome to GitQlient"));
   title->setObjectName("title");

   const auto lineTitle = new QFrame();
   lineTitle->setObjectName("separator");

   const auto centerLayout = new QVBoxLayout();
   centerLayout->setSpacing(20);
   centerLayout->setContentsMargins(QMargins());
   centerLayout->addWidget(title);
   centerLayout->addWidget(lineTitle);
   centerLayout->addLayout(widgetsLayout);
   centerLayout->addStretch();

   const auto layout = new QGridLayout(this);
   layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 0, 0);
   layout->addLayout(centerLayout, 1, 1);
   layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 2, 2);

   connect(mOpenRepo, &QPushButton::clicked, this, &ConfigWidget::openRepo);
   connect(mCloneRepo, &QPushButton::clicked, this, &ConfigWidget::cloneRepo);
   connect(mInitRepo, &QPushButton::clicked, this, &ConfigWidget::initRepo);
}

void ConfigWidget::openRepo()
{
   const QString dirName(QFileDialog::getExistingDirectory(this, "Choose the directory of a Git project"));

   if (!dirName.isEmpty())
   {
      QDir d(dirName);
      emit signalOpenRepo(d.absolutePath());
   }
}

void ConfigWidget::cloneRepo()
{
   CreateRepoDlg cloneDlg(CreateRepoDlgType::CLONE, mGit);

   connect(&cloneDlg, &CreateRepoDlg::signalRepoCloned, this, &ConfigWidget::signalOpenRepo);

   cloneDlg.exec();
}

void ConfigWidget::initRepo()
{
   CreateRepoDlg cloneDlg(CreateRepoDlgType::INIT, mGit);

   connect(&cloneDlg, &CreateRepoDlg::signalRepoCloned, this, &ConfigWidget::signalOpenRepo);

   cloneDlg.exec();
}

QWidget *ConfigWidget::createConfigWidget()
{
   mBtnGroup = new QButtonGroup();
   mBtnGroup->addButton(new QPushButton(tr("General")), 0);
   mBtnGroup->addButton(new QPushButton(tr("Git config")), 1);

   const auto firstBtn = mBtnGroup->button(0);
   firstBtn->setProperty("selected", true);
   firstBtn->style()->unpolish(firstBtn);
   firstBtn->style()->polish(firstBtn);

   const auto buttons = mBtnGroup->buttons();
   const auto buttonsLayout = new QVBoxLayout();
   buttonsLayout->setContentsMargins(QMargins());

   auto count = 0;
   for (auto btn : buttons)
   {
      buttonsLayout->addWidget(btn);

      if (count < buttons.count() - 1)
      {
         ++count;
         const auto line = new QFrame();
         line->setObjectName("separator2px");
         buttonsLayout->addWidget(line);
      }
   }

   buttonsLayout->addStretch();

   const auto stackedWidget = new QStackedWidget();
   stackedWidget->setMinimumHeight(300);
   stackedWidget->addWidget(createGeneralPage());
   stackedWidget->addWidget(createConfigPage());
   stackedWidget->setCurrentIndex(0);

   connect(mBtnGroup, qOverload<int>(&QButtonGroup::buttonClicked), this, [this, stackedWidget](int index) {
      const auto selectedBtn = mBtnGroup->button(index);
      const auto buttons = mBtnGroup->buttons();

      for (auto btn : buttons)
      {
         btn->setProperty("selected", selectedBtn == btn);
         btn->style()->unpolish(btn);
         btn->style()->polish(btn);
      }

      stackedWidget->setCurrentIndex(index);
   });

   const auto tabWidget = new QFrame();
   tabWidget->setObjectName("tabWidget");
   const auto layout = new QHBoxLayout(tabWidget);
   layout->setSpacing(0);
   layout->setContentsMargins(QMargins());
   layout->addLayout(buttonsLayout);
   layout->addWidget(stackedWidget);

   return tabWidget;
}

#include <QTimer>
QWidget *ConfigWidget::createGeneralPage()
{
   QSettings settings;

   const auto autoFetch = new QSpinBox();
   autoFetch->setRange(0, 60);
   autoFetch->setValue(settings.value("autoFetch", 0).toInt());

   const auto labelAutoFetch = new QLabel(tr("The interval is expected to be in minutes. "
                                             "Choose a value between 0 (for disabled) and 60"));
   labelAutoFetch->setWordWrap(true);

   const auto fetchLayout = new QVBoxLayout();
   fetchLayout->setAlignment(Qt::AlignTop);
   fetchLayout->setContentsMargins(QMargins());
   fetchLayout->setSpacing(0);
   fetchLayout->addWidget(autoFetch);
   fetchLayout->addWidget(labelAutoFetch);

   const auto fetchLayoutLabel = new QVBoxLayout();
   fetchLayoutLabel->setAlignment(Qt::AlignTop);
   fetchLayoutLabel->setContentsMargins(QMargins());
   fetchLayoutLabel->setSpacing(0);
   fetchLayoutLabel->addWidget(new QLabel(tr("Auto-Fetch interval")));
   fetchLayoutLabel->addStretch();

   const auto autoPrune = new QCheckBox();
   autoPrune->setChecked(settings.value("autoPrune", true).toBool());

   const auto disableLogs = new QCheckBox();
   disableLogs->setChecked(settings.value("logsDisabled", false).toBool());

   const auto levelCombo = new QComboBox();
   levelCombo->addItems({ "Trace", "Debug", "Info", "Warning", "Error", "Fatal" });
   levelCombo->setCurrentIndex(settings.value("logsLevel", 2).toInt());

   const auto autoFormat = new QCheckBox(tr(" (needs clang-format)"));
   autoFormat->setChecked(settings.value("autoFormat", true).toBool());

   const auto lStatus = new QLabel();

   const auto pbReset = new QPushButton(tr("Reset"));
   connect(pbReset, &QPushButton::clicked, this,
           [autoFetch, autoPrune, disableLogs, levelCombo, autoFormat, lStatus]() {
              QSettings settings;
              autoFetch->setValue(settings.value("autoFetch", 0).toInt());
              autoPrune->setChecked(settings.value("autoPrune", true).toBool());
              disableLogs->setChecked(settings.value("logsDisabled", false).toBool());
              levelCombo->setCurrentIndex(settings.value("logsLevel", 2).toInt());
              autoFormat->setChecked(settings.value("autoFormat", true).toBool());

              QTimer::singleShot(3000, [lStatus]() { lStatus->setText(""); });
              lStatus->setText(tr("Changes reseted"));
           });

   const auto pbApply = new QPushButton(tr("Apply"));
   connect(pbApply, &QPushButton::clicked, this,
           [autoFetch, autoPrune, disableLogs, levelCombo, autoFormat, lStatus]() {
              QSettings settings;
              settings.setValue("autoFetch", autoFetch->value());
              settings.setValue("autoPrune", autoPrune->isChecked());
              settings.setValue("logsDisabled", disableLogs->isChecked());
              settings.setValue("logsLevel", levelCombo->currentIndex());
              settings.setValue("autoFormat", autoFormat->isChecked());

              QTimer::singleShot(3000, [lStatus]() { lStatus->setText(""); });
              lStatus->setText(tr("Changes applied"));
           });

   const auto buttonsLayout = new QHBoxLayout();
   buttonsLayout->setContentsMargins(QMargins());
   buttonsLayout->setSpacing(0);
   buttonsLayout->addWidget(pbReset);
   buttonsLayout->addStretch();
   buttonsLayout->addWidget(lStatus);
   buttonsLayout->addStretch();
   buttonsLayout->addWidget(pbApply);

   const auto frame = new QFrame();
   frame->setObjectName("configPage");
   const auto layout = new QGridLayout(frame);
   layout->setContentsMargins(20, 20, 20, 20);
   layout->setSpacing(20);
   layout->setAlignment(Qt::AlignTop);
   layout->addLayout(fetchLayoutLabel, 0, 0);
   layout->addLayout(fetchLayout, 0, 1);
   layout->addWidget(new QLabel(tr("Auto-Prune")), 2, 0);
   layout->addWidget(autoPrune, 2, 1);
   layout->addWidget(new QLabel(tr("Disable logs")), 3, 0);
   layout->addWidget(disableLogs, 3, 1);
   layout->addWidget(new QLabel(tr("Set log level")), 4, 0);
   layout->addWidget(levelCombo, 4, 1);
   layout->addWidget(new QLabel(tr("Auto-Format files")), 5, 0);
   layout->addWidget(autoFormat, 5, 1);
   layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding), 6, 0, 1, 2);
   layout->addLayout(buttonsLayout, 7, 0, 1, 2);

   return frame;
}

QWidget *ConfigWidget::createConfigPage()
{
   const auto frame = new QFrame();
   frame->setObjectName("configPage");

   return frame;
}
