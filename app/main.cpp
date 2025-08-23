#include <QApplication>
#include <QFontDatabase>
#include <QIcon>
#include <QTimer>
#include <QTranslator>

#include <GitQlient.h>
#include <GitQlientSettings.h>
#include <QLogger.h>

using namespace QLogger;

int main(int argc, char *argv[])
{
   QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

   QApplication app(argc, argv);

   QApplication::setOrganizationName("CescSoftware");
   QApplication::setOrganizationDomain("francescmm.com");
   QApplication::setApplicationName("GitQlient");
   QApplication::setApplicationVersion(VER);
   QApplication::setWindowIcon(QIcon(":/icons/GitQlientLogoIco"));

   QFontDatabase::addApplicationFont(":/DejaVuSans");
   QFontDatabase::addApplicationFont(":/DejaVuSansMono");

   const auto languageFile = GitQlientSettings().globalValue("UILanguage", "gitqlient_en").toString();
   QTranslator qtTranslator;

   if (qtTranslator.load(languageFile, QString::fromUtf8(":/translations/")))
      app.installTranslator(&qtTranslator);

   QStringList repos;
   if (GitQlient::parseArguments(app.arguments(), &repos))
   {
      GitQlient mainWin;
      mainWin.setRepositories(repos);
      mainWin.restorePinnedRepos();
      mainWin.show();

      return app.exec();
   }

   return 0;
}
