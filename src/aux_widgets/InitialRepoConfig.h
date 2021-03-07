#pragma once

#include <QDialog>

namespace Ui
{
class InitialRepoConfig;
}

class GitQlientSettings;

class InitialRepoConfig : public QDialog
{
   Q_OBJECT

public:
   explicit InitialRepoConfig(const QSharedPointer<GitQlientSettings> &settings, QWidget *parent = nullptr);
   ~InitialRepoConfig();

private:
   Ui::InitialRepoConfig *ui;
   QSharedPointer<GitQlientSettings> mSettings;
};
