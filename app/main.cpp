#include <QApplication>
#include <QSettings>

#include <GitQlient.h>
#include <QLogger.h>

using namespace QLogger;

int main(int argc, char *argv[])
{
   QApplication app(argc, argv);
   app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

   QSettings settings("Francesc M.", "GitQlient");

   const auto mainWin = new GitQlient(argc, argv);
   mainWin->showMaximized();

   QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

   const auto ret = app.exec();

   return ret;
}
