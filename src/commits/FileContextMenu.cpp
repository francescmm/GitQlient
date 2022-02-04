#include "FileContextMenu.h"

#include <GitQlientSettings.h>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QMessageBox>
#include <QProcess>

FileContextMenu::FileContextMenu(const QString gitProject, const QString &file, bool editionAllowed, QWidget *parent)
   : QMenu(parent)
   , mFile(file)
   , mGitProject(gitProject.endsWith("/") ? gitProject : gitProject + QString("/"))
{
   setAttribute(Qt::WA_DeleteOnClose);

   const auto fileHistoryAction = addAction(tr("History"));
   fileHistoryAction->setEnabled(false);

   connect(addAction(tr("Blame")), &QAction::triggered, this, &FileContextMenu::signalShowFileHistory);

   const auto fileDiffAction = addAction(tr("Diff"));
   connect(fileDiffAction, &QAction::triggered, this, &FileContextMenu::signalOpenFileDiff);

   addSeparator();

   if (editionAllowed)
   {
      connect(addAction(tr("Edit file")), &QAction::triggered, this, &FileContextMenu::signalEditFile);

      addSeparator();
   }

   connect(addAction(tr("Open containing folder")), &QAction::triggered, this, &FileContextMenu::openFileExplorer);
   connect(addAction(tr("Copy path")), &QAction::triggered, this,
           [this]() { QApplication::clipboard()->setText(mFile); });
}

void FileContextMenu::openFileExplorer()
{
   QString absoluteFilePath = mGitProject + mFile;
   absoluteFilePath = absoluteFilePath.left(absoluteFilePath.lastIndexOf("/"));
   QString app;
   QStringList arguments;
#ifdef Q_OS_LINUX
   GitQlientSettings settings;
   const auto fileExplorer = settings.globalValue("FileBrowser", "xdg-open").toString();

   if (fileExplorer.isEmpty())
   {
      QMessageBox::critical(parentWidget(), tr("Error opening file explorer"),
                            tr("The file explorer value in the settings is invalid. Please define what file explorer "
                               "you want to use to open file locations."));
      return;
   }

   arguments = fileExplorer.split(" ");
   arguments.append(absoluteFilePath);
   app = arguments.takeFirst();
#elif defined(Q_OS_WIN)
   app = QString::fromUtf8("explorer.ext");
   arguments = QStringList { "/select", QDir::toNativeSeparators(absoluteFilePath) };
#elif defined(Q_OS_OSX)
   app = QString::fromUtf8("/usr/bin/open");
   arguments = QStringList { "-R", absoluteFilePath };
#endif

   auto ret = QProcess::startDetached(app, arguments);

   if (!ret)
   {
      QMessageBox::critical(parentWidget(), tr("Error opening file explorer"),
                            tr("There was a problem opening the file explorer."));
   }
}
