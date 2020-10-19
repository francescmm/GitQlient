#include "PomodoroButton.h"

#include <QToolButton>
#include <QLabel>
#include <QMenu>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QStyle>

PomodoroButton::PomodoroButton(QWidget *parent)
   : QFrame(parent)
   , mButton(new QToolButton())
   , mArrow(new QToolButton())
   , mCounter(new QLabel())
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

   mArrow->setObjectName("Arrow");
   mArrow->setIcon(QIcon(":/icons/arrow_down"));
   mArrow->setIconSize(QSize(10, 10));
   mArrow->setToolButtonStyle(Qt::ToolButtonIconOnly);
   mArrow->setToolTip(tr("Options"));
   mArrow->setPopupMode(QToolButton::InstantPopup);
   mArrow->setMenu(menu);
   mArrow->setFixedWidth(10);
   mArrow->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

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
