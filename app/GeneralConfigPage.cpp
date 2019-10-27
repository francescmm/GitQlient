#include "GeneralConfigPage.h"

#include <QSettings>
#include <QTimer>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>

GeneralConfigPage::GeneralConfigPage(QWidget *parent)
   : QFrame(parent)
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

   const auto layout = new QGridLayout(this);
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
}
