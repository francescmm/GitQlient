#ifndef CONFIG_H
#define CONFIG_H

#include <QWidget>

class QPushButton;

class ConfigWidget : public QWidget
{
   Q_OBJECT
public:
   explicit ConfigWidget(QWidget *parent = nullptr);

private:
   QPushButton *mOpenRepo = nullptr;
   QPushButton *mCloneRepo = nullptr;
   QPushButton *mInitRepo = nullptr;
};

#endif // CONFIG_H
