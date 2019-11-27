#include "GitQlientSettings.h"

void GitQlientSettings::setValue(const QString &key, const QVariant &value)
{
   QSettings::setValue(key, value);

   emit valueChanged(key, value);
}

void GitQlientSettings::setProjectOpened(const QString &projectPath)
{
   auto projects = QSettings::value("recentProjects", QStringList()).toStringList();
   auto timesUsed = QSettings::value("recentProjectsCount", QString()).toList();
   int count = 1;

   if (projects.contains(projectPath))
   {
      const auto index = projects.indexOf(projectPath);
      timesUsed[index] = count + timesUsed[index].toInt();
   }
   else
   {
      projects.append(projectPath);
      timesUsed.append(count);
   }

   GitQlientSettings::setValue("recentProjects", projects);
   GitQlientSettings::setValue("recentProjectsCount", timesUsed);
}

QVector<QString> GitQlientSettings::getRecentProjects() const
{
   const auto projects = QSettings::value("recentProjects", QStringList()).toStringList();
   QVector<QString> recentProjects;
   const auto end = std::min(projects.size(), 5);

   for (auto i = 0; i < end; ++i)
      recentProjects.append(projects.at(i));

   return recentProjects;
}
