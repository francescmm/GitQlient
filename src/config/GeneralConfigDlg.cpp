#include "GeneralConfigDlg.h"

#include <GitQlientSettings.h>
#include <GitQlientStyles.h>
#include <QLogger.h>
#include <CheckBox.h>

#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QMessageBox>

using namespace QLogger;

GeneralConfigDlg::GeneralConfigDlg(QWidget *parent)
   : QDialog(parent)
   , mDisableLogs(new CheckBox())
   , mLevelCombo(new QComboBox())
   , mStylesSchema(new QComboBox())
   , mClose(new QPushButton(tr("Close")))
   , mReset(new QPushButton(tr("Reset")))
   , mApply(new QPushButton(tr("Apply")))

{
   mClose->setMinimumWidth(75);
   mReset->setMinimumWidth(75);
   mApply->setMinimumWidth(75);

   GitQlientSettings settings;

   mDisableLogs->setChecked(settings.globalValue("logsDisabled", false).toBool());

   mLevelCombo->addItems({ "Trace", "Debug", "Info", "Warning", "Error", "Fatal" });
   mLevelCombo->setCurrentIndex(settings.globalValue("logsLevel", 2).toInt());

   const auto currentStyle = settings.globalValue("colorSchema", "dark").toString();
   mStylesSchema->addItems({ "dark", "bright" });
   mStylesSchema->setCurrentText(currentStyle);
   connect(mStylesSchema, &QComboBox::currentTextChanged, this, [this, currentStyle](const QString &newText) {
      if (newText != currentStyle)
         mShowResetMsg = true;
   });

   connect(mClose, &QPushButton::clicked, this, &GeneralConfigDlg::close);
   connect(mReset, &QPushButton::clicked, this, &GeneralConfigDlg::resetChanges);
   connect(mApply, &QPushButton::clicked, this, &GeneralConfigDlg::accept);

   const auto buttonsLayout = new QHBoxLayout();
   buttonsLayout->setContentsMargins(QMargins());
   buttonsLayout->setSpacing(20);
   buttonsLayout->addWidget(mClose);
   buttonsLayout->addStretch();
   buttonsLayout->addWidget(mReset);
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

   setFixedSize(400, 200);

   setStyleSheet(GitQlientStyles::getStyles());
}

void GeneralConfigDlg::resetChanges()
{
   GitQlientSettings settings;
   mDisableLogs->setChecked(settings.globalValue("logsDisabled", false).toBool());
   mLevelCombo->setCurrentIndex(settings.globalValue("logsLevel", 2).toInt());
   mStylesSchema->setCurrentText(settings.globalValue("colorSchema", "bright").toString());
}

void GeneralConfigDlg::accept()
{
   GitQlientSettings settings;
   settings.setGlobalValue("logsDisabled", mDisableLogs->isChecked());
   settings.setGlobalValue("logsLevel", mLevelCombo->currentIndex());
   settings.setGlobalValue("colorSchema", mStylesSchema->currentText());

   if (mShowResetMsg)
      QMessageBox::information(this, tr("Reset needed!"),
                               tr("You need to restart GitQlient to see the changes in the styles applid."));

   const auto logger = QLoggerManager::getInstance();
   logger->overwriteLogLevel(static_cast<LogLevel>(mLevelCombo->currentIndex()));

   if (mDisableLogs->isChecked())
      logger->pause();
   else
      logger->resume();

   QDialog::accept();
}
