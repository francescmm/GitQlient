#pragma once

#include <QFrame>
#include <QTime>

class QToolButton;
class QLabel;
class QAction;
class QTimer;
class GitBase;

class PomodoroButton : public QFrame
{
   Q_OBJECT

signals:
   /**
    * @brief Signal when the link has been clicked
    *
    */
   void clicked();

public:
   /**
    * @brief PomodoroButton Overloaded constructor.
    * @param parent The parent widget.
    */
   explicit PomodoroButton(const QSharedPointer<GitBase> &git, QWidget *parent = nullptr);

   /**
    * @brief setText Sets the text to the button.
    * @param text The new text.
    */
   void setText(const QString &text);

protected:
   /**
    *
    *@brief Event that processes whether the user  presses or not the mouse
    *
    * @param e The event
    */
   void mousePressEvent(QMouseEvent *e) override;

   /**
    * @brief Event that processes whether the user releases the mouse button or not
    *
    * @param event The event
    */
   void mouseReleaseEvent(QMouseEvent *e) override;

   /**
    * @brief eventFilter Event filter method to position the menu of the tool button in a custom position.
    * @param obj The object to filter, in this case QMenu.
    * @param event The event to filter, in this case QEvent::Show.
    * @return Returns true if filtered.
    */
   bool eventFilter(QObject *obj, QEvent *event) override;

private:
   enum class State
   {
      OnHold,
      Running,
      InBreak,
      InBreakRunning,
      InLongBreak,
      InLongBreakRunning,
      Finished
   };
   QTime mDurationTime;
   QTime mBreakTime;
   QTime mLongBreakTime;
   bool mPressed = false;
   int mBigBreakCount = 0;
   State mState = State::OnHold;
   QSharedPointer<GitBase> mGit;
   QToolButton *mButton = nullptr;
   QToolButton *mArrow = nullptr;
   QLabel *mCounter = nullptr;
   QTimer *mTimer = nullptr;
   QAction *mStopAction = nullptr;
   QAction *mStartAction = nullptr;
   QAction *mRestartAction = nullptr;
   QAction *mConfigAction = nullptr;

   void onTimeout();
   void onClick();
   void onRunningMode();
   void onBreakingMode();
   void onLongBreakingMode();
};
