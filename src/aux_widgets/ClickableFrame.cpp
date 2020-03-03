#include "ClickableFrame.h"

#include <QMouseEvent>
#include <QVBoxLayout>
#include <QLabel>

ClickableFrame::ClickableFrame(QWidget *parent)
   : QFrame(parent)
{
   setAttribute(Qt::WA_DeleteOnClose);
}

ClickableFrame::ClickableFrame(const QString &text, Qt::Alignment alignment, QWidget *parent)
   : QFrame(parent)
{
   const auto layout = new QVBoxLayout(this);
   layout->setContentsMargins(2, 2, 2, 2);
   layout->setSpacing(0);
   layout->addWidget(new QLabel(text));
   layout->setAlignment(alignment);
}

void ClickableFrame::mousePressEvent(QMouseEvent *e)
{
   mPressed = rect().contains(e->pos()) && e->button() == Qt::LeftButton;
}

void ClickableFrame::mouseReleaseEvent(QMouseEvent *e)
{
   if (mPressed && rect().contains(e->pos()) && e->button() == Qt::LeftButton)
      emit clicked();
}
