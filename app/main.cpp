#include <QApplication>
#include <QSettings>
#include <QtSingleApplication>

#include <GitQlient.h>
#include <QLogger.h>

#include <QDebug>

using namespace QLogger;

int main(int argc, char *argv[])
{
   QtSingleApplication app(argc, argv);
   QStringList arguments;

   while (argc--)
      arguments.prepend(argv[argc]);

   if (app.sendMessage(arguments.join(",")))
      return 0;

   app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

   QtSingleApplication::setOrganizationName("CescSoftware");
   QtSingleApplication::setOrganizationDomain("francescmm.com");
   QtSingleApplication::setApplicationName(APP_NAME);

   const auto mainWin = new GitQlient(arguments);
   mainWin->showMaximized();

   QObject::connect(&app, &QtSingleApplication::messageReceived,
                    [mainWin](const QString &message) { mainWin->setArgumentsPostInit(message.split(",")); });
   QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

   const auto ret = app.exec();

   return ret;
}
