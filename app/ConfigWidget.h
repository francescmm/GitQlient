#ifndef CONFIG_H
#define CONFIG_H

#include <QFrame>

class QPushButton;
class QButtonGroup;

class ConfigWidget : public QFrame
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
   QButtonGroup *mBtnGroup = nullptr;

   void openRepo();
   QWidget *createConfigWidget();
   QWidget *createConfigPage();
   QWidget *createGeneralPage();
   QWidget *createProfilesPage();
};

#endif // CONFIG_H
