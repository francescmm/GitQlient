#ifndef REPOCONFIGDLG_H
#define REPOCONFIGDLG_H

#include <QDialog>
#include <QMap>

class GitBase;
class QGridLayout;
class QLineEdit;

namespace Ui
{
class RepoConfigDlg;
}

class RepoConfigDlg : public QDialog
{
   Q_OBJECT

public:
   explicit RepoConfigDlg(const QSharedPointer<GitBase> &git, QWidget *parent = nullptr);
   ~RepoConfigDlg();

private:
   Ui::RepoConfigDlg *ui = nullptr;
   QSharedPointer<GitBase> mGit;
   QMap<QLineEdit *, QString> mLineeditKeyMap;

   void addUserConfig(const QStringList &elements, QGridLayout *layout);
   void setConfig();
   void clearCache();
   void calculateCacheSize();
   void toggleBsAccesInfo();
};

#endif // REPOCONFIGDLG_H
