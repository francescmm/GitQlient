#ifndef GIQLIENT_H
#define GIQLIENT_H

#include <QWidget>
#include <QMap>

class QTabWidget;
class GitQlientRepo;

class GitQlient : public QWidget
{
   Q_OBJECT
public:
   explicit GitQlient(QWidget *parent = nullptr);

   void setRepositories(const QStringList repositories);

private:
   bool mFirstRepoInitialized = false;
   QTabWidget *mRepos = nullptr;
   QMap<GitQlientRepo *, QString> mCurrentRepos;

   void setRepoName(const QString &repoName);
   void openRepo();
   void addRepoTab(const QString &repoPath = "");
   void repoClosed(int tabIndex);
   void closeTab(int tabIndex);
};

#endif // GIQLIENT_H
