#ifndef REPOCONFIGDLG_H
#define REPOCONFIGDLG_H

#include <QDialog>
#include <QMap>
#include <QTranslator>

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

signals:
   void reloadView();

public:
   explicit RepoConfigDlg(const QSharedPointer<GitBase> &git, QWidget *parent = nullptr);
   ~RepoConfigDlg();

private:
   Ui::RepoConfigDlg *ui = nullptr;
   QSharedPointer<GitBase> mGit;
   QMap<QLineEdit *, QString> mLineeditKeyMap;
   QTranslator mTranslator; // contains the translations for this application
   QTranslator mTranslatorQt; // contains the translations for qt
   QString mCurrLang; // contains the currently loaded language
   QString mLangPath; // Path of language files. This is always fixed to /languages.
   int mOriginalRepoOrder = 0;

   void addUserConfig(const QStringList &elements, QGridLayout *layout);
   void setConfig();
   void clearCache();
   void calculateCacheSize();
   void toggleBsAccesInfo();
   void createLanguageMenu();
   void switchTranslator(QTranslator &translator, const QString &filename);
   void loadLanguage(const QString &rLanguage);

private slots:
   void onLanguageChanged(int index);
};

#endif // REPOCONFIGDLG_H
