#pragma once

#include <QFrame>

class QPushButton;

class DiffButton : public QFrame
{
   Q_OBJECT

signals:
   void clicked();

public:
   explicit DiffButton(const QString &text, const QString &icon, QWidget *parent = nullptr);
   void setSelected();
   void setUnselected();

protected:
   void mousePressEvent(QMouseEvent *e) override;

private:
   bool mPressed = false;
   QPushButton *mCloseBtn = nullptr;
};
