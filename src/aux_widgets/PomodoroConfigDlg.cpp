#include "PomodoroConfigDlg.h"
#include "ui_PomodoroConfigDlg.h"

#include <GitBase.h>
#include <GitQlientSettings.h>

PomodoroConfigDlg::PomodoroConfigDlg(const QSharedPointer<GitBase> &git, QWidget *parent)
   : QDialog(parent)
   , ui(new Ui::PomodoroConfigDlg)
   , mGit(git)
{
   ui->setupUi(this);

   connect(ui->pomodoroDur, &QSlider::valueChanged, this,
           [this](int value) { ui->pomodoroDurLabel->setText(QString::number(value)); });
   connect(ui->breakDur, &QSlider::valueChanged, this,
           [this](int value) { ui->pomodoroBreakDurLabel->setText(QString::number(value)); });
   connect(ui->longBreakDur, &QSlider::valueChanged, this,
           [this](int value) { ui->pomodoroLongBreakLabel->setText(QString::number(value)); });

   GitQlientSettings settings;
   ui->cbAlarmSound->setChecked(settings.localValue(mGit->getGitDir(), "Pomodoro/Alarm", false).toBool());
   ui->cbStopResets->setChecked(
       settings.localValue(mGit->getGitDir(), "Pomodoro/StopResets", true).toBool());
   ui->pomodoroDur->setValue(settings.localValue(mGit->getGitDir(), "Pomodoro/Duration", 25).toInt());
   ui->breakDur->setValue(settings.localValue(mGit->getGitDir(), "Pomodoro/Break", 5).toInt());
   ui->longBreakDur->setValue(settings.localValue(mGit->getGitDir(), "Pomodoro/LongBreak", 15).toInt());
   ui->sbLongBreakCount->setValue(
       settings.localValue(mGit->getGitDir(), "Pomodoro/LongBreakTrigger", 4).toInt());
}

PomodoroConfigDlg::~PomodoroConfigDlg()
{
   delete ui;
}

void PomodoroConfigDlg::accept()
{
   GitQlientSettings settings;
   settings.setLocalValue(mGit->getGitDir(), "Pomodoro/Alarm", ui->cbAlarmSound->isChecked());
   settings.setLocalValue(mGit->getGitDir(), "Pomodoro/StopResets", ui->cbStopResets->isChecked());
   settings.setLocalValue(mGit->getGitDir(), "Pomodoro/Duration", ui->pomodoroDur->value());
   settings.setLocalValue(mGit->getGitDir(), "Pomodoro/Break", ui->breakDur->value());
   settings.setLocalValue(mGit->getGitDir(), "Pomodoro/LongBreak", ui->longBreakDur->value());
   settings.setLocalValue(mGit->getGitDir(), "Pomodoro/LongBreakTrigger", ui->sbLongBreakCount->value());

   QDialog::accept();
}
