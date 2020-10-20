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

   bool eventFilter(QObject *obj, QEvent *event) override;

private:
   enum class State
   {
      OnHold,
      Running,
      InBreak,
      Finished
   };
   QTime mDurationTime;
   QTime mBreakTime;
   QTime mLongBreakTime;
   bool mPressed = false;
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

   void onClick();
};
