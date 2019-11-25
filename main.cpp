#include <QApplication>
#include <QSettings>
#include <QtSingleApplication>
#include <QFontDatabase>

#include <GitQlient.h>
#include <QLogger.h>

#include <QDebug>

using namespace QLogger;

int main(int argc, char *argv[])
{
   qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");

   QtSingleApplication app(argc, argv);
   QStringList arguments;

   auto argNum = argc;
   while (argNum--)
      arguments.prepend(argv[argNum]);

   if (app.sendMessage(arguments.join(",")))
      return 0;

   QtSingleApplication::setOrganizationName("CescSoftware");
   QtSingleApplication::setOrganizationDomain("francescmm.com");
   QtSingleApplication::setApplicationName(APP_NAME);
   QFontDatabase::addApplicationFont(":/Ubuntu");
   QFontDatabase::addApplicationFont(":/UbuntuMono");

   GitQlient mainWin(arguments);
   mainWin.showMaximized();

   QObject::connect(&app, &QtSingleApplication::messageReceived,
                    [&mainWin](const QString &message) { mainWin.setArgumentsPostInit(message.split(",")); });
   QObject::connect(&app, &QtSingleApplication::lastWindowClosed, &app, &QtSingleApplication::quit);

   const auto ret = app.exec();

   return ret;
}
