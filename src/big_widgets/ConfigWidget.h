#pragma once

#include <QWidget>
#include <QMap>

class GitBase;
class QTimer;
class FileEditor;
class QPushButton;

namespace Ui
{
class ConfigWidget;
}

class ConfigWidget : public QWidget
{
   Q_OBJECT

signals:
   void reloadView();

public:
   explicit ConfigWidget(const QSharedPointer<GitBase> &git, QWidget *parent = nullptr);
   ~ConfigWidget();

private:
   Ui::ConfigWidget *ui;
   QSharedPointer<GitBase> mGit;
   int mOriginalRepoOrder = 0;
   bool mShowResetMsg = false;
   QTimer *mFeedbackTimer = nullptr;
   QPushButton *mSave = nullptr;
   QMap<int, FileEditor *> mEditors;

   void clearCache();
   void calculateCacheSize();
   void toggleBsAccesInfo();
   void enableWidgets();
   void saveFile();

private slots:
   void saveConfig();
};
