#include "GeneralConfigPage.h"

#include <GitQlientSettings.h>
#include <QLogger.h>
#include <CheckBox.h>

#include <QTimer>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>

using namespace QLogger;

GeneralConfigPage::GeneralConfigPage(QWidget *parent)
   : QFrame(parent)
   , mDisableLogs(new CheckBox())
   , mLevelCombo(new QComboBox())
   , mStatusLabel(new QLabel())
   , mStylesSchema(new QComboBox())
   , mReset(new QPushButton(tr("Reset")))
   , mApply(new QPushButton(tr("Apply")))

{
   setAttribute(Qt::WA_DeleteOnClose);

   GitQlientSettings settings;

   mDisableLogs->setChecked(settings.globalValue("logsDisabled", false).toBool());

   mLevelCombo->addItems({ "Trace", "Debug", "Info", "Warning", "Error", "Fatal" });
   mLevelCombo->setCurrentIndex(settings.globalValue("logsLevel", 2).toInt());

   mStylesSchema->addItems({ "dark", "bright" });
   mStylesSchema->setCurrentText(settings.globalValue("colorSchema", "bright").toString());

   mStatusLabel->setObjectName("configLabel");

   connect(mReset, &QPushButton::clicked, this, &GeneralConfigPage::resetChanges);
   connect(mApply, &QPushButton::clicked, this, &GeneralConfigPage::applyChanges);

   const auto buttonsLayout = new QHBoxLayout();
   buttonsLayout->setContentsMargins(QMargins());
   buttonsLayout->setSpacing(0);
   buttonsLayout->addWidget(mReset);
   buttonsLayout->addStretch();
   buttonsLayout->addWidget(mStatusLabel);
   buttonsLayout->addStretch();
   buttonsLayout->addWidget(mApply);

   auto row = 0;
   const auto layout = new QGridLayout(this);
   layout->setContentsMargins(20, 20, 20, 20);
   layout->setSpacing(20);
   layout->setAlignment(Qt::AlignTop);
   layout->addWidget(new QLabel(tr("Disable logs")), row, 0);
   layout->addWidget(mDisableLogs, row, 1);
   layout->addWidget(new QLabel(tr("Set log level")), ++row, 0);
   layout->addWidget(mLevelCombo, row, 1);
   layout->addWidget(new QLabel(tr("Styles schema")), ++row, 0);
   layout->addWidget(mStylesSchema, row, 1);
   layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding), ++row, 0, 1, 2);
   layout->addLayout(buttonsLayout, ++row, 0, 1, 2);
}

void GeneralConfigPage::resetChanges()
{
   GitQlientSettings settings;
   mDisableLogs->setChecked(settings.globalValue("logsDisabled", false).toBool());
   mLevelCombo->setCurrentIndex(settings.globalValue("logsLevel", 2).toInt());
   mStylesSchema->setCurrentText(settings.globalValue("colorSchema", "bright").toString());

   QTimer::singleShot(3000, mStatusLabel, &QLabel::clear);

   mStatusLabel->setText(tr("Changes reseted."));
}

void GeneralConfigPage::applyChanges()
{
   GitQlientSettings settings;
   settings.setGlobalValue("logsDisabled", mDisableLogs->isChecked());
   settings.setGlobalValue("logsLevel", mLevelCombo->currentIndex());
   settings.setGlobalValue("colorSchema", mStylesSchema->currentText());

   QTimer::singleShot(3000, mStatusLabel, &QLabel::clear);
   mStatusLabel->setText(tr("Changes applied! \n Reset is needed if the color schema changed."));

   const auto logger = QLoggerManager::getInstance();
   logger->overwriteLogLevel(static_cast<LogLevel>(mLevelCombo->currentIndex()));

   if (mDisableLogs->isChecked())
      logger->pause();
   else
      logger->resume();
}
