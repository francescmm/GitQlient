/*
        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#include <QApplication>

#include <GitQlient.h>
#include <QLogger.h>

using namespace QLogger;

int main(int argc, char *argv[])
{
   auto logsEnabled = true;
   QStringList repos;
   auto i = 0;

   while (i < argc)
   {
      if (QString(argv[i++]) == "-noLog")
         logsEnabled = false;
      else if (QString(argv[i]) == "-repos")
      {
         while (i < argc - 1 && !QString(argv[++i]).startsWith("-"))
            repos.append(argv[i]);
      }
   }

   QApplication app(argc, argv);
   app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

   const auto manager = QLoggerManager::getInstance();

   if (logsEnabled)
      manager->addDestination("GitQlient.log", { "UI", "Git" }, LogLevel::Debug);

   QLog_Info("UI", "*******************************************");
   QLog_Info("UI", "*          GitQlient has started          *");
   QLog_Info("UI", "*                 -alpha-                 *");
   QLog_Info("UI", "*******************************************");

   const auto mainWin = new GitQlient();

   if (!repos.isEmpty())
      mainWin->setRepositories(repos);

   mainWin->showMaximized();

   QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

   const auto ret = app.exec();

   QLog_Info("UI", "*            Closing GitQlient            *");

   return ret;
}
