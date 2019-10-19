#include "UnstagedFilesContextMenu.h"

#include <git.h>
#include <GitSyncProcess.h>

#include <QFile>
#include <QDir>
#include <QMessageBox>

UnstagedFilesContextMenu::UnstagedFilesContextMenu(QSharedPointer<Git> git, const QString &fileName, QWidget *parent)
   : QMenu(parent)
   , mGit(git)
   , mFileName(fileName)
{

   connect(addAction("Revert file changes"), &QAction::triggered, this, [this, fileName]() {
      const auto ret = mGit->resetFile(fileName);

      emit signalCheckedOut(ret);
   });

   connect(addAction("Ignore file name"), &QAction::triggered, this, [this, fileName]() {
      const auto ret = addEntryToGitIgnore(fileName);

      if (ret)
         emit signalCheckedOut(ret);
   });

   connect(addAction("Ignore file extension"), &QAction::triggered, this, [this, fileName]() {
      auto fileParts = fileName.split(".");
      fileParts.takeFirst();
      const auto extension = QString("*.%1").arg(fileParts.join("."));
      const auto ret = addEntryToGitIgnore(extension);

      if (ret)
         emit signalCheckedOut(ret);
   });

   QAction *removeAction = nullptr;
   connect(removeAction = addAction("Remove file"), &QAction::triggered, this, []() {});
   removeAction->setDisabled(true);

   addSeparator();

   connect(addAction("Add all files to commit"), &QAction::triggered, this, &UnstagedFilesContextMenu::signalCommitAll);
   connect(addAction("Revert all changes"), &QAction::triggered, this, [this, fileName]() { emit signalRevertAll(); });
}

bool UnstagedFilesContextMenu::addEntryToGitIgnore(const QString &entry)
{
   auto entryAdded = false;
   QDir d(mGit->getWorkingDir());
   QFile f(d.absolutePath() + "/.gitignore");

   if (!f.exists())
   {
      if (f.open(QIODevice::ReadWrite))
         f.close();
   }

   if (f.open(QIODevice::Append))
   {
      const auto bytesWritten = f.write(entry.toUtf8() + "\n");

      if (bytesWritten != -1)
      {
         QMessageBox::information(this, tr("File added to .gitignore"),
                                  tr("The file has been added to the ignore list in the file .gitignore."));

         entryAdded = true;
      }
      else
         QMessageBox::warning(this, tr("Unable to add the entry"),
                              tr("It was impossible to add the entry in the .gitignore file."));

      f.close();
   }

   return entryAdded;
}
