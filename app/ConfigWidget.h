#ifndef CONFIG_H
#define CONFIG_H

#include <QFrame>

class QPushButton;
class QButtonGroup;
class Git;

class ConfigWidget : public QFrame
{
   Q_OBJECT

signals:
   void signalOpenRepo(const QString &repoPath);

public:
   explicit ConfigWidget(QWidget *parent = nullptr);

private:
   QSharedPointer<Git> mGit;
   QPushButton *mOpenRepo = nullptr;
   QPushButton *mCloneRepo = nullptr;
   QPushButton *mInitRepo = nullptr;
   QButtonGroup *mBtnGroup = nullptr;

   void openRepo();
   void cloneRepo();
   void initRepo();
   QWidget *createConfigWidget();
   QWidget *createConfigPage();
};

#endif // CONFIG_H
