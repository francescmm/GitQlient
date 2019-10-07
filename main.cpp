/*
        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#include <QSettings>
#include <QApplication>
#include <QFile>

#include "common.h"
#include "MainWindow.h"

using namespace QGit;

int main(int argc, char *argv[])
{

   QApplication app(argc, argv);
   app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

   const auto mainWin = new MainWindow();
   mainWin->show();

   QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

   return app.exec();
}
