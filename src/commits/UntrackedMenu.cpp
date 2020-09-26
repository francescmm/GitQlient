#include <UntrackedMenu.h>
#include <GitBase.h>
#include <QLogger.h>

using namespace QLogger;

#include <QProcess>
#include <QMessageBox>
#include <QDir>
#include <QFile>

UntrackedMenu::UntrackedMenu(const QSharedPointer<GitBase> &git, const QString &fileName, QWidget *parent)
   : QMenu(parent)
   , mGit(git)
   , mFileName(fileName)
{
   connect(addAction(tr("Stage file")), &QAction::triggered, this, &UntrackedMenu::signalStageFile);
   connect(addAction(tr("Delete file")), &QAction::triggered, this, &UntrackedMenu::onDeleteFile);

   const auto ignoreMenu = addMenu(tr("Ignore"));

   connect(ignoreMenu->addAction("Ignore file"), &QAction::triggered, this, [this]() {
      const auto ret = QMessageBox::question(this, tr("Ignoring file"),
                                             tr("Are you sure you want to add the file to the black list?"));

      if (ret == QMessageBox::Yes)
      {
         const auto gitRet = addEntryToGitIgnore(mFileName);

         if (gitRet)
            emit signalCheckoutPerformed();
      }
   });

   connect(ignoreMenu->addAction("Ignore extension"), &QAction::triggered, this, [this]() {
      const auto msgBoxRet = QMessageBox::question(
          this, tr("Ignoring file"), tr("Are you sure you want to add the file extension to the black list?"));

      if (msgBoxRet == QMessageBox::Yes)
      {
         auto fileParts = mFileName.split(".");
         fileParts.takeFirst();
         const auto extension = QString("*.%1").arg(fileParts.join("."));
         const auto ret = addEntryToGitIgnore(extension);

         if (ret)
            emit signalCheckoutPerformed();
      }
   });

   connect(ignoreMenu->addAction("Ignore containing folder"), &QAction::triggered, this, [this]() {
      const auto msgBoxRet = QMessageBox::question(
          this, tr("Ignoring folder"), tr("Are you sure you want to add the containing folder to the black list?"));

      if (msgBoxRet == QMessageBox::Yes)
      {
         const auto path = mFileName.lastIndexOf("/");
         const auto folder = mFileName.left(path);
         const auto extension = QString("%1/*").arg(folder);
         const auto ret = addEntryToGitIgnore(extension);

         if (ret)
            emit signalCheckoutPerformed();
      }
   });
}

void UntrackedMenu::onDeleteFile()
{
   const auto path = QString("%1").arg(mFileName);

   QLog_Info("UI", "Removing paht: " + path);

   QProcess p;
   p.setWorkingDirectory(mGit->getWorkingDir());
   p.start("rm", { "-rf", path });

   if (p.waitForFinished())
      emit signalCheckoutPerformed();
}

bool UntrackedMenu::addEntryToGitIgnore(const QString &entry)
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
         QMessageBox::critical(this, tr("Unable to add the entry"),
                               tr("It was impossible to add the entry in the .gitignore file."));

      f.close();
   }

   return entryAdded;
}
