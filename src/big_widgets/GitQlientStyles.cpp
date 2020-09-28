#include "GitQlientStyles.h"

#include <QFile>
#include <GitQlientSettings.h>

GitQlientStyles *GitQlientStyles::INSTANCE = nullptr;

GitQlientStyles *GitQlientStyles::getInstance()
{
   if (INSTANCE == nullptr)
      INSTANCE = new GitQlientStyles();

   return INSTANCE;
}

QString GitQlientStyles::getStyles()
{
   QString styles;
   QFile stylesFile(":/stylesheet");

   if (stylesFile.open(QIODevice::ReadOnly))
   {
      GitQlientSettings settings;
      const auto colorSchema = settings.globalValue("colorSchema", "dark").toString();
      QFile colorsFile(QString(":/colors_%1").arg(colorSchema));
      QString colorsCss;

      if (colorsFile.open(QIODevice::ReadOnly))
      {
         colorsCss = QString::fromUtf8(colorsFile.readAll());
         colorsFile.close();
      }

      styles = stylesFile.readAll() + colorsCss;

      stylesFile.close();
   }

   return styles;
}

QColor GitQlientStyles::getTextColor()
{
   auto textColor = QColor("white");
   GitQlientSettings settings;
   const auto colorSchema = settings.globalValue("colorSchema", "dark").toString();

   if (colorSchema == "bright")
      textColor = "black";

   return textColor;
}

QColor GitQlientStyles::getGraphSelectionColor()
{
   GitQlientSettings settings;
   const auto colorSchema = settings.globalValue("colorSchema", "dark").toString();

   QColor c;

   if (colorSchema == "dark")
      c.setNamedColor("#505152");
   else
      c.setNamedColor("#C6C6C7");

   return c;
}

QColor GitQlientStyles::getGraphHoverColor()
{
   GitQlientSettings settings;
   const auto colorSchema = settings.globalValue("colorSchema", "dark").toString();

   QColor c;

   if (colorSchema == "dark")
      c.setNamedColor("#404142");
   else
      c.setNamedColor("#EFEFEF");

   return c;
}

QColor GitQlientStyles::getBackgroundColor()
{
   GitQlientSettings settings;
   const auto colorSchema = settings.globalValue("colorSchema", "dark").toString();

   QColor c;
   c.setNamedColor(colorSchema == "dark" ? QString("#2E2F30") : QString("white"));

   return c;
}

QColor GitQlientStyles::getTabColor()
{
   GitQlientSettings settings;
   const auto colorSchema = settings.globalValue("colorSchema", "dark").toString();

   QColor c;
   c.setNamedColor(colorSchema == "dark" ? QString("#404142") : QString("white"));

   return c;
}

QColor GitQlientStyles::getBlue()
{
   GitQlientSettings settings;
   const auto colorSchema = settings.globalValue("colorSchema", "dark").toString();

   QColor c;
   c.setNamedColor(QString::fromUtf8(colorSchema == "dark" ? "#579BD5" : "#325CC7"));

   return c;
}

QColor GitQlientStyles::getRed()
{
   return QColor("#FF2222");
}

QColor GitQlientStyles::getGreen()
{
   return QColor("#65952B");
}

QColor GitQlientStyles::getOrange()
{
   return QColor("#FF9320");
}

QColor GitQlientStyles::getGitQlientOrange()
{
   return QColor("#D89000");
}

std::array<QColor, GitQlientStyles::kBranchColors> GitQlientStyles::getBranchColors()
{
   static std::array<QColor, kBranchColors> colors { { getTextColor(), getRed(), getBlue(), getGreen(), getOrange(),
                                                       QColor("#848484") /* grey */, QColor("#FF79C6") /* pink */,
                                                       QColor("#CD9077") /* pastel */ } };

   return colors;
}

QColor GitQlientStyles::getBranchColorAt(int index)
{
   if (index < kBranchColors && index >= 0)
      return getBranchColors().at(static_cast<size_t>(index));

   return QColor();
}

QColor GitQlientStyles::getCurrentBranchColor()
{
   static auto currentBranch = QColor("#005b96");
   return currentBranch;
}

QColor GitQlientStyles::getLocalBranchColor()
{
   static auto localBranch = QColor("#6497b1");
   return localBranch;
}

QColor GitQlientStyles::getRemoteBranchColor()
{
   static auto remoteBranch = QColor("#011f4b");
   return remoteBranch;
}

QColor GitQlientStyles::getDetachedColor()
{
   static auto detached = QColor("#851e3e");
   return detached;
}

QColor GitQlientStyles::getTagColor()
{
   static auto tag = QColor("#dec3c3");
   return tag;
}

QColor GitQlientStyles::getRefsColor()
{
   static auto refs = QColor("#FF5555");
   return refs;
}
