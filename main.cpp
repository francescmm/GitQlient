#include <QApplication>
#include <QSettings>
#include <QFontDatabase>

#include <GitQlient.h>
#include <QLogger.h>

#include <QDebug>

using namespace QLogger;

int main(int argc, char *argv[])
{
   qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");

   QApplication app(argc, argv);
   QStringList arguments;

   auto argNum = argc;

   while (argNum--)
      arguments.prepend(argv[argNum]);

   QApplication::setOrganizationName("CescSoftware");
   QApplication::setOrganizationDomain("francescmm.com");
   QApplication::setApplicationName(APP_NAME);

   QFontDatabase::addApplicationFont(":/Ubuntu");
   QFontDatabase::addApplicationFont(":/UbuntuMono");

   GitQlient mainWin(arguments);
   mainWin.showMaximized();

   const auto ret = app.exec();

   return ret;
}
