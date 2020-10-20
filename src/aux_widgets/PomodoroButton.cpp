#include "PomodoroButton.h"

#include <GitQlientSettings.h>
#include <GitBase.h>

#include <QTime>
#include <QToolButton>
#include <QLabel>
#include <QMenu>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QTimer>
#include <QMessageBox>

PomodoroButton::PomodoroButton(const QSharedPointer<GitBase> &git, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mButton(new QToolButton())
   , mArrow(new QToolButton())
   , mCounter(new QLabel())
   , mTimer(new QTimer(this))
{
   setContentsMargins(0, 0, 0, 0);
   setToolTip(tr("Pomodoro"));

   const auto menu = new QMenu(mButton);
   menu->installEventFilter(this);

   mStartAction = menu->addAction(tr("Start"));
   mStopAction = menu->addAction(tr("Stop"));
   mRestartAction = menu->addAction(tr("Restart"));
   menu->addSeparator();
   mConfigAction = menu->addAction(tr("Configuration"));

   mButton->setIcon(QIcon(":/icons/pomodoro"));
   mButton->setIconSize(QSize(22, 22));
   mButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
   mButton->setObjectName("ToolButtonAboveMenu");
   connect(mButton, &QToolButton::clicked, this, &PomodoroButton::onClick);

   mArrow->setObjectName("Arrow");
   mArrow->setIcon(QIcon(":/icons/arrow_down"));
   mArrow->setIconSize(QSize(10, 10));
   mArrow->setToolButtonStyle(Qt::ToolButtonIconOnly);
   mArrow->setToolTip(tr("Options"));
   mArrow->setPopupMode(QToolButton::InstantPopup);
   mArrow->setMenu(menu);
   mArrow->setFixedWidth(10);
   mArrow->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

   GitQlientSettings settings;
   const auto durationMins = settings.localValue(mGit->getGitQlientSettingsDir(), "Pomodoro/Duration", 25).toInt();
   mDurationTime = QTime(0, durationMins, 0);
   mCounter->setText(mDurationTime.toString("mm:ss"));

   const auto breakMins = settings.localValue(mGit->getGitQlientSettingsDir(), "Pomodoro/Break", 5).toInt();
   mBreakTime = QTime(0, breakMins, 0);

   const auto longBreakMins = settings.localValue(mGit->getGitQlientSettingsDir(), "Pomodoro/LongBreak", 15).toInt();
   mLongBreakTime = QTime(0, longBreakMins, 0);

   mBigBreakCount = settings.localValue(mGit->getGitQlientSettingsDir(), "Pomodoro/LongBreakTrigger", 4).toInt();

   mTimer->setInterval(1000);
   connect(mTimer, &QTimer::timeout, this, &PomodoroButton::onTimeout);

   const auto layout = new QGridLayout(this);
   layout->setContentsMargins(QMargins());
   layout->setSpacing(0);
   layout->addWidget(mButton, 0, 0);
   layout->addWidget(mCounter, 1, 0);
   layout->addWidget(mArrow, 0, 1, 2, 1);
}

void PomodoroButton::setText(const QString &text)
{
   mCounter->setText(text);
}

void PomodoroButton::updateCounters()
{
   mState = State::OnHold;
   mTimer->stop();
   mButton->setIcon(QIcon(":/icons/pomodoro"));

   GitQlientSettings settings;

   const auto durationMins = settings.localValue(mGit->getGitQlientSettingsDir(), "Pomodoro/Duration", 25).toInt();
   mDurationTime = QTime(0, durationMins, 0);
   mCounter->setText(mDurationTime.toString("mm:ss"));

   const auto breakMins = settings.localValue(mGit->getGitQlientSettingsDir(), "Pomodoro/Break", 5).toInt();
   mBreakTime = QTime(0, breakMins, 0);

   const auto longBreakMins = settings.localValue(mGit->getGitQlientSettingsDir(), "Pomodoro/LongBreak", 15).toInt();
   mLongBreakTime = QTime(0, longBreakMins, 0);

   const auto longBreakTriggerCount
       = settings.localValue(mGit->getGitQlientSettingsDir(), "Pomodoro/LongBreakTrigger", 4).toInt();

   if (longBreakTriggerCount < mBigBreakOriginalValue)
   {
      mBigBreakOriginalValue = longBreakTriggerCount;
      mBigBreakCount = longBreakTriggerCount;
   }
   else
   {
      mBigBreakCount = longBreakTriggerCount - (mBigBreakOriginalValue - mBigBreakCount);
      mBigBreakOriginalValue = longBreakTriggerCount;
   }
}

void PomodoroButton::mousePressEvent(QMouseEvent *e)
{
   if (isEnabled())
      mPressed = true;

   QFrame::mousePressEvent(e);
}

void PomodoroButton::mouseReleaseEvent(QMouseEvent *e)
{
   if (isEnabled() && mPressed)
   {
      onClick();

      emit clicked();
   }

   QFrame::mouseReleaseEvent(e);
}

bool PomodoroButton::eventFilter(QObject *obj, QEvent *event)
{
   if (const auto menu = qobject_cast<QMenu *>(obj); menu && event->type() == QEvent::Show)
   {
      auto localPos = mButton->pos();
      localPos.setX(localPos.x());
      auto pos = mapToGlobal(localPos);
      menu->show();
      pos.setY(pos.y() + height());
      menu->move(pos);

      return true;
   }

   return false;
}

void PomodoroButton::onTimeout()
{
   if (mState == State::Running)
      onRunningMode();
   else if (mState == State::InBreakRunning)
      onBreakingMode();
   else if (mState == State::InLongBreakRunning)
      onLongBreakingMode();
}

void PomodoroButton::onClick()
{
   switch (mState)
   {
      case State::OnHold:
      case State::Finished:
         mState = State::Running;
         mTimer->start();
         mButton->setIcon(QIcon(":/icons/pomodoro_running"));
         --mBigBreakCount;
         break;
      case State::InBreak:
         mState = State::InBreakRunning;
         mTimer->start();
         break;
      case State::InLongBreak:
         mState = State::InBreakRunning;
         mTimer->start();
         break;
      case State::InBreakRunning:
      case State::InLongBreakRunning:
      case State::Running:
         mState = State::OnHold;
         mTimer->stop();
         mButton->setIcon(QIcon(":/icons/pomodoro"));
         ++mBigBreakCount;
         break;
   }
}

void PomodoroButton::onRunningMode()
{
   mDurationTime = mDurationTime.addSecs(-1);
   const auto timeStr = mDurationTime.toString("mm:ss");
   mCounter->setText(timeStr);

   if (mDurationTime == QTime(0, 0, 0))
   {
      mTimer->stop();

      GitQlientSettings settings;
      const auto durationMins = settings.localValue(mGit->getGitQlientSettingsDir(), "Pomodoro/Duration", 25).toInt();
      mDurationTime = QTime(0, durationMins, 0);

      mButton->setIcon(QIcon(":/icons/pomodoro_timeout"));

      if (mBigBreakCount == 0)
      {
         mBigBreakCount = settings.localValue(mGit->getGitQlientSettingsDir(), "Pomodoro/LongBreakTrigger", 4).toInt();

         mCounter->setText(mLongBreakTime.toString("mm:ss"));
      }
      else
         mCounter->setText(mBreakTime.toString("mm:ss"));

      if (QMessageBox::question(this, tr("Time for a break!"), tr("It's time to do a break. Are you ready?"))
          == QMessageBox::Yes)
      {
         if (mBigBreakCount == 0)
            mState = State::InLongBreakRunning;
         else
            mState = State::InBreakRunning;

         mTimer->start();
      }
      else
      {
         if (mBigBreakCount == 0)
            mState = State::InLongBreak;
         else
            mState = State::InBreak;
      }
   }
}

void PomodoroButton::onBreakingMode()
{
   mBreakTime = mBreakTime.addSecs(-1);
   mCounter->setText(mBreakTime.toString("mm:ss"));

   if (mBreakTime == QTime(0, 0, 0))
   {
      mTimer->stop();

      GitQlientSettings settings;
      const auto breakMins = settings.localValue(mGit->getGitQlientSettingsDir(), "Pomodoro/Break", 5).toInt();
      mBreakTime = QTime(0, breakMins, 0);

      mState = State::Finished;

      const auto answer
          = QMessageBox::question(this, tr("Time to work!"), tr("It's time to go back to work. Are you ready?"));

      if (answer == QMessageBox::Yes)
      {
         mCounter->setText(mDurationTime.toString("mm:ss"));
         mState = State::Running;
         mTimer->start();
         mButton->setIcon(QIcon(":/icons/pomodoro_running"));
         --mBigBreakCount;
      }
   }
}

void PomodoroButton::onLongBreakingMode()
{
   mLongBreakTime = mLongBreakTime.addSecs(-1);
   mCounter->setText(mLongBreakTime.toString("mm:ss"));

   if (mLongBreakTime == QTime(0, 0, 0))
   {
      mTimer->stop();

      GitQlientSettings settings;
      const auto breakMins = settings.localValue(mGit->getGitQlientSettingsDir(), "Pomodoro/LongBreak", 15).toInt();
      mLongBreakTime = QTime(0, breakMins, 0);

      mState = State::Finished;

      const auto answer
          = QMessageBox::question(this, tr("Time to work!"), tr("It's time to go back to work. Are you ready?"));

      if (answer == QMessageBox::Yes)
      {
         mCounter->setText(mDurationTime.toString("mm:ss"));
         mState = State::Running;
         mTimer->start();
         mButton->setIcon(QIcon(":/icons/pomodoro_running"));
      }
   }
}
