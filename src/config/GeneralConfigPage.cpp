#include "GeneralConfigPage.h"

#include <GitQlientSettings.h>
#include <QLogger.h>

#include <QTimer>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>

using namespace QLogger;

GeneralConfigPage::GeneralConfigPage(QWidget *parent)
   : QFrame(parent)
   , mAutoFetch(new QSpinBox())
   , mAutoPrune(new QCheckBox())
   , mDisableLogs(new QCheckBox())
   , mLevelCombo(new QComboBox())
   , mAutoFormat(new QCheckBox(tr(" (needs clang-format)")))
   , mStatusLabel(new QLabel())
   , mReset(new QPushButton(tr("Reset")))
   , mApply(new QPushButton(tr("Apply")))

{
   setAttribute(Qt::WA_DeleteOnClose);

   GitQlientSettings settings;

   mAutoFetch->setRange(0, 60);
   mAutoFetch->setValue(settings.value("autoFetch", 0).toInt());

   const auto labelAutoFetch = new QLabel(tr("The interval is expected to be in minutes. "
                                             "Choose a value between 0 (for disabled) and 60"));
   labelAutoFetch->setWordWrap(true);

   const auto fetchLayout = new QVBoxLayout();
   fetchLayout->setAlignment(Qt::AlignTop);
   fetchLayout->setContentsMargins(QMargins());
   fetchLayout->setSpacing(0);
   fetchLayout->addWidget(mAutoFetch);
   fetchLayout->addWidget(labelAutoFetch);

   const auto fetchLayoutLabel = new QVBoxLayout();
   fetchLayoutLabel->setAlignment(Qt::AlignTop);
   fetchLayoutLabel->setContentsMargins(QMargins());
   fetchLayoutLabel->setSpacing(0);
   fetchLayoutLabel->addWidget(new QLabel(tr("Auto-Fetch interval")));
   fetchLayoutLabel->addStretch();

   mAutoPrune->setChecked(settings.value("autoPrune", true).toBool());

   mDisableLogs->setChecked(settings.value("logsDisabled", false).toBool());

   mLevelCombo->addItems({ "Trace", "Debug", "Info", "Warning", "Error", "Fatal" });
   mLevelCombo->setCurrentIndex(settings.value("logsLevel", 2).toInt());

   mAutoFormat->setChecked(settings.value("autoFormat", true).toBool());

   mExternalEditor = new QLineEdit("nano");

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
   layout->addLayout(fetchLayoutLabel, 0, 0);
   layout->addLayout(fetchLayout, row, 1);
   layout->addWidget(new QLabel(tr("Auto-Prune")), ++row, 0);
   layout->addWidget(mAutoPrune, row, 1);
   layout->addWidget(new QLabel(tr("Disable logs")), ++row, 0);
   layout->addWidget(mDisableLogs, row, 1);
   layout->addWidget(new QLabel(tr("Set log level")), ++row, 0);
   layout->addWidget(mLevelCombo, row, 1);
   layout->addWidget(new QLabel(tr("Auto-Format files")), ++row, 0);
   layout->addWidget(mAutoFormat, row, 1);
   layout->addWidget(new QLabel(tr("External editor")), ++row, 0);
   layout->addWidget(mExternalEditor, row, 1);
   layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding), ++row, 0, 1, 2);
   layout->addLayout(buttonsLayout, ++row, 0, 1, 2);
}

void GeneralConfigPage::resetChanges()
{
   GitQlientSettings settings;
   mAutoFetch->setValue(settings.value("autoFetch", 0).toInt());
   mAutoPrune->setChecked(settings.value("autoPrune", true).toBool());
   mDisableLogs->setChecked(settings.value("logsDisabled", false).toBool());
   mLevelCombo->setCurrentIndex(settings.value("logsLevel", 2).toInt());
   mAutoFormat->setChecked(settings.value("autoFormat", true).toBool());
   mExternalEditor->setText(settings.value("externalEditor", "nano").toString());

   QTimer::singleShot(3000, [this]() { mStatusLabel->setText(""); });
   mStatusLabel->setText(tr("Changes reseted"));
}

void GeneralConfigPage::applyChanges()
{
   GitQlientSettings settings;
   settings.setValue("autoFetch", mAutoFetch->value());
   settings.setValue("autoPrune", mAutoPrune->isChecked());
   settings.setValue("logsDisabled", mDisableLogs->isChecked());
   settings.setValue("logsLevel", mLevelCombo->currentIndex());
   settings.setValue("autoFormat", mAutoFormat->isChecked());
   settings.setValue("externalEditor", mExternalEditor->text());

   QTimer::singleShot(3000, [this]() { mStatusLabel->setText(""); });
   mStatusLabel->setText(tr("Changes applied"));

   const auto logger = QLoggerManager::getInstance();
   logger->overwriteLogLevel(static_cast<LogLevel>(mLevelCombo->currentIndex()));

   if (mDisableLogs->isChecked())
      logger->pause();
   else
      logger->resume();
}
