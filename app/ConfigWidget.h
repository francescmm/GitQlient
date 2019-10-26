#ifndef CONFIG_H
#define CONFIG_H

#include <QWidget>

class QPushButton;

class ConfigWidget : public QWidget
{
   Q_OBJECT

signals:
   void signalOpenRepo(const QString &repoPath);

public:
   explicit ConfigWidget(QWidget *parent = nullptr);

private:
   QPushButton *mOpenRepo = nullptr;
   QPushButton *mCloneRepo = nullptr;
   QPushButton *mInitRepo = nullptr;

   void openRepo();
};

#endif // CONFIG_H
