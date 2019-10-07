/*
        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#ifndef REVSVIEW_H
#define REVSVIEW_H

#include <domain.h>

class MainWindow;
class Git;
class RepositoryModel;
class DiffWidget;
class RepositoryView;

class RevsView : public Domain
{
   Q_OBJECT

signals:
   void signalViewUpdated();
   void signalOpenDiff(const QString &sha);
   void rebase(const QString &from, const QString &to, const QString &onto);
   void merge(const QStringList &shas, const QString &into);
   void moveRef(const QString &refName, const QString &toSHA);

public:
   explicit RevsView(bool isMain = false);
   ~RevsView() override;
   void clear(bool complete) override;
   void setEnabled(bool enabled);
   RepositoryView *getRepoList() { return listViewLog; }

private slots:
   void on_newRevsAdded(const RepositoryModel *, const QVector<QString> &);
   void on_loadCompleted(const RepositoryModel *, const QString &stats);

protected:
   bool doUpdate(bool force) override;

private:
   friend class MainWindow;
   RepositoryView *listViewLog = nullptr;
};

#endif
