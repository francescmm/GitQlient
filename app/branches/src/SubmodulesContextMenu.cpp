#include "SubmodulesContextMenu.h"

#include <AddSubmoduleDlg.h>
#include <BranchDlg.h>
#include <GitBase.h>
#include <GitQlientStyles.h>
#include <GitStashes.h>
#include <GitSubmodules.h>

#include <QApplication>
#include <QMessageBox>
#include <QModelIndex>

SubmodulesContextMenu::SubmodulesContextMenu(const QSharedPointer<GitBase> &git, const QModelIndex &index,
                                             int totalSubmodules, QWidget *parent)
   : QMenu(parent)
   , mGit(git)
{
   setAttribute(Qt::WA_DeleteOnClose);

   if (!index.isValid())
   {
      const auto addSubmoduleAction = addAction(tr("Add submodule"));
      connect(addSubmoduleAction, &QAction::triggered, this, [this] {
         const auto git = QSharedPointer<GitSubmodules>::create(mGit);
         AddSubmoduleDlg addDlg(git);
         const auto ret = addDlg.exec();
         if (ret == QDialog::Accepted)
            emit infoUpdated();
      });

      if (totalSubmodules > 0)
      {
         const auto updateSubmoduleAction = addAction(tr("Update all"));
         connect(updateSubmoduleAction, &QAction::triggered, this, [this]() {
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            QScopedPointer<GitSubmodules> git(new GitSubmodules(mGit));
            const auto ret = git->submoduleUpdate({});
            QApplication::restoreOverrideCursor();

            if (ret)
               emit infoUpdated();
         });
      }
   }
   else
   {
      const auto submoduleName = index.data().toString();
      const auto updateSubmoduleAction = addAction(tr("Update"));
      connect(updateSubmoduleAction, &QAction::triggered, this, [this, submoduleName]() {
         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
         QScopedPointer<GitSubmodules> git(new GitSubmodules(mGit));
         const auto ret = git->submoduleUpdate(submoduleName);
         QApplication::restoreOverrideCursor();

         if (ret)
            emit infoUpdated();
      });

      const auto openSubmoduleAction = addAction(tr("Open"));
      connect(openSubmoduleAction, &QAction::triggered, this,
              [this, submoduleName]() { emit openSubmodule(mGit->getWorkingDir().append("/").append(submoduleName)); });

      const auto deleteSubmoduleAction = addAction(tr("Delete"));
      connect(deleteSubmoduleAction, &QAction::triggered, this, [this, submoduleName]() {
         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
         QScopedPointer<GitSubmodules> git(new GitSubmodules(mGit));
         const auto ret = git->submoduleRemove(submoduleName);
         QApplication::restoreOverrideCursor();

         if (ret)
            emit infoUpdated();
      });
   }
}

SubmodulesContextMenu::SubmodulesContextMenu(const QSharedPointer<GitBase> &git, const QString &name, QWidget *parent)
   : QMenu(parent)
   , mGit(git)
{
   const auto submoduleName = name;
   const auto updateSubmoduleAction = addAction(tr("Update"));
   connect(updateSubmoduleAction, &QAction::triggered, this, [this, submoduleName]() {
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      QScopedPointer<GitSubmodules> git(new GitSubmodules(mGit));
      const auto ret = git->submoduleUpdate(submoduleName);
      QApplication::restoreOverrideCursor();

      if (ret)
         emit infoUpdated();
   });

   const auto openSubmoduleAction = addAction(tr("Open"));
   connect(openSubmoduleAction, &QAction::triggered, this,
           [this, submoduleName]() { emit openSubmodule(mGit->getWorkingDir().append("/").append(submoduleName)); });
}
